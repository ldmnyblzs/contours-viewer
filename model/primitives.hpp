#ifndef MODEL_PRIMITIVES_HPP
#define MODEL_PRIMITIVES_HPP 1

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/container/flat_set.hpp>
#include <optional>
#include <variant>

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
  std::vector<std::size_t> prev, next;
  std::size_t forward = 0;
  int stable = 0;
  int unstable = 0;
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
  std::list<std::size_t> arcs;
  bool reachable_stable = false, reachable_unstable = false, visited = false;
  std::map<std::pair<int, int>, int*> stable_changes, unstable_changes;
  std::map<std::pair<int, int>, std::variant<std::vector<std::size_t>, std::size_t> > parts;
};


struct MSVertexProperty {
  char type;
};

using MS = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, MSVertexProperty>;
using MS_vertex = typename boost::graph_traits<MS>::vertex_descriptor;
using MS_edge = typename boost::graph_traits<MS>::edge_descriptor;

#endif // MODEL_PRIMITIVES_HPP
