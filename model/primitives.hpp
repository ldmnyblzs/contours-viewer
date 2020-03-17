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

#ifndef MODEL_PRIMITIVES_HPP
#define MODEL_PRIMITIVES_HPP 1

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/container/flat_set.hpp>

using Kernel = CGAL::Simple_cartesian<double>;
using Point = Kernel::Point_3;
using Vector = Kernel::Vector_3;
using Mesh = CGAL::Surface_mesh<Point>;
using Halfedge = Mesh::Halfedge_index;

using Transform = CGAL::Aff_transformation_3<Kernel>;

struct VertexProperty;
struct EdgeProperty;

using Graph = boost::adjacency_list<boost::vecS, boost::listS, boost::bidirectionalS,
				    VertexProperty, EdgeProperty>;
using Reverse = boost::reverse_graph<Graph>;

using GraphVertex = boost::graph_traits<Graph>::vertex_descriptor;
using GraphEdge = boost::graph_traits<Graph>::edge_descriptor;
using ReverseVertex = boost::graph_traits<Reverse>::vertex_descriptor;
using ReverseEdge = boost::graph_traits<Reverse>::edge_descriptor;

struct VertexProperty {
  double area = 0.0;
  boost::container::flat_set<GraphVertex> eq_edges;
  bool visited = false;
  int id;
  float level;
  int label;
};

struct AArc {
  Point center;
  Point source;
  Point target;
  Vector normal;
  AArc(Point center, Point source, Point target, Vector normal) :
    center(std::move(center)),
    source(std::move(source)),
    target(std::move(target)),
    normal(std::move(normal)) {
  }
};

struct EdgeProperty {
  double area_inside = 0.0, area_outside = 0.0;
  boost::container::flat_set<GraphVertex> roots_inside, roots_outside;
  int level = 0;
  std::list<AArc> arcs;
};

#endif // MODEL_PRIMITIVES_HPP
