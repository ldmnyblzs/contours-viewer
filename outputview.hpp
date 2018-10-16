#ifndef OUTPUT_VIEW_HPP
#define OUTPUT_VIEW_HPP

#undef TrueColor
#include <gdpp.h>

#include "result.hpp"
#include "mesh.hpp"

class Parameters;

class OutputView final : public wxGrid {
  void Initialize(const wxString &fileName);
public:
  template <typename... Args>
  explicit OutputView(const wxString &fileName, Args&&... args) :
    wxGrid(std::forward<Args>(args)...) {
    Initialize(fileName);
  }
  void UpdateParameters(const Parameters &parameters);
  void UpdateMeshData(const Mesh &mesh);
  void UpdateSU(const SUPair &su);
  GD::Image Screenshot() const;
};

#endif // OUTPUT_VIEW_HPP
