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
