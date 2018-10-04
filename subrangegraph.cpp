#include "subrangegraph.hpp"

#include <boost/range/numeric.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>

SubrangeGraph::vertex SubrangeGraph::add_vertex(int level) {
  const auto vertex = boost::add_vertex(graph);
  graph[vertex].level = level;
  return vertex;
}
SubrangeGraph::edge SubrangeGraph::add_edge(vertex source, vertex target) {
  return std::get<0>(boost::add_edge(source, target, graph));
}
SubrangeGraph::edge SubrangeGraph::add_edge(vertex source, vertex target, const Arc_ &arc) {
  const auto edge = add_edge(source, target);
  graph[edge].arc = arc;
  return edge;
}

void SubrangeGraph::set_area(vertex vertex, double area) {
    graph[vertex].area = area;
}

void SubrangeGraph::connect_neighbor(const Halfedge &halfedge,
                             int sphere,
                             Type type,
                             vertex vertex) {
    const auto opit = neighbors.find({halfedge.opposite(), sphere, opposite[type]});
    if (opit != neighbors.end()) {
        add_edge(opit->second, vertex);
        neighbors.erase(opit);
    } else {
        neighbors.emplace(Neighbor{halfedge, sphere, type}, vertex);
    }
}

SubrangeGraph SubrangeGraph::merge(const SubrangeGraph &other) {
    std::vector<vertex> vertex_vector(other.number_of_vertices());
    auto new_vertex = boost::make_iterator_property_map(vertex_vector.begin(), other.index_map());
    boost::copy_graph(other.graph, graph, boost::orig_to_copy(new_vertex));
    for (auto &&n : other.neighbors) {
        const auto &neighbor = n.first;
        const auto opit = neighbors.find({neighbor.halfedge.opposite(), neighbor.sphere, opposite[neighbor.type]});
        if (opit != neighbors.end()) {
            add_edge(opit->second, new_vertex[n.second]);
            neighbors.erase(opit);
        } else {
            neighbors.emplace(neighbor, new_vertex[n.second]);
        }
    }
    return *this;
}

void SubrangeGraph::save() const
{
    std::ofstream file("subrange.dot");
    boost::write_graphviz(file, graph);
    file.close();
}

LevelGraph SubrangeGraph::minor_graph(int count) const {
    LevelGraph temp;
    std::vector<std::vector<LevelGraph::vertex> > vertices(count);

    std::vector<int> component_vec(number_of_vertices());
    auto component_map = boost::make_iterator_property_map(component_vec.begin(), index_map());

  for (int level = 0; level < count; ++level) {
    const auto level_graph = boost::make_filtered_graph(graph,
                                                        boost::keep_all(),
                                                        std::function<bool(vertex)>([this, level](vertex v) {
                                                          return graph[v].level == level;
                                                          }));
    const int component_count = boost::connected_components(level_graph, component_map);
    vertices[level].resize(component_count);
    for (int component = 0; component < component_count; ++component) {
      const auto component_graph = boost::make_filtered_graph(graph,
							      boost::keep_all(),
                                  std::function<bool(vertex)>([this, &component_map, level, component](vertex v) {
                                return graph[v].level == level && component_map[v] == component;
							      }));
      vertices[level][component] = temp.add_vertex(level,
                                                   boost::accumulate(boost::vertices(component_graph),
                                                                     0.0,
                                                                     [this](const auto &init, const auto &vertex) {
          return init + graph[vertex].area;
      }));
    }
  }
  const auto between_components = boost::make_filtered_graph(graph,
                                 std::function<bool(edge)>([this](edge e) {
                                 return graph[boost::source(e, graph)].level != graph[boost::target(e, graph)].level;
							       }),
							     boost::keep_all());
  for (const auto &edge : boost::make_iterator_range(boost::edges(between_components))) {
    const auto source = boost::source(edge, graph);
    const auto target = boost::target(edge, graph);
    if (graph[source].level < graph[target].level) {
        temp.add_edge(vertices[graph[source].level][component_map[source]], vertices[graph[target].level][component_map[target]], *graph[edge].arc);
    } else {
        temp.add_edge(vertices[graph[target].level][component_map[target]], vertices[graph[source].level][component_map[source]], *graph[edge].arc);
    }
  }
  return temp;
}
