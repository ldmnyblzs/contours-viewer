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

#ifndef MESHVIEW_HPP
#define MESHVIEW_HPP

#include <vtkAxesActor.h>
#include <vtkNew.h>
#include <vtkPolyData.h>

#include "model/primitives.hpp"
#include "wxVTKWidget.hpp"

class MeshView final : public wxVTKWidget {
  wxCriticalSection m_critical_section;
  vtkNew<vtkPolyData> m_preparedMesh, m_displayedMesh;
  vtkNew<vtkPolyData> m_preparedArcs, m_displayedArcs;
  vtkNew<vtkAxesActor> m_axesActor;
  void Initialize();

public:
  template <typename... Args>
  explicit MeshView(Args &&... args)
      : wxVTKWidget(std::forward<Args>(args)...) {
    Initialize();
  }
  void PrepareMesh(const Mesh &mesh);
  void PrepareArcs(const Graph &graph,
                   std::vector<GraphEdge> stable_edges,
                   std::vector<GraphEdge> unstable_edges);
  void SwapMesh();
  void SwapArcs();
};

#endif // MESHVIEW_HPP
