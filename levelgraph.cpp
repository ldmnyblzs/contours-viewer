#include "levelgraph.hpp"

#include <fstream>
#include <deque>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/max_element.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/algorithm/for_each.hpp>

template <typename G>
double LevelGraph::subtree_area(const G &g,
                                typename boost::graph_traits<G>::vertex_descriptor vertex,
                                std::set<typename boost::graph_traits<G>::vertex_descriptor> &visited) const {
    visited.insert(vertex);
    double area = g[vertex].area;
    for (const auto &edge : boost::make_iterator_range(boost::in_edges(vertex, g)))
        area += subtree_area(g, boost::source(edge, g), visited);
    return area;
}

template <typename G>
unsigned int LevelGraph::search(G &g, const double min_area, const LineType line_type) {
    std::set<std::set<typename boost::graph_traits<G>::vertex_descriptor> > min_areas;
    for (const auto &vertex : boost::make_iterator_range(boost::vertices(g))) {
        if (boost::in_degree(vertex, g) == 0) {
            auto v = vertex;
            while (boost::out_degree(v, g) == 1) {
                std::set<typename boost::graph_traits<G>::vertex_descriptor> visited;
                const double area = subtree_area(g, v, visited);
                if (area >= min_area) {
                    min_areas.insert(visited);
                    break;
                }
                v = boost::target(*boost::begin(boost::out_edges(v, g)), g);
            }
        }
    }

    unsigned int count = 0;
    for (auto &&set1 : min_areas) {
        unsigned int includes = 0;
        for (auto &&set2 : min_areas)
            if (std::includes(set1.begin(),
                              set1.end(),
                              set2.begin(),
                              set2.end()))
                ++includes;
        if (includes == 1) {
            ++count;
            for (auto &&vertex : set1) {
                std::size_t asdf = 0;
                for (const auto &edge : boost::make_iterator_range(boost::out_edges(vertex, g)))
                    asdf += set1.count(boost::target(edge, g));

                if (asdf == 0)
                    for (const auto &edge : boost::make_iterator_range(boost::out_edges(vertex, g)))
                        g[edge].type = line_type;
            }
        }
    }
    return count;
}

std::pair<unsigned int, unsigned int> LevelGraph::get_SU(const double min_area) {
    m_last_modified = std::chrono::system_clock::now();
    for (auto &&edge : boost::make_iterator_range(boost::edges(graph)))
        graph[edge].type = SIMPLE;
    auto reverse = boost::make_reverse_graph(graph);
    return {search(graph, min_area, STABLE), search(reverse, min_area, UNSTABLE)};
}

std::chrono::system_clock::time_point LevelGraph::last_modified() const
{
    return m_last_modified;
}

ReebGraph LevelGraph::minor()
{
    using namespace std;
    using namespace boost;
    using namespace boost::adaptors;
	using namespace boost::range;

    for (const auto edge : edges()) {
        graph[edge].reset();
    }

	auto stable_map = boost::get(&EdgeProperty::stable, graph);
    for (const auto root : edges_of_type(graph, STABLE)) {
        vector<default_color_type> colors(num_vertices(graph), default_color_type::white_color);
        depth_first_visit(graph,
                          source(root, graph),
                          su_visitor<STABLE, decltype(stable_map)>(stable_map),
                          make_iterator_property_map(colors.begin(), vertex_indices()));
    }

    auto reverse = make_reverse_graph(graph);
	auto unstable_map = boost::get(&EdgeProperty::unstable, reverse);
    for (auto root : edges_of_type(reverse, UNSTABLE)) {
        vector<default_color_type> colors(num_vertices(reverse), default_color_type::white_color);
        depth_first_visit(reverse,
                          source(root, reverse),
                          su_visitor<UNSTABLE, decltype(unstable_map)>(unstable_map),
                          make_iterator_property_map(colors.begin(), vertex_indices()));
   }
	
   for (auto edge : edges()) {
        EdgeProperty &properties = graph[edge];
        properties.accessible = properties.stable != 0 && properties.unstable != 0;
    }

    for (auto vertex : vertices()) {
        graph[vertex].accessible = false;
        for (auto edge : in_and_out_edges(vertex))
            if (graph[edge].accessible) {
                graph[vertex].accessible = true;
                break;
            }
    }

    const auto filtered = make_filtered_graph(graph,
                                              std::function<bool(const edge&)>([this](const auto &edge) {
        return graph[edge].accessible;
    }),
    std::function<bool(const vertex&)>([this](const auto &vertex) {
        return graph[vertex].accessible;
    }));

    auto junctions = copy_range<deque<LevelGraph::vertex> >(root_vertices(filtered));
    ReebGraph result(junctions.size());
    for_each(combine(junctions, result.vertices()),
	     [&result, this](const auto &root) {
	       result.level(get<1>(root), graph[get<0>(root)].level);
	     });
	
    auto reeb_vertices = copy_range<map<LevelGraph::vertex, ReebGraph::vertex> >(junctions |
                                                                                 indexed(0) |
                                                                                 transformed([](const auto &it) {
        return make_pair(it.value(), it.index());
    }));

    while (!junctions.empty()) {
        for (const auto adjacent : make_iterator_range(adjacent_vertices(junctions.front(), filtered))) {
            LevelGraph::vertex vertex = adjacent;
            while (in_degree(vertex, filtered) == 1 && out_degree(vertex, filtered) == 1)
                 vertex = *begin(adjacent_vertices(vertex, filtered));

            auto target_it = reeb_vertices.lower_bound(vertex);
            if (target_it == reeb_vertices.end() || target_it->first != vertex) {
				const float level = graph[vertex].level - (out_degree(vertex, filtered) == 0 ? 1.0f : 0.5f);
                target_it = reeb_vertices.emplace_hint(target_it, vertex, result.add_vertex(level));
                if (in_degree(vertex, filtered) != 0 && out_degree(vertex, filtered) != 0)
                    junctions.push_back(vertex);
            }
            result.add_edge(reeb_vertices.at(junctions.front()), target_it->second);
        }

        junctions.pop_front();
    }

    return result;
}

void LevelGraph::save() const
{
    std::ofstream file("level.dot");
    boost::write_graphviz(file,
                          graph,
                          [this](auto &stream, const auto &vertex) {
        stream << "[label=\"" << boost::get(boost::vertex_index, graph, vertex) << ':'
               << graph[vertex].level << ':'
               << graph[vertex].area << "\"]";
    },
    [this](auto &stream, const auto &edge) {
        stream << "[label=\"" << graph[edge].stable << ':' << graph[edge].unstable;
        switch (graph[edge].type) {
        case STABLE:
            stream << ":S";
            break;
        case UNSTABLE:
            stream << ":U";
            break;
        }
        stream << "\"]";
    });
    file.flush();
    file.close();
}
