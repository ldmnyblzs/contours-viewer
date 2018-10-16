#include "computation.hpp"

#include <boost/range/numeric.hpp>
#include <tbb/combinable.h>
#include <tbb/parallel_for.h>

#include "subrangegraph.hpp"

struct ArcAndVertex {
  Arc_ arc;
  SubrangeGraph::vertex vertex;
};

Computation::Computation(Mesh mesh, std::function<bool()> cancelled)
    : m_mesh(std::move(mesh)), m_cancelled(std::move(cancelled)) {}

bool Computation::run(const Parameters &parameters) {
  m_su.clear();
  m_level_graphs.clear();
  m_reeb_graphs.clear();

  for (auto center_sphere : parameters) {
    const auto centers = static_cast<CenterSphereGenerator>(
        center_sphere.get<1>())(m_mesh.volume());

    std::vector<std::vector<std::vector<std::pair<unsigned int, unsigned int>>>>
        results(center_sphere.get<1>().next_count());
    for (auto level_count : center_sphere.get<1>()) {
      results[level_count.get<0>()].resize(level_count.get<1>().next_count());
      for (auto area_ratio : level_count.get<1>()) {
        results[level_count.get<0>()][area_ratio.get<0>()].resize(
            centers.size());
      }
    }

    for (std::size_t c = 0; c < centers.size(); ++c) {
      auto min_distance = m_mesh.min_distance(centers[c]);
      auto max_distance = m_mesh.max_distance(centers[c]);

      for (auto level_count : center_sphere.get<1>()) {
        auto step = (max_distance - min_distance) / (level_count.get<1>() + 1);
        auto spheres = [&] {
          std::vector<Sphere> temp;
          temp.reserve(static_cast<std::size_t>(level_count.get<1>() + 1));
          for (int i = 1; i <= level_count.get<1>(); ++i)
            temp.emplace_back(centers[c],
                              std::pow(min_distance + step * i, 2.0));
          // a sphere that's larger than the mesh
          temp.emplace_back(centers[c], std::pow(max_distance + step, 2.0));
          return temp;
        }();

        if (m_cancelled())
          return false;

        tbb::combinable<SubrangeGraph> graph;
        tbb::parallel_for(m_mesh.faces(), [&](const auto &range) {
          for (auto &&face : range) {
            const int first_sphere = static_cast<int>(
                (face.min_distance(centers[c]) - min_distance) / step);
            const int last_sphere = std::min<int>(
                static_cast<int>(
                    (face.max_distance(centers[c]) - min_distance) / step),
                level_count.get<1>());
            std::vector<ArcAndVertex> current_vertices = {ArcAndVertex{
                face.null_arc(), graph.local().add_vertex(first_sphere)}};
            for (auto sphere_index = first_sphere; sphere_index <= last_sphere;
                 ++sphere_index) {

              if (this->m_cancelled())
                tbb::task::self().cancel_group_execution();

              std::vector<ArcAndVertex> next_vertices;
              const auto next_arcs =
                  face.intersect(spheres[sphere_index], sphere_index);

              for (auto &&curr_v : current_vertices) {
                // area
                double arc_area = curr_v.arc.approximate_area();
                std::vector<Point> polygon;
                if (curr_v.arc.source) {
                  polygon.push_back(curr_v.arc.target->point());
                  polygon.push_back(curr_v.arc.source->point());
                  // external graph
                  graph.local().connect_neighbor(curr_v.arc.source->halfedge,
                                                 sphere_index, SOURCE,
                                                 curr_v.vertex);
                  graph.local().connect_neighbor(curr_v.arc.target->halfedge,
                                                 sphere_index, TARGET,
                                                 curr_v.vertex);
                }

                bool is_first = true;
                boost::optional<Vertex> prev_vertex = boost::none;
                boost::optional<Vertex> first_vertex = boost::none;
                std::set<Vertex> skip;
                for (auto &&v : curr_v.arc.passed_by) {
                  if (skip.count(v) == 0) {
                    [&] {
                      for (auto &&n_arc : next_arcs) {
                        if (n_arc.beyond(v)) {
                          // skip other vertices covered by this
                          skip.insert(n_arc.passed_by.cbegin(),
                                      n_arc.passed_by.cend());
                          // internal graph
                          next_vertices.push_back(
                              {n_arc,
                               graph.local().add_vertex(sphere_index + 1)});
                          graph.local().add_edge(curr_v.vertex,
                                                 next_vertices.back().vertex,
                                                 n_arc);
                          // area
                          arc_area += n_arc.approximate_area();
                          if (n_arc.source) {
                            polygon.push_back(n_arc.source->point());
                            polygon.push_back(n_arc.target->point());
                            // external graph
                            if (!curr_v.arc.source ||
                                (curr_v.arc.source &&
                                 (curr_v.arc.source->halfedge !=
                                  n_arc.source->halfedge)))
                              graph.local().connect_neighbor(
                                  n_arc.source->halfedge, sphere_index, SOURCE,
                                  curr_v.vertex);
                            if (!curr_v.arc.target ||
                                (curr_v.arc.target &&
                                 (curr_v.arc.target->halfedge !=
                                  n_arc.target->halfedge)))
                              graph.local().connect_neighbor(
                                  n_arc.target->halfedge, sphere_index, TARGET,
                                  curr_v.vertex);
                          }
                          prev_vertex.reset();
                          return;
                        }
                      }
                      // area
                      polygon.push_back(v.point());
                      // external graph
                      if (prev_vertex)
                        graph.local().connect_neighbor(
                            prev_vertex->halfedge_to(v), sphere_index, BOTH,
                            curr_v.vertex);
                      prev_vertex = v;
                      if (is_first)
                        first_vertex = v;
                    }();
                  }
                  is_first = false;
                }
                if (!curr_v.arc.source && prev_vertex && first_vertex &&
                    prev_vertex != first_vertex)
                  graph.local().connect_neighbor(
                      prev_vertex->halfedge_to(*first_vertex), sphere_index,
                      BOTH, curr_v.vertex);

                graph.local().set_area(
                    curr_v.vertex,
                    arc_area + polygon_area(polygon, face.normal()));
              }
              current_vertices = std::move(next_vertices);
            }
          }
        });

        if (m_cancelled())
          return false;

        const auto combined = graph.combine([this](auto &g1, const auto &g2) {
          if (this->m_cancelled())
            tbb::task::self().cancel_group_execution();
          return g1.merge(g2);
        });

        if (m_cancelled())
          return false;

        auto level_graph = combined.minor_graph(level_count.get<1>() + 1);
        for (auto area_ratio : level_count.get<1>()) {
          if (m_cancelled())
            return false;
          results[level_count.get<0>()][area_ratio.get<0>()][c] =
              level_graph.get_SU(m_mesh.surface_area() * area_ratio.get<1>());

          auto reeb_graph = level_graph.minor();

          if (centers.size() == 1) { // won't be aggregated, might be rendered
            m_level_graphs.push_back(level_graph);
            m_reeb_graphs.push_back(reeb_graph);
          }
        }
      }

      if (m_cancelled())
        return false;
    }

    for (auto level_count : center_sphere.get<1>()) {
      for (auto area_ratio : level_count.get<1>()) {
        for (auto aggregation : area_ratio.get<1>()) {
          const auto &r = results[level_count.get<0>()][area_ratio.get<0>()];
          switch (aggregation.get<1>()) {
          case FIRST:
            m_su.push_back(r[0]);
            break;
          case AVERAGE:
            m_su.push_back(boost::accumulate(r, SUPair(0, 0)) / centers.size());
            break;
          case SMIN:
            m_su.emplace_back(*boost::min_element(r));
            break;
          case SMAX:
            m_su.emplace_back(*boost::max_element(r));
            break;
          case UMIN:
            m_su.emplace_back(
                *boost::min_element(r, [](const auto &su1, const auto &su2) {
                  return std::tie(su1.second, su1.first) <
                         std::tie(su2.second, su2.first);
                }));
            break;
          case UMAX:
            m_su.emplace_back(
                *boost::max_element(r, [](const auto &su1, const auto &su2) {
                  return std::tie(su1.second, su1.first) <
                         std::tie(su2.second, su2.first);
                }));
            break;
          }
        }
      }
    }

    if (m_cancelled())
      return false;
  }
  return true;
}

const Mesh &Computation::mesh() const noexcept { return m_mesh; }

const std::vector<SUPair> &Computation::su() const noexcept { return m_su; }

const std::vector<double> &Computation::ratios() const noexcept {
  return m_ratios;
}

const std::vector<LevelGraph> &Computation::level_graphs() const noexcept {
  return m_level_graphs;
}

const std::vector<ReebGraph> &Computation::reeb_graphs() const noexcept {
  return m_reeb_graphs;
}

double Computation::polygon_area(const std::vector<Point> &points,
                                 const Vector &normal) const {
  const auto size = points.size();
  if (size >= 3) {
    Vector sum(0, 0, 0);
    for (std::size_t j = 0; j < size; ++j)
      sum += CGAL::cross_product(points[j] - CGAL::ORIGIN,
                                 points[(j + 1) % size] - CGAL::ORIGIN);
    return CGAL::scalar_product(sum, normal) * 0.5;
  }
  return 0;
}

/*bool Computation::cancelled() const {
    const auto current = std::chrono::system_clock::now();
    const auto elapsed = current - m_started;
    if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >
   m_deadline) return true; return m_cancelled;
    }*/
