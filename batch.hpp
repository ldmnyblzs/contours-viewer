#ifndef BATCH_HPP
#define BATCH_HPP

#include <boost/filesystem/path.hpp>

#include "ratios.hpp"
#include "parameters.hpp"
#include "computation.hpp"

/**
 * @brief Stores the computed values of the mesh
 *
 * There is no point in keeping the whole Mesh object just to query these.
 */
struct MeshData {
  double surface_area, volume, a, b, c, projected_circumference, projected_area;
  std::vector<double> ratios;
  explicit MeshData(const Mesh &mesh) :
    surface_area(mesh.surface_area()),
    volume(mesh.volume()),
    a(mesh.a()),
    b(mesh.b()),
    c(mesh.c()),
    projected_circumference(mesh.projected_circumference()),
    projected_area(mesh.projected_area()),
    ratios(mesh.ratios())
  {
  }
};

#undef Status

enum Status {
    WAITING,
    RUNNING,
    OK,
    ERROR_
};

class Batch {
    class decimal_point : public std::numpunct<char> {
    protected:
        char do_decimal_point() const final {
            return '.';
        }
    };
    class decimal_coma : public std::numpunct<char> {
    protected:
        char do_decimal_point() const final {
            return L',';
        }
    };

  std::function<bool()> m_cancelled;
  std::function<void(std::size_t, Status)> m_status_changed;
  std::chrono::time_point<std::chrono::system_clock> m_started;
  int m_deadline = 5 * 60;
  
  boost::filesystem::path m_directory;
  std::vector<std::string> m_data;
  Parameters m_parameters;
  std::vector<std::string> m_files;
  std::vector<std::string> m_file_data;
  
  std::vector<MeshData> m_mesh_data;
  std::vector<Status> m_statuses;
  std::vector<std::vector<SUPair> > m_su;

  bool cancelled() const;
public:
  explicit Batch(const std::string &path,
  std::function<bool()> cancelled = []{return false;},
  std::function<void(std::size_t, Status)> status_changed = [](std::size_t, Status){return;});
  bool run();
  void save(const std::string &file_path) const;
  const std::vector<std::string> &files() const;
  const std::vector<Status> &statuses() const;
  const Parameters &parameters() const;
};

#endif // BATCH_HPP
