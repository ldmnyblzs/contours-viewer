#ifndef ARC_HPP
#define ARC_HPP

#include <vector>
#include <algorithm>

#include <boost/optional.hpp>
#include <boost/math/constants/constants.hpp>
#include <CGAL/Exact_spherical_kernel_3.h>
#include <CGAL/Cartesian_converter.h>

#include "vertex.hpp"
#include "halfedge.hpp"

typedef CGAL::Exact_spherical_kernel_3 SKernel;
typedef SKernel::Circle_3 SCircle;
typedef SKernel::Circular_arc_point_3 Circular_arc_point;
typedef SKernel::Circular_arc_3 Circular_arc;
typedef SKernel::Line_arc_3 Line_arc;

static const CGAL::Cartesian_converter<SKernel, Kernel> to_simple;
static const CGAL::Cartesian_converter<Kernel, SKernel> to_spherical;

static const SKernel::Compute_approximate_angle_3 approximate_angle;

inline SKernel::Point_3 to_point(const Circular_arc_point &original) noexcept {
  return {CGAL::to_double(original.x()),
      CGAL::to_double(original.y()),
      CGAL::to_double(original.z())};
}

template <typename Mesh>
struct Endpoint {
  Circular_arc_point arc_point;
  Halfedge_base<Mesh> halfedge;

  mutable boost::optional<Point> m_point_cache;
  auto point() const {
	if (!m_point_cache)
		m_point_cache = to_simple(to_point(arc_point));
	return *m_point_cache;
  }
};

template <typename Mesh>
struct Arc_base {
  std::vector<Vertex_base<Mesh> > passed_by;
  SCircle spherical_circle;
  int sphere_index;
  boost::optional<Endpoint<Mesh> > source = boost::none;
  boost::optional<Endpoint<Mesh> > target = boost::none;

  template <typename VertexRange>
  explicit Arc_base(const VertexRange &passed_by) :
    passed_by(std::cbegin(passed_by), std::cend(passed_by)) {
  }
  template <typename VertexRange>
  Arc_base(const VertexRange &passed_by,
	   const SCircle &circle,
	   int sphere_index) :
    passed_by(std::cbegin(passed_by), std::cend(passed_by)),
    spherical_circle(circle),
    sphere_index(sphere_index) {
  }
  template <typename VertexRange>
  Arc_base(const VertexRange &passed_by,
       const SCircle &circle,
       int sphere_index,
       const Circular_arc_point &source_point,
       const Halfedge_base<Mesh> &source_halfedge,
       const Circular_arc_point &target_point,
       const Halfedge_base<Mesh> &target_halfedge) :
      passed_by(std::cbegin(passed_by), std::cend(passed_by)),
      spherical_circle(circle),
      sphere_index(sphere_index),
      source(Endpoint<Mesh>{source_point, source_halfedge}),
      target(Endpoint<Mesh>{target_point, target_halfedge}) {
  }
  Arc_base(const Arc_base &) = default;
  Arc_base(Arc_base&&) = default;
  Arc_base &operator=(const Arc_base &) = default;
  Arc_base &operator=(Arc_base &&) = default;
  ~Arc_base() = default;

  bool beyond(Vertex_base<Mesh> vertex) const {
    return std::find(passed_by.begin(), passed_by.end(), vertex) != passed_by.end();
  }
  mutable boost::optional<double> m_squared_radius_cache = boost::none;
  double squared_radius() const {
	  if (!m_squared_radius_cache)
		  m_squared_radius_cache = CGAL::to_double(spherical_circle.squared_radius());
	  return *m_squared_radius_cache;
  }
  mutable boost::optional<double> m_radius_cache = boost::none;
  double radius() const {
	  if (!m_radius_cache)
		  m_radius_cache = std::sqrt(squared_radius());
	  return *m_radius_cache;
  }
  mutable boost::optional<double> m_angle_cache = boost::none;
  double angle() const {
	using namespace boost::math::double_constants;
	
	if (!m_angle_cache) {
		const auto normal = spherical_circle.supporting_plane().orthogonal_vector();
		if (source)
			m_angle_cache = approximate_angle((std::tie(normal.x(), normal.y(), normal.z()) > std::make_tuple(0.0, 0.0, 0.0)) ?
				Circular_arc(spherical_circle, source->arc_point, target->arc_point) :
				Circular_arc(spherical_circle, target->arc_point, source->arc_point));
		else if (squared_radius() > 0)
			m_angle_cache = 2.0 * pi;
		else
			m_angle_cache = 0;
	}
	return *m_angle_cache;
  }
  mutable boost::optional<double> m_area_cache = boost::none;
  double approximate_area() const {
	  if (!m_area_cache) {
		const auto a = angle();
		m_area_cache = squared_radius() * 0.5 * (a - std::sin(a));
	  }
	  return *m_area_cache;
  }
  mutable boost::optional<double> m_length_cache = boost::none;
  double approximate_length() const {
	  if (!m_length_cache)
		m_length_cache = radius() * angle();
	return *m_length_cache;
  }
  auto to_source() const {
	if (source)
		return source->point() - to_simple(spherical_circle.center());
	return Vector(0, 0, 0);
  }
};

#endif // ARC_HPP
