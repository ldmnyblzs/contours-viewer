#include "meshview.hpp"

#include <boost/math/constants/constants.hpp>
#include <vtkLookupTable.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkAppendPolyData.h>
#include <vtkArcSource.h>
#include <vtkRegularPolygonSource.h>
#include <vtkCellData.h>
#include <vtkUnsignedCharArray.h>
#include <vtkProperty.h>
#include <vtkLightCollection.h>

#include "mesh.hpp"
#include "levelgraph.hpp"

void MeshView::Initialize() {
  vtkNew<vtkActor> meshActor;
  meshActor->SetMapper(m_meshMapper.GetPointer());
  meshActor->GetProperty()->BackfaceCullingOn();
  meshActor->GetProperty()->SetColor(0.8, 0.8, 0.8);
  meshActor->GetProperty()->LightingOff();
  meshActor->GetProperty()->SetInterpolationToFlat();
  meshActor->GetProperty()->ShadingOff();
  meshActor->GetProperty()->EdgeVisibilityOn();
  meshActor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
  
  /*vtkNew<vtkLookupTable> contourColors;
  contourColors->SetNumberOfTableValues(3);
  contourColors->Build();
  
  contourColors->SetTableValue(LevelGraph::SIMPLE, 0.0, 0.0, 1.0);
  contourColors->SetTableValue(LevelGraph::STABLE, 0.0, 1.0, 0.0);
  contourColors->SetTableValue(LevelGraph::UNSTABLE, 1.0, 0.0, 0.0);*/
  
  /*m_contourMapper->SetScalarRange(LevelGraph::SIMPLE, LevelGraph::UNSTABLE);
  m_contourMapper->SetColorModeToMapScalars();
  m_contourMapper->SetLookupTable(contourColors.GetPointer());*/
  
	std::array<vtkNew<vtkActor>, 3> contourActors;
	for (std::size_t i = 0; i < 3; ++i) {
		contourActors[i]->SetMapper(m_contourMappers[i].GetPointer());
		contourActors[i]->SetScale(1.001);
		contourActors[i]->GetProperty()->BackfaceCullingOn();
		contourActors[i]->GetProperty()->LightingOff();
		contourActors[i]->GetProperty()->SetInterpolationToFlat();
		contourActors[i]->GetProperty()->ShadingOff();
	}
  
  contourActors[0]->GetProperty()->SetColor(0.0, 0.0, 1.0);
  contourActors[1]->GetProperty()->SetColor(0.0, 1.0, 0.0);
  contourActors[2]->GetProperty()->SetColor(1.0, 0.0, 0.0);
  
  m_axesActor->SetConeResolution(20);
  m_axesActor->AxisLabelsOff();
  m_axesActor->GetXAxisTipProperty()->BackfaceCullingOn();
  m_axesActor->GetXAxisTipProperty()->LightingOff();
  m_axesActor->GetXAxisTipProperty()->SetInterpolationToFlat();
  m_axesActor->GetXAxisTipProperty()->ShadingOff();
  m_axesActor->GetYAxisTipProperty()->BackfaceCullingOn();
  m_axesActor->GetYAxisTipProperty()->LightingOff();
  m_axesActor->GetYAxisTipProperty()->SetInterpolationToFlat();
  m_axesActor->GetYAxisTipProperty()->ShadingOff();
  m_axesActor->GetZAxisTipProperty()->BackfaceCullingOn();
  m_axesActor->GetZAxisTipProperty()->LightingOff();
  m_axesActor->GetZAxisTipProperty()->SetInterpolationToFlat();
  m_axesActor->GetZAxisTipProperty()->ShadingOff();
  m_axesActor->GetXAxisShaftProperty()->BackfaceCullingOn();
  m_axesActor->GetXAxisShaftProperty()->LightingOff();
  m_axesActor->GetXAxisShaftProperty()->SetInterpolationToFlat();
  m_axesActor->GetXAxisShaftProperty()->ShadingOff();
  m_axesActor->GetXAxisShaftProperty()->SetLineWidth(2.0);
  m_axesActor->GetYAxisShaftProperty()->BackfaceCullingOn();
  m_axesActor->GetYAxisShaftProperty()->LightingOff();
  m_axesActor->GetYAxisShaftProperty()->SetInterpolationToFlat();
  m_axesActor->GetYAxisShaftProperty()->ShadingOff();
  m_axesActor->GetYAxisShaftProperty()->SetLineWidth(2.0);
  m_axesActor->GetZAxisShaftProperty()->BackfaceCullingOn();
  m_axesActor->GetZAxisShaftProperty()->LightingOff();
  m_axesActor->GetZAxisShaftProperty()->SetInterpolationToFlat();
  m_axesActor->GetZAxisShaftProperty()->ShadingOff();
  m_axesActor->GetZAxisShaftProperty()->SetLineWidth(2.0);
  
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderer->AddActor(meshActor.GetPointer());
  for (std::size_t i = 0; i < 3; ++i)
	renderer->AddActor(contourActors[i].GetPointer());
  renderer->AddActor(m_axesActor.GetPointer());
  
  vtkNew<vtkGenericOpenGLRenderWindow> window;
  window->AddRenderer(renderer.GetPointer());
  
  vtkNew<vtkGenericRenderWindowInteractor> renderWindowInteractor;
  vtkNew<vtkInteractorStyleTrackballCamera> interactorStyle;
  renderWindowInteractor->SetRenderWindow(window.GetPointer());
  renderWindowInteractor->SetInteractorStyle(interactorStyle.GetPointer());
  
  SetRenderWindowInteractor(renderWindowInteractor.GetPointer());
  renderWindowInteractor->Enable();
}

void MeshView::UpdateMesh(const Mesh &mesh) {
  vtkNew<vtkPoints> points;
  for (auto vertex : mesh.vertices()) {
    const auto point = vertex.point();
    points->InsertNextPoint(CGAL::to_double(point.x()),
			    CGAL::to_double(point.y()),
			    CGAL::to_double(point.z()));
  }
  
  vtkNew<vtkCellArray> faces;
  for (auto face : mesh.faces()) {
    faces->InsertNextCell(3);
    for (auto vertex : face.vertices_around())
      faces->InsertCellPoint(vertex.handle());
  }
  
  vtkNew<vtkPolyData> data;
  data->SetPoints(points.GetPointer());
  data->SetPolys(faces.GetPointer());
  
  m_meshMapper->SetInputData(data.GetPointer());

  const auto x_length = mesh.xmax() * 1.5;
  const auto y_length = mesh.ymax() * 1.5;
  const auto z_length = mesh.zmax() * 1.5;
  const auto tip_length = mesh.a() * 0.1;
  const auto x_shaft_length = x_length - tip_length;
  const auto y_shaft_length = y_length - tip_length;
  const auto z_shaft_length = z_length - tip_length;
  m_axesActor->SetTotalLength(x_length, y_length, z_length);
  m_axesActor->SetNormalizedTipLength(tip_length / x_length, tip_length / y_length, tip_length / z_length);
  m_axesActor->SetNormalizedShaftLength(x_shaft_length / x_length, y_shaft_length / y_length, z_shaft_length / z_length);
  
  ResetCamera();
}

void MeshView::UpdateTexture(const LevelGraph &graph) {
	using namespace boost::math::double_constants;
	
	std::array<int, 3> counts = {0, 0, 0};
	for (const auto &edge : graph.edges())
		counts[graph.type(edge)] += graph.arcs(edge).size();
	
	std::array<vtkNew<vtkAppendPolyData>, 3> data;
	for (std::size_t i = 0; i < 3; ++i) {
		data[i]->UserManagedInputsOn();
		data[i]->SetNumberOfInputs(counts[i]);
	}
	
	std::array<int, 3> index = {0, 0, 0};
	for (const auto &edge : graph.edges()) {
		const auto type = graph.type(edge);
		for (const auto &arc : graph.arcs(edge)) {
			const auto center = arc.spherical_circle.center();
			const auto resolution = static_cast<int>(std::ceil(arc.approximate_length())) * 5;
			
			if (arc.source) {
				const auto to_source = arc.to_source();
				const auto normal = arc.spherical_circle.supporting_plane().orthogonal_vector();
				vtkNew<vtkArcSource> arcSource;
				arcSource->UseNormalAndAngleOn();
				arcSource->SetNormal(CGAL::to_double(normal.x()),
					CGAL::to_double(normal.y()),
					CGAL::to_double(normal.z()));
				arcSource->SetPolarVector(to_source.x(), to_source.y(), to_source.z());
				arcSource->SetAngle(arc.angle() / pi * 180.0);
				arcSource->SetCenter(CGAL::to_double(center.x()), CGAL::to_double(center.y()), CGAL::to_double(center.z()));
				/*arcSource->SetPoint1(source.x(), source.y(), source.z());
				arcSource->SetPoint2(target.x(), target.y(), target.z());*/
				arcSource->SetResolution(resolution);
				//arcSource->SetNegative(arc.angle() >= pi);
				data[type]->SetInputConnectionByNumber(index[type]++, arcSource->GetOutputPort());
			} else {
				const auto normal = arc.spherical_circle.supporting_plane().orthogonal_vector();
				vtkNew<vtkRegularPolygonSource> circleSource;
				circleSource->SetCenter(CGAL::to_double(center.x()), CGAL::to_double(center.y()), CGAL::to_double(center.z()));
				circleSource->SetRadius(arc.radius());
				circleSource->SetNormal(CGAL::to_double(normal.x()), CGAL::to_double(normal.y()), CGAL::to_double(normal.z()));
				circleSource->SetNumberOfSides(resolution);
				circleSource->GeneratePolygonOff();
				circleSource->GeneratePolylineOn();
				data[type]->SetInputConnectionByNumber(index[type]++, circleSource->GetOutputPort());
			}
		}
	}
	
	for (std::size_t i = 0; i < 3; ++i) {
		data[i]->Update();
		m_contourMappers[i]->SetInputData(data[i]->GetOutput());
	}
	
	Refresh();
	//ResetCamera();
}
