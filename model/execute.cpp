#include "primitives.hpp"
#include "execute.hpp"
#include <CGAL/bounding_box.h>

void load_mesh(const std::string &filename,
	       Mesh &mesh,
	       const boost::filesystem::path &directory) {
  const boost::iostreams::mapped_file_source file((directory / filename).string());
  shape::read_STL(file.begin(), file.end(), mesh, mesh.points());

  const auto origin = CGAL::centroid(mesh.points().begin(), mesh.points().end());
  std::transform(mesh.points().begin(),
		 mesh.points().end(),
		 mesh.points().begin(),
		 Transform(CGAL::TRANSLATION,
			   Vector(origin, Point(CGAL::ORIGIN))));
  
  CGAL::convex_hull_3(mesh.points().begin(), mesh.points().end(), mesh);
  mesh.collect_garbage();
}

std::array<double, 13> mesh_properties(const Mesh &mesh) {
  const auto abc = shape::axes(mesh.points());
  const auto [circ, area] = shape::projected_properties(mesh.points(), abc[0], abc[1], abc[2]);
  const auto bounding_box = CGAL::bounding_box(mesh.points().begin(), mesh.points().end());
  return {shape::surface_area(mesh, mesh.points()),
      shape::volume(mesh, mesh.points(), Point(CGAL::ORIGIN)),
      CGAL::sqrt(abc[0].squared_length()),
      CGAL::sqrt(abc[1].squared_length()),
      CGAL::sqrt(abc[2].squared_length()),
      circ,
      area,
      bounding_box.xmin(),
      bounding_box.xmax(),
      bounding_box.ymin(),
      bounding_box.ymax(),
      bounding_box.zmin(),
      bounding_box.zmax(),};
}
