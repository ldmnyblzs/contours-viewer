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
