#ifndef MESH_HPP
#define MESH_HPP

#include <string>
#include <CGAL/Surface_mesh.h>
#include <boost/range/algorithm/min_element.hpp>
#include <boost/range/algorithm/max_element.hpp>
#include <CGAL/bounding_box.h>

#include "vertex.hpp"
#include "halfedge.hpp"
#include "face.hpp"

class Mesh {
  typedef CGAL::Surface_mesh<Point> surface_mesh;
  typedef surface_mesh::Vertex_index Vertex_handle;
  typedef surface_mesh::Vertex_iterator Vertex_iterator;
  typedef surface_mesh::Vertex_around_face_iterator Vertex_around_face_iterator;
  
  typedef surface_mesh::Halfedge_index Halfedge_handle;
  typedef surface_mesh::Halfedge_iterator Halfedge_iterator;
  typedef surface_mesh::Halfedge_around_face_iterator Halfedge_around_face_iterator;
  
  typedef surface_mesh::Face_index Face_handle;
  typedef surface_mesh::Face_iterator Face_iterator;
  
  friend class Vertex_base<Mesh>;
  friend class Halfedge_base<Mesh>;
  friend class Face_base<Mesh>;
  friend struct Arc_base<Mesh>; // MSVC complains about friend class if it's a struct

  // TODO: doesn't have move c'tor, falls back on copy c'tor
  surface_mesh m_surface_mesh;

  // primary measurements
  Cuboid m_bounding_box = Cuboid(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
  double m_surface_area = 0.0;
  double m_volume = 0.0;
  double m_a = 0.0;
  double m_b = 0.0;
  double m_c = 0.0;
  double m_projected_circumference = 0.0;
  double m_projected_area = 0.0;

  // derived measurements
  std::vector<double> m_ratios;

  void initialize(std::vector<Point> &points);
public:
  enum format {
    STL,
    OFF
  };
  /*!
   * Create mesh from a range of points.
   */
  template <typename InputIterator>
  Mesh(InputIterator first, InputIterator last) {
    std::vector<Point> points;
    points.reserve(std::distance(first, last));
    std::copy(first,
	      last,
	      std::back_inserter(points));
    initialize(points);
  }
  /*!
   * Auxiliary constructor that loads a mesh from an input stream.
   */
  explicit Mesh(std::istream &stream, const format form = STL);

  /*!
   * Calculate all the measurements
   */
  void measure();

  FaceRange<Mesh, Face_iterator> faces() const;
  HalfedgeRange<Mesh, Halfedge_iterator> halfedges() const;
  VertexRange<Mesh, Vertex_iterator> vertices() const;
  
  double min_distance(const Point &center) const;
  float min_distance(float x, float y, float z) const;
  double max_distance(const Point &center) const;
  float max_distance(float x, float y, float z) const;
  
  std::size_t number_of_vertices() const;
  std::size_t number_of_halfedges() const;
  std::size_t number_of_faces() const;
  
  double xmin() const;
  double xmax() const;
  double ymin() const;
  double ymax() const;
  double zmin() const;
  double zmax() const;
  double surface_area() const;
  double volume() const;
  double a() const;
  double b() const;
  double c() const;
  double projected_circumference() const;
  double projected_area() const;
  const std::vector<double>& ratios() const;
private:
  HalfedgeRange<Mesh, Halfedge_around_face_iterator> halfedges_around_face(Face_handle face) const;
  VertexRange<Mesh, Vertex_around_face_iterator> vertices_around_face(Face_handle face) const;

  Vertex_base<Mesh> source(Halfedge_handle halfedge) const;
  Vertex_base<Mesh> target(Halfedge_handle halfedge) const;
  Halfedge_base<Mesh> next(Halfedge_handle halfedge) const;
  Halfedge_base<Mesh> opposite(Halfedge_handle halfedge) const;
  Halfedge_base<Mesh> halfedge(Vertex_handle source, Vertex_handle target) const;
  
  Point vertex_point(Vertex_handle vertex) const;
  double vertex_distance(const Point &center, Vertex_handle vertex) const;
  
  Triangle face_triangle(Face_handle face) const;
  double face_distance(const Point &center, Face_handle face) const;
  Vector face_normal(Face_handle face) const;
};

typedef Vertex_base<Mesh> Vertex;
typedef Halfedge_base<Mesh> Halfedge;
typedef Face_base<Mesh> Face;
typedef Arc_base<Mesh> Arc_;

#endif // MESH_HPP
