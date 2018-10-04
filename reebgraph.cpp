#include "reebgraph.hpp"

#include <fstream>
#include <boost/graph/graphviz.hpp>

ReebGraph::ReebGraph(ReebGraph::size_type vertex_count) :
    graph(vertex_count)
{
}

ReebGraph::vertex ReebGraph::add_vertex(float level)
{
    return boost::add_vertex({level}, graph);
}

ReebGraph::edge ReebGraph::add_edge(ReebGraph::vertex source, ReebGraph::vertex target)
{
    return std::get<0>(boost::add_edge(source, target, graph));
}

void ReebGraph::save() const
{
    std::ofstream file("reeb.dot");
    boost::write_graphviz(file, graph);
    file.close();
}

const ReebGraph::adjacency_list &ReebGraph::adj_list() const
{
    return graph;
}
