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

#ifndef OUTPUT_VIEW_HPP
#define OUTPUT_VIEW_HPP

#include <dependencies.hpp>

#ifdef GD_FOUND
#undef TrueColor
#include <gdpp.h>
#endif //GD_FOUND

#include <wx/thread.h>
#include "model/parameters.hpp"

class OutputView final : public wxGrid {
  wxCriticalSection m_critical_section;
  std::vector<wxString> m_table;
  void Initialize(const std::string &fileName);
public:
  template <typename... Args>
  explicit OutputView(const std::string &fileName, Args&&... args) :
    wxGrid(std::forward<Args>(args)...) {
    Initialize(fileName);
  }
  void UpdateParameters(const Vector &offset, int level_count, double area_ratio);
  void UpdateMeshData(const std::array<double, 13> &properties);
  void UpdateRatios(const std::array<double, 6> &properties);
  void UpdateSU(int S, int U);
  void UpdateReeb(const std::string &reeb);
  void UpdateMorse(const std::string &morse);
  void Swap();
#ifdef GD_FOUND
  GD::Image Screenshot() const;
#endif //GD_FOUND
};

#endif // OUTPUT_VIEW_HPP
