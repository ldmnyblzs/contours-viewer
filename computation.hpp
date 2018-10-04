#ifndef COMPUTATION_HPP
#define COMPUTATION_HPP

#include <string>
#include <functional>

#include "parameters.hpp"
#include "result.hpp"
#include "mesh.hpp"
#include "levelgraph.hpp"

/*
 * An execution of the algorithm.
 */
class Computation
{
  Mesh m_mesh;
  
  std::function<bool()> m_cancelled;
    
  // OUTPUT
  std::vector<SUPair> m_su;
  std::vector<double> m_ratios;
  std::vector<LevelGraph> m_level_graphs;
  std::vector<ReebGraph> m_reeb_graphs;
  
  double polygon_area(const std::vector<Point> &points, const Vector &normal) const;
public:
  /*!
   * Create a Computation with the given mesh
   */
  explicit Computation(Mesh mesh, std::function<bool()> cancelled = []{return false;});
  /*!
   * Run the actual computation
   */
  bool run(const Parameters &parameters);

  const Mesh &mesh() const noexcept;
  const std::vector<SUPair> & su() const noexcept;
  const std::vector<double> &ratios() const noexcept;
  const std::vector<LevelGraph> &level_graphs() const noexcept;
  const std::vector<ReebGraph> &reeb_graphs() const noexcept;
};

#endif // COMPUTATION_HPP
