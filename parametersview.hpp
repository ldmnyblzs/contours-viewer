#ifndef PARAMETERS_VIEW_HPP
#define PARAMETERS_VIEW_HPP

#include <wx/grid.h>

class Parameters;

class ParametersView final : public wxGrid {
  void Initialize();
public:
  template <typename... Args>
  explicit ParametersView(Args&&... args) :
    wxGrid(std::forward<Args>(args)...) {
    Initialize();
  }
  void Update(const Parameters &parameters);
};

#endif // PARAMETERS_VIEW_HPP
