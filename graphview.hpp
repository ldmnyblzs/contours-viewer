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

#ifndef GRAPHVIEW_HPP
#define GRAPHVIEW_HPP

#include <vtkSmartPointer.h>
#include <vtkDirectedGraph.h>
#include <vtkGraphLayoutView.h>

#include "wxVTKWidget.hpp"
//#include "reebgraph.hpp"

#include "model/primitives.hpp"

class GraphView final : public wxVTKWidget {
  wxCriticalSection m_critical_section;
  vtkNew<vtkGraphLayoutView> m_view;
  vtkNew<vtkDirectedGraph> m_preparedGraph, m_displayedGraph;
  void Initialize();
public:
  template <typename... Args>
  explicit GraphView(Args&&... args) :
    wxVTKWidget(std::forward<Args>(args)...) {
    Initialize();
  }

  void UpdateGraph(const Graph &reeb);
  void Swap();
};

#endif // GRAPHVIEW_HPP
