/*
  Copyright 2019 Balázs Ludmány

  This file is part of contours-viewer.

  contours-viewer is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  contours-viewer is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with contours-viewer.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "primitives.hpp"
#include "execute.hpp"
#include <CGAL/bounding_box.h>

void load_mesh(const std::string &filename,
	       Mesh &mesh,
	       const boost::filesystem::path &directory) {
  const boost::iostreams::mapped_file_source file((directory / filename).string());
  contours::read_STL(file.begin(), file.end(), mesh, mesh.points());

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
  const auto abc = contours::axes(mesh.points());
  const auto [circ, area] = contours::projected_properties(mesh.points(), abc[0], abc[1], abc[2]);
  const auto bounding_box = CGAL::bounding_box(mesh.points().begin(), mesh.points().end());
  return {contours::surface_area(mesh, mesh.points()),
      contours::volume(mesh, mesh.points(), Point(CGAL::ORIGIN)),
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
