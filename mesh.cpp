#include "mesh.hpp"

#include <fstream>
#include <boost/range/algorithm/max_element.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/numeric.hpp>
#include <boost/filesystem.hpp>
//#include <boost/algorithm/string.hpp>
#include <CGAL/IO/read_off_points.h>
//#include <CGAL/IO/STL_reader.h>
#include <CGAL/centroid.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_2.h>

#include "ratios.hpp"

Mesh::Mesh(std::istream &stream, const format form) {
  std::vector<Point> points;
  switch (form) {
  case STL: {
    std::vector<std::array<double, 3> > temp_points;
    std::vector<std::array<int, 3> > facets;
    
    // read_STL has no error handling (it's an undocumented function after all).
    // It looks for the word "solid" at the beginning of the file, and if it
    // fails it treats it as if it was in the binary STL format. This means
    // that it skips the first 80 bytes then interprets the next 4 bytes as the
    // number of triangles. The problem is that if it's not a valid STL file
    // it can be a huge number. The function then tries to read this many
    // triangles ignoring if it has reached the end of file yet which in turn
    // leads to a seemingly infinite loop.
    try {
      //      if (!CGAL::read_STL(stream, temp_points, facets))
      //	throw "Invalid file";
    } catch (...) {
    }
    if (temp_points.size() < 4)
      throw "Not enough points";
    points.reserve(temp_points.size());
    for (auto &&p : temp_points)
      points.emplace_back(p[0], p[1], p[2]);
    break;}
  case OFF:
    if (!CGAL::read_off_points(stream, std::back_inserter(points)))
      throw "Invalid file";
    break;
  default:
    throw "Unknown format";
  }
  initialize(points);
}

void Mesh::initialize(std::vector<Point> &points) {
  if (points.size() < 4)
    throw "Not enough points";

  std::transform(points.begin(),
		 points.end(),
		 points.begin(),
		 Transform(CGAL::TRANSLATION,
			   Vector(CGAL::centroid(points.begin(), points.end()),
				  Point(CGAL::ORIGIN))));

  CGAL::convex_hull_3(points.begin(), points.end(), m_surface_mesh);
  
  if (m_surface_mesh.number_of_faces() < 4)
    throw "Not enough faces";
}

void Mesh::measure() {
  m_bounding_box = CGAL::bounding_box(m_surface_mesh.points().begin(), m_surface_mesh.points().end());
  m_surface_area = boost::accumulate(CGAL::faces(m_surface_mesh),
				     double(0.0),
				     [this](const auto &init, const auto &face) {
        return init + CGAL::sqrt(this->face_triangle(face).squared_area());
    });
    m_volume = boost::accumulate(CGAL::faces(m_surface_mesh),
                                 double(0.0),
                                 [this](auto init, auto face) {
        const auto t = this->face_triangle(face);
        return init + CGAL::abs(CGAL::volume(Point(CGAL::ORIGIN), t[0], t[1], t[2]));
    });

    double max_squared_a = 0, max_squared_b = 0, max_squared_c = 0;
    Vector a, b, c;

    for (auto point1 = m_surface_mesh.points().begin();
         point1 != std::prev(m_surface_mesh.points().end());
         ++point1) {
        for (auto point2 = std::next(point1);
             point2 != m_surface_mesh.points().end();
             ++point2) {
            const auto squared_distance = CGAL::squared_distance(*point1, *point2);
            if (squared_distance > max_squared_a) {
                max_squared_a = squared_distance;
                a = Vector(*point1, *point2);
            }
        }
    }

    const Plane projection_plane(CGAL::ORIGIN, a);
    for (auto point1 = m_surface_mesh.points().begin();
         point1 != std::prev(m_surface_mesh.points().end());
         ++point1) {
        const auto projected1 = projection_plane.projection(*point1);
        for (auto point2 = std::next(point1);
             point2 != m_surface_mesh.points().end();
             ++point2) {
            const auto projected2 = projection_plane.projection(*point2);
            const auto squared_distance = CGAL::squared_distance(projected1, projected2);
            if (squared_distance > max_squared_b) {
                max_squared_b = squared_distance;
                b = Vector(projected1, projected2);
            }
        }
    }

    c = CGAL::cross_product(a, b);
    const Line projection_line(CGAL::ORIGIN, c);
    for (auto point1 = m_surface_mesh.points().begin();
         point1 != std::prev(m_surface_mesh.points().end());
         ++point1) {
        const auto projected1 = projection_line.projection(*point1);
        for (auto point2 = std::next(point1);
             point2 != m_surface_mesh.points().end();
             ++point2) {
            const auto projected2 = projection_line.projection(*point2);
            const auto squared_distance = CGAL::squared_distance(projected1, projected2);
            if (squared_distance > max_squared_c)
                max_squared_c = squared_distance;
        }
    }

    m_a = CGAL::sqrt(max_squared_a);
    m_b = CGAL::sqrt(max_squared_b);
    m_c = CGAL::sqrt(max_squared_c);

    const Vector x = a / CGAL::sqrt(a.squared_length()),
            y = b / CGAL::sqrt(b.squared_length()),
            z = c / CGAL::sqrt(c.squared_length());
    const Transform isoper_basis(x.x(), x.y(), x.z(),
                                      y.x(), y.y(), y.z(),
                                      z.x(), z.y(), z.z());

    // project onto the ab plane
    const Plane isoper_plane(CGAL::ORIGIN, c);
    std::vector<Kernel::Point_2> projected_points;
    projected_points.reserve(m_surface_mesh.number_of_vertices());
    for (auto point : m_surface_mesh.points()) {
        const auto projected = isoper_basis(isoper_plane.projection(point));
        projected_points.emplace_back(projected.x(), projected.y());
    }

    // get the outline
    CGAL::Polygon_2<Kernel> outline;
    CGAL::convex_hull_2(projected_points.cbegin(), projected_points.cend(), std::back_inserter(outline));
    m_projected_circumference = std::accumulate(outline.edges_begin(), outline.edges_end(), double(0.0), [](auto init, const auto &edge) {
        return init + CGAL::sqrt(edge.squared_length());
    });
    m_projected_area = outline.area();

    const std::unordered_map<std::string, double> inputs({
	{"surface_area", m_surface_area},
	  {"volume", m_volume},
	    {"a", m_a},
	      {"b", m_b},
		{"c", m_c},
		  {"projected_circumference", m_projected_circumference},
		    {"projected_area", m_projected_area}
      });
    m_ratios = Ratios::calculate(inputs);
}

FaceRange<Mesh, Mesh::Face_iterator> Mesh::faces() const {
    const auto range = m_surface_mesh.faces();
    return FaceRange<Mesh, Face_iterator>(FaceIterator<Mesh, Face_iterator>(this, range.begin(), range.end()),
                                    FaceIterator<Mesh, Face_iterator>(this, range.end(), range.end(), number_of_faces()));
}

VertexRange<Mesh, Mesh::Vertex_iterator> Mesh::vertices() const {
    const auto range = m_surface_mesh.vertices();
    return VertexRange<Mesh, Vertex_iterator>(VertexIterator<Mesh, Vertex_iterator>(this, range.begin(), range.end()),
                                        VertexIterator<Mesh, Vertex_iterator>(this, range.end(), range.end(), number_of_vertices()));
}

VertexRange<Mesh, Mesh::Vertex_around_face_iterator> Mesh::vertices_around_face(Face_handle face) const {
    const auto range = CGAL::vertices_around_face(m_surface_mesh.halfedge(face), m_surface_mesh);
    return VertexRange<Mesh, Vertex_around_face_iterator>(VertexIterator<Mesh, Vertex_around_face_iterator>(this, range.begin(), range.end()),
                                                    VertexIterator<Mesh, Vertex_around_face_iterator>(this, range.end(), range.end(), 3));
}

Vertex_base<Mesh> Mesh::source(Halfedge_handle halfedge) const {
    return Vertex_base<Mesh>(this, m_surface_mesh.source(halfedge));
}

Vertex_base<Mesh> Mesh::target(Halfedge_handle halfedge) const {
    return Vertex_base<Mesh>(this, m_surface_mesh.target(halfedge));
}

Halfedge_base<Mesh> Mesh::next(Halfedge_handle halfedge) const {
    return Halfedge_base<Mesh>(this, m_surface_mesh.next(halfedge));
}

Halfedge_base<Mesh> Mesh::opposite(Halfedge_handle halfedge) const {
    return Halfedge_base<Mesh>(this, m_surface_mesh.opposite(halfedge));
}

Halfedge_base<Mesh> Mesh::halfedge(Vertex_handle source, Vertex_handle target) const {
    return Halfedge_base<Mesh>(this, m_surface_mesh.halfedge(source, target));
}

HalfedgeRange<Mesh, Mesh::Halfedge_iterator> Mesh::halfedges() const {
    const auto range = m_surface_mesh.halfedges();
    return HalfedgeRange<Mesh, Halfedge_iterator>(HalfedgeIterator<Mesh, Halfedge_iterator>(this, range.begin(), range.end()),
                                            HalfedgeIterator<Mesh, Halfedge_iterator>(this, range.end(), range.end(), number_of_halfedges()));
}

HalfedgeRange<Mesh, Mesh::Halfedge_around_face_iterator> Mesh::halfedges_around_face(Face_handle face) const {
    const auto range = CGAL::halfedges_around_face(m_surface_mesh.halfedge(face), m_surface_mesh);
    return HalfedgeRange<Mesh, Halfedge_around_face_iterator>(HalfedgeIterator<Mesh, Halfedge_around_face_iterator>(this, range.begin(), range.end()),
                                                        HalfedgeIterator<Mesh, Halfedge_around_face_iterator>(this, range.end(), range.end(), 3));
}

Point Mesh::vertex_point(Vertex_handle vertex) const {
    return m_surface_mesh.point(vertex);
}
double Mesh::vertex_distance(const Point &center, Vertex_handle vertex) const {
    return CGAL::sqrt(CGAL::squared_distance(center, vertex_point(vertex)));
}
double Mesh::max_distance(const Point &center) const {
    const auto max_vertex = boost::max_element(CGAL::vertices(m_surface_mesh),
                                               [&](const auto &v1, const auto &v2) {
        return vertex_distance(center, v1) < vertex_distance(center, v2);
    });
    return vertex_distance(center, *max_vertex);
}

float Mesh::max_distance(float x, float y, float z) const {
    return static_cast<float>(max_distance(Point(x, y, z)));
}

Triangle Mesh::face_triangle(Face_handle face) const {
    const auto he = m_surface_mesh.halfedge(face);
    return Triangle(vertex_point(m_surface_mesh.target(he)),
                           vertex_point(m_surface_mesh.target(m_surface_mesh.next(he))),
                           vertex_point(m_surface_mesh.source(he)));
}
double Mesh::face_distance(const Point &center, Face_handle face) const {
    return CGAL::sqrt(CGAL::squared_distance(center, face_triangle(face)));
}

Vector Mesh::face_normal(Face_handle face) const
{
    return CGAL::Polygon_mesh_processing::compute_face_normal(face, m_surface_mesh);
}
double Mesh::min_distance(const Point &center) const {
    const auto min_face = boost::min_element(CGAL::faces(m_surface_mesh),
                                             [&](const auto &f1, const auto &f2) {
        return face_distance(center, f1) < face_distance(center, f2);
    });
    return face_distance(center, *min_face);
}

float Mesh::min_distance(float x, float y, float z) const {
    return static_cast<float>(min_distance(Point(x, y, z)));
}

double Mesh::surface_area() const {
    return m_surface_area;
}

double Mesh::volume() const {
    return m_volume;
}

double Mesh::a() const {
    return m_a;
}

double Mesh::b() const {
    return m_b;
}

double Mesh::c() const {
    return m_c;
}

double Mesh::projected_circumference() const
{
    return m_projected_circumference;
}

double Mesh::projected_area() const
{
    return m_projected_area;
}

size_t Mesh::number_of_vertices() const {
    return m_surface_mesh.number_of_vertices();
}

size_t Mesh::number_of_halfedges() const {
    return m_surface_mesh.number_of_halfedges();
}

std::size_t Mesh::number_of_faces() const {
    return m_surface_mesh.number_of_faces();
}

double Mesh::xmin() const {
    return m_bounding_box.xmin();
}
double Mesh::xmax() const {
    return m_bounding_box.xmax();
}
double Mesh::ymin() const {
    return m_bounding_box.ymin();
}
double Mesh::ymax() const {
    return m_bounding_box.ymax();
}
double Mesh::zmin() const {
    return m_bounding_box.zmin();
}
double Mesh::zmax() const {
    return m_bounding_box.zmax();
}
const std::vector<double>& Mesh::ratios() const {
  return m_ratios;
}
