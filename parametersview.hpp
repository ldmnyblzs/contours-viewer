/*
  Copyright 2019 Balázs Ludmány

  This file is part of contours-viewer.

  contours-viewer is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  contours-viewer is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with contours-viewer.  If not, see <https://www.gnu.org/licenses/>.
*/

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
