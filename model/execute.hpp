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

#ifndef MODEL_EXECUTE_HPP
#define MODEL_EXECUTE_HPP

#include <CGAL/centroid.h>
#include <CGAL/convex_hull_3.h>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/filesystem/path.hpp>
#include <contours/intersect_faces.hpp>
#include <contours/intersect_halfedges.hpp>
#include <contours/mesh_properties.hpp>
#include <contours/stl_io.hpp>
#include <contours/util.hpp>
#include <contours/merge_equal_vertices.hpp>
#include <contours/discover_graph.hpp>
#include <contours/find_equilibra.hpp>
#include <contours/make_reeb.hpp>
#include <contours/encode_graph.hpp>
#include <contours/axes.hpp>
#include <string>
#include <vector>

#include "parameters.hpp"

void load_mesh(const std::string &filename,
	       Mesh &mesh,
	       const boost::filesystem::path &directory = boost::filesystem::path());

std::array<double, 13> mesh_properties(const Mesh &mesh);

template <typename Saver>
void execute(const std::string &filename,
	     const Mesh &mesh,
	     const double area,
	     const double volume,
	     const Parameters &center_spheres,
	     Saver &saver) {
  using namespace std;
  using namespace boost;
  using namespace boost::adaptors;

  for (const auto &center_sphere : center_spheres | indexed()) {
    const auto centers = center_sphere.value().value(volume);

    vector<vector<vector<pair<unsigned int, unsigned int>>>> results(center_sphere.value().next.size());
    for (const auto level_count : center_sphere.value().next | indexed()) {
      results[level_count.index()].resize(level_count.value().next.size());
      for (const auto area_ratio : level_count.value().next | indexed()) {
        results[level_count.index()][area_ratio.index()].resize(centers.size());
      }
    }

    for (const auto &center : centers | indexed()) {
      const auto min_distance =
	contours::min_distance(mesh, mesh.points(), center.value());
      const auto max_distance =
	contours::max_distance(mesh, mesh.points(), center.value()) + 0.0001;

      for (const auto &level_count : center_sphere.value().next | indexed()) {
        const auto step =
	  (max_distance - min_distance) / (level_count.value().value + 1);
	std::vector<std::map<int, std::pair<Point, Point>>> h_i(mesh.num_halfedges());
	auto halfedge_intersections = boost::make_iterator_property_map(h_i.begin(), CGAL::get(boost::halfedge_index, mesh));
        contours::intersect_halfedges(mesh, mesh.points(), center.value(), min_distance,
                                   step, halfedge_intersections);
        Graph graph;
        auto area_map = boost::get(&VertexProperty::area, graph);
        auto eq_map = boost::get(&VertexProperty::eq_edges, graph);
        auto edge_level = boost::get(&EdgeProperty::level, graph);
	auto visited_map = boost::get(&VertexProperty::visited, graph);
	auto area_inside_map = boost::get(&EdgeProperty::area_inside, graph);
	auto roots_inside_map = boost::get(&EdgeProperty::roots_inside, graph);
	auto reverse = make_reverse_graph(graph);
	auto area_outside_map = boost::get(&EdgeProperty::area_outside, reverse);
	auto roots_outside_map = boost::get(&EdgeProperty::roots_outside, reverse);
	auto vertex_level = boost::get(&VertexProperty::level, graph);
	auto vertex_label = boost::get(&VertexProperty::label, graph);
	auto arc_list = boost::get(&EdgeProperty::arcs, graph);
	auto vertex_id = boost::get(&VertexProperty::id, graph);
	std::vector<GraphVertex> stable_vertices, unstable_vertices;

	std::vector<std::map<int, GraphVertex>> f_h(mesh.num_halfedges()), t_h(mesh.num_halfedges());
	auto to_halfedge = boost::make_iterator_property_map(t_h.begin(), CGAL::get(boost::halfedge_index, mesh));
	auto from_halfedge = boost::make_iterator_property_map(f_h.begin(), CGAL::get(boost::halfedge_index, mesh));
	contours::intersect_faces(mesh, mesh.points(), halfedge_intersections,
                               to_halfedge, from_halfedge, center.value(), min_distance,
                               step, graph, area_map, eq_map, edge_level, arc_list);
        contours::merge_equal_vertices(graph, eq_map, area_map, visited_map, arc_list);
        contours::discover_graph(graph, area_map, area_inside_map, roots_inside_map, back_inserter(stable_vertices));
	contours::discover_graph(reverse, area_map, area_outside_map, roots_outside_map, back_inserter(unstable_vertices));
        for (const auto &area_ratio : level_count.value().next | indexed()) {
          vector<GraphEdge> stable_edges;
	  vector<ReverseEdge> unstable_edges;
          contours::find_equilibria(graph, stable_vertices, area_inside_map, roots_inside_map, back_inserter(stable_edges), area * area_ratio.value().value);
          contours::find_equilibria(reverse, unstable_vertices, area_outside_map, roots_outside_map, back_inserter(unstable_edges), area * area_ratio.value().value);
	  
	  vector<GraphEdge> underlying_unstable;
	  for (const auto &u : unstable_edges)
	    underlying_unstable.push_back(get(edge_underlying, reverse, u));
	  
	  saver.level_graph(filename, center_sphere.value().value, level_count.value().value, graph, stable_edges, underlying_unstable);
          results[level_count.index()][area_ratio.index()][center.index()] =
              make_pair(stable_edges.size(), unstable_edges.size());

	  // this part only makes sense when dealing with a single graph
	  if (boost::find(area_ratio.value().next, FIRST) != area_ratio.value().next.end() && center.index() == 0) {
	    for (const auto &vertex : boost::make_iterator_range(boost::vertices(graph)))
	      graph[vertex].visited = false;
	    contours::mark_inside(graph, stable_vertices, stable_edges, visited_map);
	    contours::mark_inside(reverse, unstable_vertices, unstable_edges, visited_map);

	    // make a copy
	    Graph reeb(graph);
	    auto reeb_visited_map = boost::get(&VertexProperty::visited, reeb);
	    auto reeb_edge_level = boost::get(&EdgeProperty::level, reeb);
	    auto reeb_vertex_level = boost::get(&VertexProperty::level, reeb);
	    contours::make_reeb(reeb, reeb_visited_map, reeb_edge_level, reeb_vertex_level);
	    for (const auto &vertex : boost::make_iterator_range(boost::vertices(reeb)) | indexed())
	      reeb[vertex.value()].id = vertex.index();
	    auto reeb_vertex_id = boost::get(&VertexProperty::id, reeb);
	    auto reeb_vertex_label = boost::get(&VertexProperty::label, reeb);
	    contours::reeb_encode(reeb, reeb_vertex_id, reeb_vertex_label);
	    saver.reeb(filename, center_sphere.value().value, level_count.value().value, area_ratio.value().value, FIRST, reeb, contours::encode(reeb, reeb_vertex_label));
	    if (contours::make_morse(reeb, reeb_vertex_level, reeb_vertex_label))
	      saver.morse(filename, center_sphere.value().value, level_count.value().value, area_ratio.value().value, FIRST, contours::encode(reeb, reeb_vertex_label));
	  }
        }
      }
    }

    for (const auto &level_count : center_sphere.value().next | indexed()) {
      for (const auto &area_ratio : level_count.value().next | indexed()) {
        for (const auto &aggregation : area_ratio.value().next) {
          const auto &r = results[level_count.index()][area_ratio.index()];
	  pair<float, float> su;
          switch (aggregation) {
          case FIRST:
            su = r[0];
            break;
          case AVERAGE:
	    su.first = 0.0f;
	    su.second = 0.0f;
	    for (const auto a : r) {
	      su.first += a.first;
	      su.second += a.second;
	    }
	    su.first /= centers.size();
	    su.second /= centers.size();
            break;
          case SMIN:
            su = *min_element(r);
            break;
          case SMAX:
            su = *max_element(r);
            break;
          case UMIN:
            su = *min_element(r, [](const auto &su1, const auto &su2) {
		return std::tie(su1.second, su1.first) < std::tie(su2.second, su2.first);
            });
            break;
          case UMAX:
            su = *max_element(r, [](const auto &su1, const auto &su2) {
		return std::tie(su1.second, su1.first) < std::tie(su2.second, su2.first);
            });
            break;
          }
	  saver.su(filename, center_sphere.value().value, level_count.value().value, area_ratio.value().value, aggregation, su);
        }
      }
    }
  }
}

#endif // MODEL_EXECUTE_HPP
