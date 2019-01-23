#include "meshview.hpp"

#include <boost/math/constants/constants.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/binary_search.hpp>
#include <boost/range/irange.hpp>
#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkArcSource.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLightCollection.h>
#include <vtkLookupTable.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkUnsignedCharArray.h>

void MeshView::Initialize() {
  vtkNew<vtkPolyDataMapper> meshMapper;
  meshMapper->SetInputData(m_displayedMesh.GetPointer());

  vtkNew<vtkActor> meshActor;
  meshActor->SetMapper(meshMapper.GetPointer());
  meshActor->GetProperty()->BackfaceCullingOn();
  meshActor->GetProperty()->SetColor(0.8, 0.8, 0.8);
  meshActor->GetProperty()->LightingOff();
  meshActor->GetProperty()->SetInterpolationToFlat();
  meshActor->GetProperty()->ShadingOff();
  meshActor->GetProperty()->EdgeVisibilityOn();
  meshActor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);

  vtkNew<vtkLookupTable> contourColors;
  contourColors->SetNumberOfTableValues(3);
  contourColors->Build();

  contourColors->SetTableValue(0, 0.0, 0.0, 1.0);
  contourColors->SetTableValue(1, 0.0, 1.0, 0.0);
  contourColors->SetTableValue(2, 1.0, 0.0, 0.0);

  vtkNew<vtkPolyDataMapper> contourMapper;
  contourMapper->SetInputData(m_displayedArcs.GetPointer());
  contourMapper->SetScalarRange(0, 2);
  contourMapper->SetColorModeToMapScalars();
  contourMapper->SetLookupTable(contourColors.GetPointer());

  vtkNew<vtkActor> contourActor;
  contourActor->SetMapper(contourMapper.GetPointer());

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
  renderer->AddActor(contourActor.GetPointer());
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

void MeshView::PrepareMesh(const Mesh &mesh) {
  wxCriticalSectionLocker lock(m_critical_section);
  
  vtkNew<vtkPoints> points;
  for (const auto vertex : mesh.vertices()) {
    const auto point = mesh.point(vertex);
    points->InsertNextPoint(CGAL::to_double(point.x()),
                            CGAL::to_double(point.y()),
                            CGAL::to_double(point.z()));
  }
  vtkNew<vtkCellArray> faces;
  for (auto face : mesh.faces()) {
    faces->InsertNextCell(3);
    for (auto vertex : CGAL::vertices_around_face(mesh.halfedge(face), mesh))
      faces->InsertCellPoint(get(boost::vertex_index, mesh, vertex));
  }

  m_preparedMesh->SetPoints(points.GetPointer());
  m_preparedMesh->SetPolys(faces.GetPointer());
}

void MeshView::PrepareArcs(const Graph &graph,
                           std::vector<GraphEdge> stable_edges,
                           std::vector<GraphEdge> unstable_edges) {
  wxCriticalSectionLocker lock(m_critical_section);
  
  using namespace boost;

  sort(stable_edges);
  sort(unstable_edges);
  
  vtkNew<vtkAppendPolyData> data;
  for (const auto &edge : make_iterator_range(edges(graph))) {
    for (const auto &arc : graph[edge].arcs) {
      vtkNew<vtkArcSource> arcSource;
      arcSource->UseNormalAndAngleOn();
      arcSource->SetNormal(arc.normal.x(), arc.normal.y(), arc.normal.z());
      const auto to_source = arc.source - arc.center;
      const auto to_target = arc.target - arc.center;
      arcSource->SetPolarVector(to_source.x(), to_source.y(), to_source.z());
      const auto arc_angle = atan2(-scalar_product(arc.normal, cross_product(to_source, to_target)), -scalar_product(to_source, to_target)) + M_PI;
      arcSource->SetAngle(arc_angle / M_PI * 180.0);
      arcSource->SetCenter(arc.center.x(), arc.center.y(), arc.center.z());
      const int resolution = ceil(sqrt(to_source.squared_length()) * arc_angle) * 5;
      arcSource->SetResolution(resolution);
      arcSource->Update();

      vtkNew<vtkIntArray> color;
      const int value = binary_search(stable_edges, edge) ? 1 : (binary_search(unstable_edges, edge) ? 2 : 0);
      for (const auto i : irange(0, resolution))
	color->InsertNextValue(value);
      
      arcSource->GetOutput()->GetCellData()->SetScalars(color.GetPointer());
      data->AddInputData(arcSource->GetOutput());
    }
  }
  data->Update();
  m_preparedArcs->ShallowCopy(data->GetOutput());
}

void MeshView::SwapMesh() {
  wxCriticalSectionLocker lock(m_critical_section);
  m_displayedMesh->ShallowCopy(m_preparedMesh.GetPointer());
  ResetCamera();
}

void MeshView::SwapArcs() {
  wxCriticalSectionLocker lock(m_critical_section);
  m_displayedArcs->ShallowCopy(m_preparedArcs.GetPointer());
  Refresh();
}
