#ifndef GRAPHVIEW_HPP
#define GRAPHVIEW_HPP

#include <vtkSmartPointer.h>

#include "wxVTKWidget.hpp"
#include "reebgraph.hpp"

class vtkGraphLayout;
class vtkGraphLayoutView;

class GraphView final : public wxVTKWidget {
  vtkSmartPointer<vtkGraphLayout> m_layout;
  vtkSmartPointer<vtkGraphLayoutView> m_view;
  void Initialize();
	template <typename Map, typename Adjacent>
  int Layout(const ReebGraph::vertex root, Map &columns, Adjacent adjacent, const int column = 0);
public:
  template <typename... Args>
  explicit GraphView(Args&&... args) :
    wxVTKWidget(std::forward<Args>(args)...) {
    Initialize();
  }

  void UpdateGraph(const ReebGraph &graph);
};

#endif // GRAPHVIEW_HPP
