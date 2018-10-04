#ifndef REEBGRAPH_H
#define REEBGRAPH_H

#include <boost/graph/adjacency_list.hpp>
#include <boost/range/adaptor/transformed.hpp>

class ReebGraph
{
	struct VertexProperty {
		float level;
	};
public:
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty> adjacency_list;
    typedef boost::graph_traits<adjacency_list>::vertex_descriptor vertex;
    typedef boost::graph_traits<adjacency_list>::edge_descriptor edge;
    typedef typename adjacency_list::vertices_size_type size_type;
private:
    adjacency_list graph;
public:
    ReebGraph(size_type vertex_count);
    vertex add_vertex(float level);
    edge add_edge(vertex source, vertex target);
    auto vertices() const {
        return boost::make_iterator_range(boost::vertices(graph));
    }
    auto index_map() const {
        return boost::get(boost::vertex_index, graph);
    }
    auto num_vertices() const {
        return boost::num_vertices(graph);
    }
    auto edges() const {
        return boost::make_iterator_range(boost::edges(graph));
    }
	auto in_edges(vertex v) const {
		return boost::in_edges(v, graph);
	}
    auto source(edge e) const {
        return boost::source(e, graph);
    }
    auto target(edge e) const {
        return boost::target(e, graph);
    }
	auto in_degree(vertex v) const {
		return boost::in_degree(v, graph);
	}
	auto out_degree(vertex v) const {
		return boost::out_degree(v, graph);
	}
	auto adjacent_vertices(vertex v) const {
		return boost::make_iterator_range(boost::adjacent_vertices(v, graph));
	}
	auto adjacent_vertices_in(vertex v) const {
		using namespace boost::adaptors;
		return in_edges(v) | transformed([this](const auto &e){return source(e);});
	}
	auto level(vertex v) const {
		return graph[v].level;
	}
	void level(vertex v, float level) {
		graph[v].level = level;
	}
    void save() const;
    const adjacency_list &adj_list() const;
};

#endif // REEBGRAPH_H
