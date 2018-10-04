#include "graphview.hpp"

#include <functional>
#include <vtkNew.h>
#include <vtkGraphLayout.h>
#include <vtkPassThroughLayoutStrategy.h>
#include <vtkViewTheme.h>
#include <vtkGraphLayoutView.h>
#include <vtkGraphToGlyphs.h>
#include <vtkGraphToPolyData.h>
#include <vtkGlyphSource2D.h>
#include <vtkGlyph2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkInteractorStyleRubberBand2D.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkAOSDataArrayTemplate.h>
#include <vtkDataSetAttributes.h>
#include <vtkVariant.h>
#include <vtkPoints.h>
#include <vtkProperty.h>
#include <boost/container/flat_set.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/max_element.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/copy.hpp>

void GraphView::Initialize() {
	m_layout = vtkSmartPointer<vtkGraphLayout>::New();
	m_layout->SetInputData(vtkDirectedGraph::New());
	m_layout->SetLayoutStrategy(vtkPassThroughLayoutStrategy::New());
	
	m_view = vtkSmartPointer<vtkGraphLayoutView>::New();
	m_view->SetRepresentationFromInputConnection(m_layout->GetOutputPort());
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
	polydata->SetInputConnection(m_layout->GetOutputPort());
	polydata->EdgeGlyphOutputOn();
	polydata->SetEdgeGlyphPosition(0.99);
	
	vtkNew<vtkGlyphSource2D> source;
	source->SetGlyphTypeToEdgeArrow();
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

template <typename Map, typename Adjacent>
int GraphView::Layout(const ReebGraph::vertex root, Map &columns, Adjacent adjacent, const int column) {
	columns[root] = column;
	
	int sum_width = 0;
	for (const auto adj : adjacent(root))
		sum_width += Layout(adj, columns, adjacent, column + sum_width);
	return std::max(sum_width, 1);
}

void GraphView::UpdateGraph(const ReebGraph &reeb) {
	using namespace boost;
	using namespace boost::container;
	
	vtkNew<vtkMutableDirectedGraph> graph;
	flat_set<float> levels;
	
	vtkNew<vtkAOSDataArrayTemplate<ReebGraph::vertex> > pedigree;
	for (const auto vertex : reeb.vertices()) {
		pedigree->InsertValue(graph->AddVertex(), vertex);
		levels.insert(reeb.level(vertex));
	}
	graph->GetVertexData()->SetPedigreeIds(pedigree.GetPointer());
	
	// find a leaf and do a depth search until we find a split
	auto first_split = *find_if(reeb.vertices(), [&reeb](const auto &vertex) {
		return reeb.in_degree(vertex) == 0;
	});	
	while (reeb.out_degree(first_split) == 1) {
		first_split = *std::begin(reeb.adjacent_vertices(first_split));
	}
	
	std::vector<int> columns_vector(reeb.num_vertices(), -1);
	auto columns = make_iterator_property_map(columns_vector.begin(), reeb.index_map());
	
	Layout(first_split, columns, [&reeb](const ReebGraph::vertex &vertex) {
		std::vector<ReebGraph::vertex> adjacent(reeb.out_degree(vertex));
		boost::copy(reeb.adjacent_vertices(vertex), adjacent.begin());
		boost::sort(adjacent, [&reeb](const ReebGraph::vertex &adj1, const ReebGraph::vertex &adj2) {
			return reeb.level(adj1) > reeb.level(adj2);
		});
		return adjacent;
	});
	Layout(first_split, columns, [&reeb](const ReebGraph::vertex &vertex) {
		std::vector<ReebGraph::vertex> adjacent(reeb.in_degree(vertex));
		boost::copy(reeb.adjacent_vertices_in(vertex), adjacent.begin());
		boost::sort(adjacent, [&reeb](const ReebGraph::vertex &adj1, const ReebGraph::vertex &adj2) {
			return reeb.level(adj1) < reeb.level(adj2);
		});
		return adjacent;
	});
	
	vtkNew<vtkPoints> points;
	for (const auto vertex : reeb.vertices()) {
		const auto row = levels.index_of(levels.find(reeb.level(vertex)));
		points->InsertNextPoint(columns[vertex] * 5.0, row * 5.0, 0.0);
	}
	graph->SetPoints(points.GetPointer());
	
	for (const auto edge : reeb.edges())
		graph->AddEdge(vtkVariant(reeb.source(edge)), vtkVariant(reeb.target(edge)));
	
	m_layout->SetInputData(graph.GetPointer());
	
	m_view->ResetCamera();
	Refresh();
}