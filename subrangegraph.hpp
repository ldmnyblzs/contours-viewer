#ifndef SUBRANGE_GRAPH_HPP
#define SUBRANGE_GRAPH_HPP

#include "levelgraph.hpp"

enum Type {SOURCE, TARGET, BOTH};

const Type opposite[] = {TARGET, SOURCE, BOTH};

struct Neighbor {
    Halfedge halfedge;
    int sphere;
    Type type;
};

namespace std {
template <> struct hash<Neighbor> {
    size_t operator()(const Neighbor &neighbor) const {
        const size_t halfedge = neighbor.halfedge.handle();
        const size_t sphere = neighbor.sphere;
        const size_t type = neighbor.type;
        // two bits overlap
        return halfedge | (sphere >> 30) | (type >> 62);
    }
};
template <> struct equal_to<Neighbor> {
    bool operator()(const Neighbor &n1, const Neighbor &n2) const {
        return std::tie(n1.halfedge, n1.sphere, n1.type) == std::tie(n2.halfedge, n2.sphere, n2.type);
    }
};
}

class SubrangeGraph {
    struct VertexProperty {
        double area;
        int level;
    };
    struct EdgeProperty {
        boost::optional<Arc_> arc;
    };

    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, VertexProperty, EdgeProperty> adjacency_list;
    adjacency_list graph;
public:
    typedef boost::graph_traits<adjacency_list>::vertex_descriptor vertex;
    typedef boost::graph_traits<adjacency_list>::edge_descriptor edge;
private:
    std::unordered_map<Neighbor, vertex> neighbors;
public:

    vertex add_vertex(int level);
    edge add_edge(vertex source, vertex target);
    edge add_edge(vertex source, vertex target, const Arc_ &arc);
    void set_area(vertex vertex, double area);
    LevelGraph minor_graph(int count) const;
    auto number_of_vertices() const {
        return boost::num_vertices(graph);
    }
    auto index_map() const {
        return boost::get(boost::vertex_index, graph);
    }
    void connect_neighbor(const Halfedge &halfedge, int sphere, Type type, vertex vertex);
    SubrangeGraph merge(const SubrangeGraph &other);
    void save() const;
};

#endif // SUBRANGE_GRAPH_HPP
