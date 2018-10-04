#ifndef LEVEL_GRAPH_HPP
#define LEVEL_GRAPH_HPP

#include <chrono>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/join.hpp>
#include <boost/graph/depth_first_search.hpp>

#include "mesh.hpp"
#include "reebgraph.hpp"

#undef minor

class LevelGraph {
public:
    enum LineType {
        SIMPLE,
        STABLE,
        UNSTABLE
    };
private:
    struct VertexProperty {
        int level;
        double area;
        bool accessible;
    };

    struct EdgeProperty {
        std::vector<Arc_> arcs;
        LineType type;
        std::size_t stable, unstable;
        bool accessible;
        void reset() {
            stable = 0;
            unstable = 0;
            accessible = false;
        }
    };

    typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS, VertexProperty, EdgeProperty> adjacency_list;

    adjacency_list graph;
public:
    typedef boost::graph_traits<adjacency_list>::vertex_descriptor vertex;
    typedef boost::graph_traits<adjacency_list>::edge_descriptor edge;
private:
    std::chrono::system_clock::time_point m_last_modified;

    template <LineType type, typename PropertyMap>
    class su_visitor : public boost::default_dfs_visitor
    {
		PropertyMap map;
        std::shared_ptr<bool> found;
		template <typename Edge, typename Graph>
        void visit(Edge e, Graph &g) {
            *found |= g[e].type == type;
            if (*found)
                ++map[e];
        }
    public:
        su_visitor(PropertyMap map) : map(map), found(new bool(false)) {}
		template <typename Edge, typename Graph>
        void tree_edge(Edge e, Graph &g) {
            visit(e, g);
        }
		template <typename Edge, typename Graph>
        void forward_or_cross_edge(Edge e, Graph &g) {
            visit(e, g);
        }
		template <typename Edge, typename Graph>
        void finish_edge(Edge e, Graph &g) {
            *found &= g[e].type != type;
        }
    };

    template <typename G> double subtree_area(const G &g,
                                                   typename boost::graph_traits<G>::vertex_descriptor vertex,
                                                   std::set<typename boost::graph_traits<G>::vertex_descriptor> &visited) const;
    template <typename G> unsigned int search(G &g, const double min_area, const LineType line_type);

    template <typename G>
    static auto edges_of_type(const G &g, const LineType type) {
        return boost::adaptors::filter(boost::edges(g),
                                       [&g, type](const auto &edge) {
            return g[edge].type == type;
        });
    }
    template <typename G>
    static auto root_vertices(const G &g) {
        return boost::adaptors::filter(boost::vertices(g),
                                       [&](const auto &vertex) {
            return boost::in_degree(vertex, g) == 0;
        });
    }
public:
    vertex add_vertex(int level, double area) {
        const auto vertex = boost::add_vertex(graph);
        graph[vertex].level = level;
        graph[vertex].area = area;
        return vertex;
    }
    edge add_edge(vertex source, vertex target, const Arc_ &arc) {
        const auto edge = std::get<0>(boost::add_edge(source, target, graph));
        graph[edge].arcs.push_back(arc);
        return edge;
    }
    auto vertices() const noexcept {
        return boost::make_iterator_range(boost::vertices(graph));
    }
    auto edges() const noexcept {
        return boost::make_iterator_range(boost::edges(graph));
    }
    auto in_and_out_edges(vertex v) const noexcept {
        return boost::join(boost::in_edges(v, graph), boost::out_edges(v, graph));
    }
    auto vertex_indices() const noexcept {
        return boost::get(boost::vertex_index, graph);
    }
    auto arcs(edge edge) const {
        return graph[edge].arcs;
    }
    auto type(edge edge) const {
        return graph[edge].type;
    }
    auto number_of_vertices() const {
        return boost::num_vertices(graph);
    }

    std::pair<unsigned int, unsigned int> get_SU(const double min_area);
    std::chrono::system_clock::time_point last_modified() const;
    ReebGraph minor();
    void save() const;
};

#endif // LEVEL_GRAPH_HPP
