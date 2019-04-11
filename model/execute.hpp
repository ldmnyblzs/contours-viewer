#ifndef MODEL_EXECUTE_HPP
#define MODEL_EXECUTE_HPP

#include <CGAL/centroid.h>
#include <CGAL/convex_hull_3.h>
#include <boost/filesystem/path.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/algorithm/count_if.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/graph/graphviz.hpp>
#include <shape/axes.hpp>
#include <shape/discover_graph.hpp>
#include <shape/encode_graph.hpp>
#include <shape/find_equilibra.hpp>
#include <shape/intersect_faces.hpp>
#include <shape/intersect_halfedges.hpp>
#include <shape/make_reeb.hpp>
#include <shape/merge_equal_vertices.hpp>
#include <shape/mesh_properties.hpp>
#include <shape/stl_io.hpp>
#include <shape/util.hpp>
#include <string>
#include <vector>
#include <variant>

#include "parameters.hpp"

void load_mesh(
    const std::string &filename, Mesh &mesh,
    const boost::filesystem::path &directory = boost::filesystem::path());

std::array<double, 13> mesh_properties(const Mesh &mesh);

template <typename Saver>
void execute(const std::string &filename, const Mesh &mesh, const double area,
             const double volume, const Parameters &center_spheres,
             Saver &saver) {
  using namespace std;
  using namespace boost;
  using namespace boost::adaptors;

  for (const auto &center_sphere : center_spheres | indexed()) {
    const auto centers = center_sphere.value().value(volume);

    vector<vector<vector<pair<unsigned int, unsigned int>>>> results(
        center_sphere.value().next.size());
    for (const auto level_count : center_sphere.value().next | indexed()) {
      results[level_count.index()].resize(level_count.value().next.size());
      for (const auto area_ratio : level_count.value().next | indexed()) {
        results[level_count.index()][area_ratio.index()].resize(centers.size());
      }
    }

    for (const auto &center : centers | indexed()) {
      const auto min_distance =
          shape::min_distance(mesh, mesh.points(), center.value());
      const auto max_distance =
          shape::max_distance(mesh, mesh.points(), center.value()) + 0.0001;

      for (const auto &level_count : center_sphere.value().next | indexed()) {
        const auto step =
            (max_distance - min_distance) / (level_count.value().value + 1);
        std::vector<std::map<int, std::pair<Point, Point>>> h_i(
            mesh.num_halfedges());
        auto halfedge_intersections = boost::make_iterator_property_map(
            h_i.begin(), CGAL::get(boost::halfedge_index, mesh));
        shape::intersect_halfedges(mesh, mesh.points(), center.value(),
                                   min_distance, step, halfedge_intersections);
        Graph graph;
        auto area_map = boost::get(&VertexProperty::area, graph);
        auto eq_map = boost::get(&VertexProperty::eq_edges, graph);
        auto edge_level = boost::get(&EdgeProperty::level, graph);
        auto visited_map = boost::get(&VertexProperty::visited, graph);
        auto area_inside_map = boost::get(&EdgeProperty::area_inside, graph);
        auto roots_inside_map = boost::get(&EdgeProperty::roots_inside, graph);
        auto reverse = make_reverse_graph(graph);
        auto area_outside_map =
            boost::get(&EdgeProperty::area_outside, reverse);
        auto roots_outside_map =
            boost::get(&EdgeProperty::roots_outside, reverse);
        auto vertex_level = boost::get(&VertexProperty::level, graph);
        auto vertex_label = boost::get(&VertexProperty::label, graph);
        auto arc_list = boost::get(&EdgeProperty::arcs, graph);
        auto vertex_id = boost::get(&VertexProperty::id, graph);
        std::vector<GraphVertex> stable_vertices, unstable_vertices;

        std::vector<std::map<int, GraphVertex>> f_h(mesh.num_halfedges()),
            t_h(mesh.num_halfedges());
        auto to_halfedge = boost::make_iterator_property_map(
            t_h.begin(), CGAL::get(boost::halfedge_index, mesh));
        auto from_halfedge = boost::make_iterator_property_map(
            f_h.begin(), CGAL::get(boost::halfedge_index, mesh));
        std::vector<AArc> arcs;
        shape::intersect_faces(mesh, mesh.points(), halfedge_intersections,
                               to_halfedge, from_halfedge, center.value(),
                               min_distance, step, graph, area_map, eq_map,
                               edge_level, arc_list, arcs);

        for (const auto &edge : boost::make_iterator_range(boost::edges(graph))) {
	  for (const auto e : boost::make_iterator_range(boost::out_edges(boost::target(edge, graph), graph)))
	    arcs[graph[edge].arcs.front()].prev.push_back(graph[e].arcs.front());
	  for (const auto e : boost::make_iterator_range(boost::in_edges(boost::source(edge, graph), graph)))
	    arcs[graph[edge].arcs.front()].next.push_back(graph[e].arcs.front());

	  //for (const auto &eq : graph[boost::source(edge, graph)].eq_edges) {
	  //  for (const auto e : boost::make_iterator_range(boost::in_edges(eq, graph)))
	  //    arcs[graph[edge].arcs.front()].next.push_back(graph[e].arcs.front());
	  //}

	  for (const auto &eq : graph[boost::target(edge, graph)].eq_edges) {
	    //for (const auto e : boost::make_iterator_range(boost::out_edges(eq, graph)))
	    //  arcs[graph[edge].arcs.front()].prev.push_back(graph[e].arcs.front());
	    for (const auto eq_edge : boost::make_iterator_range(boost::in_edges(eq, graph))) {
	      if (arcs[graph[edge].arcs.front()].target == arcs[graph[eq_edge].arcs.front()].source)
		arcs[graph[edge].arcs.front()].forward = graph[eq_edge].arcs.front();
	    }
	  }
        }

        shape::merge_equal_vertices(graph, eq_map, area_map, visited_map,
                                    arc_list);
        shape::discover_graph(graph, area_map, area_inside_map,
                              roots_inside_map, back_inserter(stable_vertices));
        shape::discover_graph(reverse, area_map, area_outside_map,
                              roots_outside_map,
                              back_inserter(unstable_vertices));
        for (const auto &area_ratio : level_count.value().next | indexed()) {
          vector<GraphEdge> stable_edges;
          vector<ReverseEdge> unstable_edges;
          shape::find_equilibria(graph, stable_vertices, area_inside_map,
                                 roots_inside_map, back_inserter(stable_edges),
                                 area * area_ratio.value().value);
          shape::find_equilibria(
              reverse, unstable_vertices, area_outside_map, roots_outside_map,
              back_inserter(unstable_edges), area * area_ratio.value().value);

          vector<GraphEdge> underlying_unstable;
          for (const auto &u : unstable_edges)
            underlying_unstable.push_back(get(edge_underlying, reverse, u));

	  std::function<void(GraphEdge)> mark_reachable_stable = [&](GraphEdge edge) {
	    graph[edge].reachable_stable = true;
	    for (const auto out : boost::make_iterator_range(boost::out_edges(boost::target(edge, graph), graph)))
	      mark_reachable_stable(out);
	  };
	  for (const auto &stable : stable_edges) {
	    mark_reachable_stable(stable);
	  }

	  std::function<void(GraphEdge)> mark_reachable_unstable = [&](GraphEdge edge) {
	    graph[edge].reachable_unstable = true;
	    for (const auto in : boost::make_iterator_range(boost::in_edges(boost::source(edge, graph), graph)))
	      mark_reachable_unstable(in);
	  };
	  for (const auto &unstable : underlying_unstable) {
	    mark_reachable_unstable(unstable);
	  }

	  std::function<int(int, GraphEdge)> stable_count = [&](int init, GraphEdge edge) -> int {
	    return boost::count_if(boost::in_edges(boost::source(edge, graph), graph),
				   [&](GraphEdge edge) {
				     return graph[edge].reachable_stable;
				   });
	  };
	  std::function<int(int, GraphEdge)> unstable_count = [&](int init, GraphEdge edge) -> int {
	    return boost::count_if(boost::out_edges(boost::target(edge, graph), graph),
				   [&](GraphEdge edge) {
				     return graph[edge].reachable_unstable;
				   });
	  };

	  std::function<void(const GraphEdge, const int)> set_stable = [&](const GraphEdge edge, const int stable) {
	    for (const auto &arc_index : graph[edge].arcs)
	      arcs[arc_index].stable = stable;
	    for (const auto in : boost::make_iterator_range(boost::in_edges(boost::source(edge, graph), graph)))
	      set_stable(in, stable);
	  };

	  std::deque<GraphEdge> start;
	  for (int i = 0; i < stable_edges.size(); ++i) {
	    GraphEdge last_edge, next_edge = stable_edges[i];
	    while (boost::out_degree(boost::target(next_edge, graph), graph) != 0 &&
		   stable_count(0, next_edge) < 2) {
	      last_edge = next_edge;
	      next_edge = *boost::begin(boost::out_edges(boost::target(last_edge, graph), graph));
	    }
	    if (boost::out_degree(boost::target(last_edge, graph), graph) != 0) {
	      start.push_back(next_edge);
	      graph[last_edge].visited = true;
	      
	      set_stable(last_edge, i + 1);
	    }
	  }
	  
	  while (!start.empty()) {
	    if (!graph[start.front()].visited) {
	      int reachable = 0, visited = 0;
	      for (const auto edge : boost::make_iterator_range(boost::in_edges(boost::source(start.front(), graph), graph))) {
		if (graph[edge].visited)
		  visited++;
		if (graph[edge].reachable_stable)
		  reachable++;
	      }
	      if (visited == reachable) {
		graph[start.front()].visited = true;
		for (const auto arc_index : graph[start.front()].arcs) {
		  int next = -1;
		  if (!arcs[arc_index].next.empty() && arcs[arcs[arc_index].next[0]].stable != 0)
		    next = arcs[arcs[arc_index].next[0]].stable;
		  else {
		    for (const auto p : arcs[arc_index].next) {
		      if (next == -1 && arcs[p].stable != 0)
			next = arcs[p].stable;
		      else if (next != -1 && arcs[p].stable != 0 && arcs[p].stable != next)
			next = 0;
		    }
		    if (next == -1)
		      next = 0;
		  }
		  arcs[arc_index].stable = next;
		}
		for (const auto e : boost::make_iterator_range(boost::out_edges(boost::target(start.front(), graph), graph)))
		  start.push_back(e);
		int s = 0;
		std::size_t length = 1;
		bool first_init = true;
		std::size_t fwd = graph[start.front()].arcs.front();
		for (auto a = arcs[graph[start.front()].arcs.front()].forward; a != graph[start.front()].arcs.front(); a = arcs[a].forward) {
		  ++length;
		  if (first_init && arcs[a].stable != 0) {
		    fwd = a;
		    first_init = false;
		  }
		}
		std::vector<std::size_t> to_set;
	        for (std::size_t i = 0; i <= length; ++i) {
		  if (arcs[fwd].stable == 0)
		    to_set.push_back(fwd);
		  else if (arcs[fwd].stable == s) {
		    for (const auto set : to_set)
		      arcs[set].stable = s;
		    to_set.clear();
		  } else {
		    if (s != 0) {
		      double min_distance = 1000000.0;
		      if (boost::in_degree(boost::source(start.front(), graph), graph) > 1) {
			//graph[start.front()].stable_changes.emplace(std::make_pair(s, arcs[fwd].stable), &arcs[fwd].unstable);
			const auto it = graph[start.front()].parts.find(std::make_pair(arcs[fwd].stable, s));
			if (it != graph[start.front()].parts.end()) {
			  if (!to_set.empty()) {
			    if (std::holds_alternative<std::vector<std::size_t> >(it->second)) {
			      std::size_t min_a1 = 0, min_a2 = 0;
			      for (const auto &a1 : to_set) {
				for (const auto &a2 : std::get<std::vector<std::size_t> >(it->second)) {
				  const auto sd = CGAL::squared_distance(arcs[a1].source, arcs[a2].source);
				  if (sd < min_distance) {
				    min_distance = sd;
				    min_a1 = a1;
				    min_a2 = a2;
				  }
				}
			      }
			      bool before = true;
			      for (const auto a1 : to_set) {
				if (a1 == min_a1)
				  before = false;
				arcs[a1].stable = before ? s : arcs[fwd].stable;
			      }
			      before = true;
			      for (const auto a2 : std::get<std::vector<std::size_t> >(it->second)) {
				if (a2 == min_a2)
				  before = false;
				arcs[a2].stable = before ? arcs[fwd].stable : s;
			      }
			      it->second = min_a2;
			      graph[start.front()].parts.emplace(std::make_pair(s, arcs[fwd].stable), min_a1);
			    } else {
			      std::size_t min_a1 = 0;
			      for (const auto &a1 : to_set) {
				const auto sd = CGAL::squared_distance(arcs[a1].source, arcs[std::get<std::size_t>(it->second)].source);
				if (sd < min_distance) {
				  min_distance = sd;
				  min_a1 = a1;
				}
			      }
			      bool before = true;
			      for (const auto a1 : to_set) {
				if (a1 == min_a1)
				  before = false;
				arcs[a1].stable = before ? s : arcs[fwd].stable;
			      }
			      graph[start.front()].parts.emplace(std::make_pair(s, arcs[fwd].stable), min_a1);
			    }
			  } else {
			    if (std::holds_alternative<std::vector<std::size_t> >(it->second)) {
			      std::size_t min_a2 = 0;
			      for (const auto &a2 : std::get<std::vector<std::size_t> >(it->second)) {
				const auto sd = CGAL::squared_distance(arcs[fwd].source, arcs[a2].source);
				if (sd < min_distance) {
				  min_distance = sd;
				  min_a2 = a2;
				}
			      }
			      bool before = true;
			      for (const auto a2 : std::get<std::vector<std::size_t> >(it->second)) {
				if (a2 == min_a2)
				  before = false;
				arcs[a2].stable = before ? arcs[fwd].stable : s;
			      }
			      it->second = min_a2;
			    }
			    graph[start.front()].parts.emplace(std::make_pair(s, arcs[fwd].stable), fwd);
			  }
			} else {
			  if (to_set.empty())
			    graph[start.front()].parts.emplace(std::make_pair(s, arcs[fwd].stable), fwd);
			  else
			    graph[start.front()].parts.emplace(std::make_pair(s, arcs[fwd].stable), to_set);
			}
		      } else if (boost::out_degree(boost::source(start.front(), graph), graph) > 1) {
			const auto neighbor = [&]{
			  for (const auto out : boost::make_iterator_range(boost::out_edges(boost::source(start.front(), graph), graph)))
			    if (out != start.front())
			      return out;
			}();
		      
			if (graph[neighbor].visited) {
			  std::size_t min_prev = 0, min_a1 = 0, min_a2 = 0;
			  const auto prev = *boost::begin(boost::in_edges(boost::source(start.front(), graph), graph));
			  const auto it_prev = graph[prev].parts.find(std::make_pair(s, arcs[fwd].stable));
			  if (it_prev != graph[*boost::begin(boost::in_edges(boost::source(start.front(), graph), graph))].parts.end()) {
			    for (const auto &a1 : to_set) {
			      const auto sd = CGAL::squared_distance(arcs[a1].source, arcs[std::get<std::size_t>(it_prev->second)].source);
			      if (sd < min_distance) {
				min_distance = sd;
				min_prev = a1;
			      }
			    }
			  }
			  bool neighbor_closer = false;
			  const auto it_neig = graph[neighbor].parts.find(std::make_pair(arcs[fwd].stable, s));
			  if (it_neig != graph[neighbor].parts.end()) {
			    if (std::holds_alternative<std::size_t>(it_neig->second)) {
			      min_a2 = std::get<std::size_t>(it_neig->second);
			      for (const auto &a1 : to_set) {
				const auto sd = CGAL::squared_distance(arcs[a1].source, arcs[std::get<std::size_t>(it_neig->second)].source);
				if (sd < min_distance) {
				  min_distance = sd;
				  min_a1 = a1;
				  neighbor_closer = true;
				}
			      }
			    } else {
			      for (const auto &a1 : to_set) {
				for (const auto &a2 : std::get<std::vector<std::size_t> >(it_neig->second)) {
				  const auto sd = CGAL::squared_distance(arcs[a1].source, arcs[a2].source);
				  if (sd < min_distance) {
				    min_distance = sd;
				    min_a1 = a1;
				    min_a2 = a2;
				    neighbor_closer = true;
				  }
				}
			      }
			    }
			  }

			  if (min_distance < 100000.0) {
			    if (neighbor_closer) {
			      bool before = true;
			      for (const auto a1 : to_set) {
				if (a1 == min_a1)
				  before = false;
				arcs[a1].stable = before ? s : arcs[fwd].stable;
			      }
			      if (std::holds_alternative<std::vector<std::size_t> >(it_neig->second)) {
				before = true;
				for (const auto a2 : std::get<std::vector<std::size_t> >(it_neig->second)) {
				  if (a2 == min_a2)
				    before = false;
				  arcs[a2].stable = before ? arcs[fwd].stable : s;
				}
				it_neig->second = min_a2;
			      }
			      graph[start.front()].parts.emplace(std::make_pair(s, arcs[fwd].stable), min_a1);
			    } else {
			      bool before = true;
			      for (const auto a1 : to_set) {
				if (a1 == min_prev)
				  before = false;
				arcs[a1].stable = before ? s : arcs[fwd].stable;
			      }
			      graph[start.front()].parts.emplace(std::make_pair(s, arcs[fwd].stable), min_prev);
			    }
			  }
			  for (auto &&n : graph[neighbor].parts) {
			    if (std::holds_alternative<std::vector<std::size_t> >(n.second)) {
			      const auto it = graph[prev].parts.find(n.first);
			      if (it != graph[prev].parts.end()) {
				min_distance = 100000.0;
				min_a1 = 0;
				for (const auto a1 : std::get<std::vector<std::size_t> >(n.second)) {
				  const auto sd = CGAL::squared_distance(arcs[a1].source, arcs[std::get<std::size_t>(it->second)].source);
				  if (sd < min_distance) {
				    min_distance = sd;
				    min_a1 = a1;
				  }
				}
				bool before = true;
				for (const auto a1 : std::get<std::vector<std::size_t> >(n.second)) {
				  if (a1 == min_a1)
				    before = false;
				  arcs[a1].stable = before ? n.first.first : n.first.second;
				}
				n.second = min_a1;
			      }
			    }
			  }
			} else {
			  if (to_set.empty())
			    graph[start.front()].parts.emplace(std::make_pair(s, arcs[fwd].stable), fwd);
			  else
			    graph[start.front()].parts.emplace(std::make_pair(s, arcs[fwd].stable), to_set);
			}
		      } else {
			const auto prev = *boost::begin(boost::in_edges(boost::source(start.front(), graph), graph));
			const auto it = graph[prev].parts.find(std::make_pair(s, arcs[fwd].stable));
			if (it != graph[prev].parts.end()) {
			  if (std::holds_alternative<std::vector<std::size_t> >(it->second)) {
			    std::cout << 's' << prev << std::endl;
			    /*for (const auto a : std::get<std::vector<std::size_t>>(it->second))
			      std::cout << a << ' ';
			      std::cout << std::endl;*/
			  } else {
			    std::size_t min_a1 = 0;
			    for (const auto &a1 : to_set) {
			      const auto sd = CGAL::squared_distance(arcs[a1].source, arcs[std::get<std::size_t>(it->second)].source);
			      if (sd < min_distance) {
				min_distance = sd;
				min_a1 = a1;
			      }
			    }
			    bool before = true;
			    for (const auto a1 : to_set) {
			      if (a1 == min_a1)
				before = false;
			      arcs[a1].stable = before ? s : arcs[fwd].stable;
			    }
			    graph[start.front()].parts.emplace(std::make_pair(s, arcs[fwd].stable), min_a1);
			  }
			} else {
			  std::cout << '-' << prev << '-' << boost::in_degree(boost::source(prev, graph), graph) << std::endl;
			  std::cout << s << ' ' << arcs[fwd].stable << std::endl;
			  for (const auto p : graph[prev].parts)
			    std::cout << p.first.first << ' ' << p.first.second << ' ' << p.second.index() << '+';
			  std::cout << std::endl;
			  for (const auto p : graph[*boost::begin(boost::in_edges(boost::source(prev, graph), graph))].parts)
			    std::cout << p.first.first << ' ' << p.first.second << ' ' << p.second.index() << '&';
			  std::cout << std::endl;
			}
		      } 
		    }
		    s = arcs[fwd].stable;
		    to_set.clear();
		  }
		  fwd = arcs[fwd].forward;
		}
		start.pop_front();
	      } else {
		start.pop_front();
	      }
	    } else {
	      start.pop_front();
	    }
	  }
	  for (const auto edge : boost::make_iterator_range(boost::edges(graph)))
	    graph[edge].visited = false;
	  
	  std::function<void(const GraphEdge, const int)> set_unstable = [&](const GraphEdge edge, const int unstable) {
	    for (const auto &arc_index : graph[edge].arcs)
	      arcs[arc_index].unstable = unstable;
	    for (const auto out : boost::make_iterator_range(boost::out_edges(boost::target(edge, graph), graph)))
	      set_unstable(out, unstable);
	  };
	  
	  for (int i = 0; i < underlying_unstable.size(); ++i) {
	    GraphEdge last_edge, next_edge = underlying_unstable[i];
	    while (boost::in_degree(boost::source(next_edge, graph), graph) != 0 &&
		   unstable_count(0, next_edge) < 2) {
	      last_edge = next_edge;
	      next_edge = *boost::begin(boost::in_edges(boost::source(last_edge, graph), graph));
	    }
	    if (boost::in_degree(boost::source(next_edge, graph), graph) != 0) {
	      start.push_back(next_edge);
	      graph[last_edge].visited = true;

	      set_unstable(last_edge, i + 1);
	    }
	  }
	  
	  while (!start.empty()) {
	    if (!graph[start.front()].visited) {
	      int reachable = 0, visited = 0;
	      for (const auto edge : boost::make_iterator_range(boost::out_edges(boost::target(start.front(), graph), graph))) {
		if (graph[edge].visited)
		  visited++;
		if (graph[edge].reachable_unstable)
		  reachable++;
	      }
	      if (visited == reachable) {
		graph[start.front()].visited = true;
		for (const auto arc_index : graph[start.front()].arcs) {
		  int prev = -1;
		  if (!arcs[arc_index].prev.empty() && arcs[arcs[arc_index].prev[0]].unstable != 0)
		    prev = arcs[arcs[arc_index].prev[0]].unstable;
		  else {
		    for (const auto p : arcs[arc_index].prev) {
		      if (prev == -1 && arcs[p].unstable != 0)
			prev = arcs[p].unstable;
		      else if (prev != -1 && arcs[p].unstable != 0 && arcs[p].unstable != prev)
			prev = 0;
		    }
		    if (prev == -1)
		      prev = 0;
		  }
		  arcs[arc_index].unstable = prev;
		}
		for (const auto e : boost::make_iterator_range(boost::in_edges(boost::source(start.front(), graph), graph)))
		  start.push_back(e);
		std::size_t fwd = arcs[graph[start.front()].arcs.front()].forward;
		int u = 0;
		std::vector<std::size_t> to_set;
		std::size_t first = graph[start.front()].arcs.front();
		bool first_init = true;
		while (fwd != first) {
		  if (arcs[fwd].unstable == 0)
		    to_set.push_back(fwd);
		  else if (arcs[fwd].unstable == u) {
		    for (const auto set : to_set)
		      arcs[set].unstable = u;
		    to_set.clear();
		  } else {
		    if (first_init) {
		      first = fwd;
		      first_init = false;
		    }
		    /*if (u != 0)
		      graph[start.front()].unstable_changes.emplace(std::make_pair(u, arcs[fwd].unstable), &arcs[fwd].stable);*/
		    u = arcs[fwd].unstable;
		    to_set.clear();
		  }
		  fwd = arcs[fwd].forward;
		}
		if (arcs[arcs[fwd].forward].unstable == u)
		  for (const auto set : to_set)
		    arcs[set].unstable = u;
		start.pop_front();
	      } else {
		//start.push_back(start.front());
		start.pop_front();
	      }
	    } else {
	      start.pop_front();
	    }
	  }

	  /*MS ms;
	  std::vector<MS_vertex> ms_stable;
	  std::vector<MS_vertex> ms_unstable;

	  for (int i = 0; i < stable_edges.size(); ++i) {
	    ms_stable.push_back(boost::add_vertex(ms));
	    ms[ms_stable.back()].type = 'S';
	  }
	  for (int i = 0; i < unstable_edges.size(); ++i) {
	    ms_unstable.push_back(boost::add_vertex(ms));
	    ms[ms_unstable.back()].type = 'U';
	  }
	  
	  for (const auto vertex : boost::make_iterator_range(boost::vertices(graph))) {
	    if (boost::in_degree(vertex, graph) > 1) {
	      GraphEdge out_edge;
	      for (auto it = boost::begin(boost::out_edges(vertex, graph));
		   it != boost::end(boost::out_edges(vertex, graph));
		   ++it)
		if (graph[*it].reachable_unstable)
		  out_edge = *it;

	      if (stable_count(0, out_edge) > 1) {
		std::set<std::pair<int, int>> previous;
		for (const auto in : boost::make_iterator_range(boost::in_edges(vertex, graph)))
		  if (graph[in].reachable_stable)
		    previous.insert(boost::begin(graph[in].stable_changes | boost::adaptors::map_keys), boost::end(graph[in].stable_changes | boost::adaptors::map_keys));
		
		std::vector<std::pair<int, int>> changes;
		boost::set_difference(graph[out_edge].stable_changes | boost::adaptors::map_keys, previous, std::back_inserter(changes));

		if (changes.size() == 2 && changes[1] == std::make_pair(changes[0].second, changes[0].first)) {
		  const auto vertex = boost::add_vertex(ms);
		  ms[vertex].type = 'H';
		  boost::add_edge(ms_stable.at(changes[0].first - 1), vertex, ms);
		  boost::add_edge(ms_stable.at(changes[0].second - 1), vertex, ms);
		  boost::add_edge(ms_unstable.at(*graph[out_edge].stable_changes[changes[0]] - 1), vertex, ms);
		  boost::add_edge(ms_unstable.at(*graph[out_edge].stable_changes[changes[1]] - 1), vertex, ms);
		}
	      }
	    }
	    if (boost::out_degree(vertex, graph) > 1) {
	      GraphEdge in_edge;
	      for (auto it = boost::begin(boost::in_edges(vertex, graph));
		   it != boost::end(boost::in_edges(vertex, graph));
		   ++it)
		if (graph[*it].reachable_unstable)
		  in_edge = *it;

	      if (unstable_count(0, in_edge) > 1) {
		std::set<std::pair<int, int>> previous;
		for (const auto out : boost::make_iterator_range(boost::out_edges(vertex, graph)))
		  if (graph[out].reachable_unstable)
		    previous.insert(boost::begin(graph[out].unstable_changes | boost::adaptors::map_keys), boost::end(graph[out].unstable_changes | boost::adaptors::map_keys));
		
		std::vector<std::pair<int, int>> changes;
		boost::set_difference(graph[in_edge].unstable_changes | boost::adaptors::map_keys, previous, std::back_inserter(changes));
		
		if (changes.size() == 2 && changes[1] == std::make_pair(changes[0].second, changes[0].first)) {
		  const auto vertex = boost::add_vertex(ms);
		  ms[vertex].type = 'H';
		  boost::add_edge(ms_unstable.at(changes[0].first - 1), vertex, ms);
		  boost::add_edge(ms_unstable.at(changes[0].second - 1), vertex, ms);
		  boost::add_edge(ms_stable.at(*graph[in_edge].unstable_changes[changes[0]] - 1), vertex, ms);
		  boost::add_edge(ms_stable.at(*graph[in_edge].unstable_changes[changes[1]] - 1), vertex, ms);
		}
	      }
	    }
	  }

	  std::ofstream ms_file("ms.dot");
	  boost::write_graphviz(ms_file, ms, boost::make_label_writer(boost::get(&MSVertexProperty::type, ms)));
	  ms_file.close();*/

          saver.level_graph(filename, center_sphere.value().value,
                            level_count.value().value, graph, stable_edges,
                            underlying_unstable, arcs);
          results[level_count.index()][area_ratio.index()][center.index()] =
              make_pair(stable_edges.size(), unstable_edges.size());

          // this part only makes sense when dealing with a single graph
          if (boost::find(area_ratio.value().next, FIRST) !=
                  area_ratio.value().next.end() &&
              center.index() == 0) {
            for (const auto &vertex :
                 boost::make_iterator_range(boost::vertices(graph)))
              graph[vertex].visited = false;
            shape::mark_inside(graph, stable_vertices, stable_edges,
                               visited_map);
            shape::mark_inside(reverse, unstable_vertices, unstable_edges,
                               visited_map);

            // make a copy
            Graph reeb(graph);
            auto reeb_visited_map = boost::get(&VertexProperty::visited, reeb);
            auto reeb_edge_level = boost::get(&EdgeProperty::level, reeb);
            auto reeb_vertex_level = boost::get(&VertexProperty::level, reeb);
            shape::make_reeb(reeb, reeb_visited_map, reeb_edge_level,
                             reeb_vertex_level);
            for (const auto &vertex :
                 boost::make_iterator_range(boost::vertices(reeb)) | indexed())
              reeb[vertex.value()].id = vertex.index();
            auto reeb_vertex_id = boost::get(&VertexProperty::id, reeb);
            auto reeb_vertex_label = boost::get(&VertexProperty::label, reeb);
            shape::reeb_encode(reeb, reeb_vertex_id, reeb_vertex_label);
            saver.reeb(filename, center_sphere.value().value,
                       level_count.value().value, area_ratio.value().value,
                       FIRST, reeb, shape::encode(reeb, reeb_vertex_label));
            if (shape::make_morse(reeb, reeb_vertex_level, reeb_vertex_label))
              saver.morse(filename, center_sphere.value().value,
                          level_count.value().value, area_ratio.value().value,
                          FIRST, shape::encode(reeb, reeb_vertex_label));
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
              return std::tie(su1.second, su1.first) <
                     std::tie(su2.second, su2.first);
            });
            break;
          case UMAX:
            su = *max_element(r, [](const auto &su1, const auto &su2) {
              return std::tie(su1.second, su1.first) <
                     std::tie(su2.second, su2.first);
            });
            break;
          }
          saver.su(filename, center_sphere.value().value,
                   level_count.value().value, area_ratio.value().value,
                   aggregation, su);
        }
      }
    }
  }
}

#endif // MODEL_EXECUTE_HPP
