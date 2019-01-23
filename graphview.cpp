#include "graphview.hpp"

#include <boost/container/flat_set.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/max_element.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <functional>
#include <vtkAOSDataArrayTemplate.h>
#include <vtkActor.h>
#include <vtkDataSetAttributes.h>
#include <vtkFast2DLayoutStrategy.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkGlyph2D.h>
#include <vtkGlyphSource2D.h>
#include <vtkGraphLayout.h>
#include <vtkGraphLayoutView.h>
#include <vtkGraphToGlyphs.h>
#include <vtkGraphToPolyData.h>
#include <vtkInteractorStyleRubberBand2D.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkNew.h>
#include <vtkPassThroughLayoutStrategy.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkVariant.h>
#include <vtkViewTheme.h>

void GraphView::Initialize() {
  vtkNew<vtkFast2DLayoutStrategy> strategy;
  vtkNew<vtkGraphLayout> layout;
  layout->SetInputData(m_displayedGraph.GetPointer());
  layout->SetLayoutStrategy(strategy.GetPointer());

  m_view->AddRepresentationFromInputConnection(layout->GetOutputPort());
  m_view->SetLayoutStrategyToPassThrough();
  m_view->SetEdgeLayoutStrategyToPassThrough();
  m_view->SetGlyphType(vtkGraphToGlyphs::CIRCLE);

  vtkNew<vtkViewTheme> theme;
  theme->SetBackgroundColor(1.0, 1.0, 1.0);
  theme->SetBackgroundColor2(1.0, 1.0, 1.0);
  theme->SetPointColor(0.0, 0.0, 0.0);
  theme->SetCellColor(0.0, 0.0, 0.0);
  theme->SetOutlineColor(0.0, 0.0, 0.0);
  theme->SetSelectedPointColor(1.0, 1.0, 0.0);
  theme->SetSelectedCellColor(1.0, 1.0, 0.0);
  m_view->ApplyViewTheme(theme.GetPointer());

  vtkNew<vtkGraphToPolyData> polydata;
  polydata->SetInputConnection(layout->GetOutputPort());
  polydata->EdgeGlyphOutputOn();
  polydata->SetEdgeGlyphPosition(0.99);

  vtkNew<vtkGlyphSource2D> source;
  source->SetGlyphTypeToEdgeArrow();
  source->SetScale(0.2);
  source->Update();

  vtkNew<vtkGlyph2D> glyph;
  glyph->SetInputConnection(0, polydata->GetOutputPort(1));
  glyph->SetInputConnection(1, source->GetOutputPort(0));

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(glyph->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());
  actor->GetProperty()->SetColor(0.0, 0.0, 0.0);
  m_view->GetRenderer()->AddActor(actor.GetPointer());

  vtkNew<vtkGenericOpenGLRenderWindow> window;
  m_view->SetRenderWindow(window.GetPointer());
  m_view->ResetCamera();

  vtkNew<vtkGenericRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window.GetPointer());

  vtkNew<vtkInteractorStyleRubberBand2D> style;
  style->SetPickColor(1.0, 1.0, 0.0);
  interactor->SetInteractorStyle(style.GetPointer());

  SetRenderWindowInteractor(interactor.GetPointer());
  interactor->Enable();
}

void GraphView::UpdateGraph(const Graph &reeb) {
  wxCriticalSectionLocker lock(m_critical_section);
  vtkNew<vtkMutableDirectedGraph> graph;

  vtkNew<vtkIntArray> pedigree;
  for (const auto &vertex : boost::make_iterator_range(boost::vertices(reeb)))
    pedigree->InsertValue(graph->AddVertex(), reeb[vertex].id);
  graph->GetVertexData()->SetPedigreeIds(pedigree.GetPointer());

  for (const auto &edge : boost::make_iterator_range(boost::edges(reeb)))
    graph->AddEdge(vtkVariant(reeb[boost::source(edge, reeb)].id),
                   vtkVariant(reeb[boost::target(edge, reeb)].id));

  m_preparedGraph->ShallowCopy(graph.GetPointer());
}

void GraphView::Swap() {
  wxCriticalSectionLocker lock(m_critical_section);
  m_displayedGraph->ShallowCopy(m_preparedGraph.GetPointer());
  m_view->ResetCamera();
  Refresh();
}
