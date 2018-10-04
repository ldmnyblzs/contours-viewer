#ifndef FACE_HPP
#define FACE_HPP

#include <array>
#include <vector>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/algorithm/count_if.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/counting_range.hpp>
#include <CGAL/iterator.h>

#include "arc.hpp"

template <typename Mesh>
class Face_base {
    typedef typename Mesh::Face_handle handle_type;
    const Mesh *mesh;
    handle_type face;
    mutable boost::optional<Triangle> cached_triangle;
public:
  Face_base(const Mesh *mesh, handle_type face) :
      mesh(mesh),
      face(face) {
  }
  Face_base(const Face_base &) = default;
  Face_base(Face_base &&) = default;
  Face_base &operator=(const Face_base &) = default;
  Face_base &operator=(Face_base &&) = default;
  ~Face_base() = default;

  auto intersect(const Sphere &sphere_in, int sphere_index) const {
      typedef std::pair<Circular_arc_point, unsigned int> Intersection;

      const auto tri = to_spherical(triangle());
      const auto sphere = to_spherical(sphere_in);
      std::vector<Arc_base<Mesh> > arcs;

      const auto vertices = vertices_around();
      const auto halfedges = halfedges_around();

      if (const auto ps = CGAL::intersection(tri.supporting_plane(), sphere)) {
          if(const auto *circle = boost::get<SCircle>(&*ps)) {
              if (boost::count_if(boost::counting_range(0, 3),[&](const auto i) {
                                  return sphere.has_on_unbounded_side(tri.vertex(i));
          }) > 0) {
                  boost::optional<Circular_arc_point> first = boost::none;
                  boost::optional<Circular_arc_point> previous = boost::none;
                  Halfedge_base<Mesh> previous_halfedge;
                  Halfedge_base<Mesh> first_halfedge;
                  std::vector<Vertex_base<Mesh> > passed_by;

                  for (std::uint8_t i = 0; i < 3; ++i) {
                      const std::uint8_t prev_i = (i + 2) % 3;
                      const auto points = [&](){
                          const auto source = tri.vertex(prev_i);
                          std::vector<Intersection> temp;
                          CGAL::intersection(Line_arc(source, tri.vertex(i)),
                                             sphere,
                                             CGAL::dispatch_or_drop_output<Intersection>(std::back_inserter(temp)));
                          if (temp.size() == 2 && CGAL::has_larger_distance_to_point(source, to_point(std::get<0>(temp[0])), to_point(std::get<0>(temp[1]))))
                              std::swap(temp[0], temp[1]);
                          return temp;
                      }();

                      if (points.size() > 0) {
                          const auto &point = std::get<0>(points[0]);
                          if (!first) {
                              first = point;
                              first_halfedge = *(halfedges.begin() + i);
                          }
                          if (previous && sphere.has_on_unbounded_side(tri.vertex(prev_i))) {
			    arcs.emplace_back(passed_by, *circle, sphere_index, *previous, previous_halfedge, point, *(halfedges.begin() + i));
                          }
                          passed_by.clear();
                          previous = (points.size() == 1) ? point : std::get<0>(points[1]);
                          previous_halfedge = *(halfedges.begin() + i);
                      }
                      passed_by.push_back(*(vertices.begin() + i));
                  }
                  if (first != previous) {
                      if (sphere.has_on_unbounded_side(tri.vertex(2))) {
                          for (std::uint8_t i = 0; *(halfedges.begin() + i) != first_halfedge; ++i)
                              passed_by.push_back(*(vertices.begin() + i));
                          arcs.emplace_back(passed_by, *circle, sphere_index, *previous, previous_halfedge, *first, first_halfedge);
                      }
                  } else if (tri.has_on(circle->center())) {
                      arcs.emplace_back(vertices_around(), *circle, sphere_index);
                  }
              }
          }
      }
      return arcs;
  }

  auto null_arc() const {
      return Arc_base<Mesh>(vertices_around());
  }
  auto handle() const {
      return face;
  }
  auto halfedges_around() const {
      return mesh->halfedges_around_face(face);
  }
  auto vertices_around() const {
      return mesh->vertices_around_face(face);
  }
  auto min_distance(const Point &center) const {
      return mesh->face_distance(center, face);
  }
  auto max_distance(const Point &center) const {
      auto vertices = vertices_around();
      const auto max_vertex = boost::max_element(vertices,
                                 [&](const auto &v1, const auto &v2) {
          return v1.distance(center) < v2.distance(center);
      });
      return max_vertex->distance(center);
  }
  auto triangle() const {
      if (!cached_triangle)
          cached_triangle = mesh->face_triangle(face);
      return *cached_triangle;
  }
  auto normal() const {
      return mesh->face_normal(face);
  }
};

template <typename Mesh, typename Iterator>
class FaceIterator :
        public boost::iterator_facade<FaceIterator<Mesh, Iterator>, Face_base<Mesh> const, boost::random_access_traversal_tag, Face_base<Mesh>> {
    friend class boost::iterator_core_access;

    Face_base<Mesh> dereference() const {
        return Face_base<Mesh>(mesh, *adaptor[index]);
    }
    bool equal(const FaceIterator<Mesh, Iterator> &other) const {
        return index == other.index;
    }
    void increment() {
        ++index;
    }
    void decrement() {
        --index;
    }
    void advance(std::size_t a) {
        index += a;
    }
    std::ptrdiff_t distance_to(const FaceIterator<Mesh, Iterator> &other) const {
        return other.index - index;
    }

    const Mesh *mesh;
    CGAL::Random_access_adaptor<Iterator> adaptor;
    std::size_t index;
public:
    FaceIterator() = default;
    FaceIterator(const Mesh *mesh, const Iterator &begin, const Iterator &end, std::size_t index = 0) :
        mesh(mesh),
        adaptor(begin, end),
        index(index) {
    }
    FaceIterator(const FaceIterator &) = default;
    FaceIterator(FaceIterator &&) = default;
    FaceIterator &operator=(const FaceIterator &) = default;
    FaceIterator &operator=(FaceIterator &&) = default;
    ~FaceIterator() = default;
};

template <typename Mesh, typename Iterator>
using FaceRange = tbb::blocked_range<FaceIterator<Mesh, Iterator> >;

#endif // FACE_HPP
