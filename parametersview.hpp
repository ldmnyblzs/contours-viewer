#ifndef PARAMETERS_VIEW_HPP
#define PARAMETERS_VIEW_HPP

#include <wx/grid.h>
#include <wx/thread.h>
#include "model/parameters.hpp"

class ParametersView final : public wxGrid {
  wxCriticalSection m_critical_section;
  std::vector<std::array<wxString, 5>> m_table;
  void Initialize();
public:
  template <typename... Args>
  explicit ParametersView(Args&&... args) :
    wxGrid(std::forward<Args>(args)...) {
    Initialize();
  }
  void Update(const Parameters &parameters);
  void Swap();
};

#endif // PARAMETERS_VIEW_HPP
