#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#endif

#include <wx/aui/aui.h>
#include <wx/wfstream.h>
#include <wx/stdstream.h>
#include <wx/grid.h>

#include "singlefile.hpp"
#include "inputform.hpp"
#ifdef VTK_FOUND
#include "meshview.hpp"
#include "graphview.hpp"
#endif //VTK_FOUND
#include "outputview.hpp"
#include "model/execute.hpp"
#include "model/ratios.hpp"

wxDEFINE_EVENT(wxEVT_SINGLEFILE_LOADED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SINGLEFILE_COMPUTED, wxThreadEvent);

void SingleFile::Initialize() {
  m_manager.SetManagedWindow(this);
  m_input_form = new InputForm(this);
  
#ifdef VTK_FOUND
  wxGLAttributes dispAttrs;
  dispAttrs.PlatformDefaults().DoubleBuffer().RGBA().BufferSize(32).MinRGBA(8, 8, 8, 8).Depth(16).SampleBuffers(0).Stencil(0).EndList();
  m_mesh_view = new MeshView(this, dispAttrs);
  m_graph_view = new GraphView(this, dispAttrs);
#endif //VTK_FOUND
  
  m_output_view = new OutputView(m_fileName, this, wxID_ANY);

  m_manager.InsertPane(m_input_form,
		       wxAuiPaneInfo()
		       .Caption("Input")
		       .MinSize(m_input_form->GetMinSize())
		       .Fixed()
		       .Left());
#ifdef VTK_FOUND
  m_manager.InsertPane(m_mesh_view,
		       wxAuiPaneInfo()
		       .Caption("Mesh")
		       .MaximizeButton()
		       .Center()
		       .Position(0));
  m_manager.InsertPane(m_graph_view,
		       wxAuiPaneInfo()
		       .Caption("Graph")
		       .MaximizeButton()
		       .Center()
		       .Position(1));
#endif //VTK_FOUND
  m_manager.InsertPane(m_output_view,
		       wxAuiPaneInfo()
		       .Caption("Output")
		       .MinSize(m_output_view->GetBestSize())
		       .Fixed()
		       .Right());
  m_manager.Update();
  
  Bind(wxEVT_SINGLEFILE_LOADED, &SingleFile::OnLoaded, this);
  Bind(wxEVT_SINGLEFILE_COMPUTED, &SingleFile::OnComputed, this);

  if (CreateThread(wxTHREAD_JOINABLE) == wxTHREAD_NO_ERROR)
    GetThread()->Run();

  m_queue.Post({LOAD, std::nullopt});
}

void SingleFile::Compute() {
  m_queue.Post({RUN, m_input_form->GetParameters()});
  SetRunning();
}

void SingleFile::Cancel() {
  m_cancelled = true;
  SetRunning(false);
}

void SingleFile::Save() {
  wxFileDialog dialog(this, "Save", "", "",
		      "Batch file (*.csv)|*.csv"
#ifdef GD_FOUND
		      "|PNG image (*.png)|*.png"
#endif //GD_FOUND
		      ,
		      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	
  if (dialog.ShowModal() == wxID_CANCEL)
    return;
  
  wxFileOutputStream file_stream(dialog.GetPath());
  wxStdOutputStream std_stream(file_stream);
  
  switch(dialog.GetFilterIndex()) {
  case 0:
    std_stream << "Function not implemented yet" << std::endl;
    break;
#ifdef GD_FOUND
  case 1: {
    const auto output = m_output_view->Screenshot();
#  ifdef VTK_FOUND
    const auto mesh = m_mesh_view->Screenshot();
    const auto graph = m_graph_view->Screenshot();
	
    const auto width = output.Width() + mesh.Width() + graph.Width();
    const auto height = std::max(std::max(output.Height(), mesh.Height()), graph.Height());

    GD::Image image(width, height, true);
    image.Fill(0, 0, GD::TrueColor(255, 255, 255).Int());
    image.Copy(output, 0, 0, 0, 0, output.Width(), output.Height());
    image.Copy(mesh, output.Width(), 0, 0, 0, mesh.Width(), mesh.Height());
    image.Copy(graph, output.Width() + mesh.Width(), 0, 0, 0, graph.Width(), graph.Height());
    image.Png(std_stream, -1);
#  else
    output.Png(std_stream, -1);
#  endif //VTK_FOUND
    break;}
#endif //GD_FOUND
  }
}

bool SingleFile::Cancelled() const {
  return m_cancelled;
}

wxThread::ExitCode SingleFile::Entry() {
  Mesh mesh;
  double area = 0.0, volume = 0.0;
  std::pair<Event, std::optional<Parameters> > event;
  while (m_queue.Receive(event) == wxMSGQUEUE_NO_ERROR) {
    switch (event.first) {
    case LOAD: {
      load_mesh(m_fileName, mesh);
      const auto properties = mesh_properties(mesh);
      const auto ratios = calculate_ratios(properties);
      area = properties[0];
      volume = properties[1];
      m_output_view->UpdateMeshData(properties);
      m_output_view->UpdateRatios(ratios);
#ifdef VTK_FOUND
      m_mesh_view->PrepareMesh(mesh);
#endif //VTK_FOUND
      wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_SINGLEFILE_LOADED));
      break;
    }
    case RUN:
      execute(m_fileName, mesh, area, volume, *(event.second), *this);
      wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_SINGLEFILE_COMPUTED));
      break;
    case EXIT:
      return wxThread::ExitCode(0);
    }
  }
  return wxThread::ExitCode(0);
}

void SingleFile::OnLoaded(wxThreadEvent & WXUNUSED(event)) {  
#ifdef VTK_FOUND
  m_mesh_view->SwapMesh();
#endif //VTK_FOUND

  m_output_view->Swap();

  m_manager.GetPane(m_output_view).MinSize(m_output_view->GetBestSize());
  m_manager.Update();
  
  SetRunning(false);
}

void SingleFile::OnComputed(wxThreadEvent & WXUNUSED(event)) {
#ifdef VTK_FOUND
  m_mesh_view->SwapArcs();
  m_graph_view->Swap();
#endif //VTK_FOUND
  m_output_view->Swap();
  
  m_manager.GetPane(m_output_view).MinSize(m_output_view->GetBestSize());
  m_manager.Update();
  
  SetRunning(false);
}

bool SingleFile::Destroy() {
  Cancel();
  m_queue.Post({EXIT, std::nullopt});
  GetThread()->Delete(nullptr, wxTHREAD_WAIT_BLOCK);
  m_manager.UnInit();
  return wxWindow::Destroy();
}

/*void SingleFile::mesh_properties(const std::string &filename,
				 double area,
				 double volume) {
  m_output_view->UpdateMeshData(area, volume);
  }*/
void SingleFile::level_graph(const std::string &filename,
			     const CenterSphereGenerator &center_sphere,
			     int level_count,
			     const Graph &graph,
			     const std::vector<GraphEdge> &stable_edges,
			     const std::vector<GraphEdge> &unstable_edges){
#ifdef VTK_FOUND
  m_mesh_view->PrepareArcs(graph, stable_edges, unstable_edges);
#endif //VTK_FOUND
}
void SingleFile::su(const std::string &filename,
		    const CenterSphereGenerator &center_sphere,
		    int level_count,
		    double area_ratio,
		    Aggregation aggregation,
		    std::pair<float, float> &su) {
  m_output_view->UpdateParameters(center_sphere.offset,
				  level_count,
				  area_ratio);
  m_output_view->UpdateSU(static_cast<int>(su.first), static_cast<int>(su.second));
}
void SingleFile::reeb(const std::string &filename,
		      const CenterSphereGenerator &center_sphere,
		      int level_count,
		      double area_ratio,
		      Aggregation aggregation,
		      const Graph &graph,
		      const std::string &code) {
#ifdef VTK_FOUND
  m_graph_view->UpdateGraph(graph);
#endif //VTK_FOUND
  m_output_view->UpdateReeb(code);
}
void SingleFile::morse(const std::string &filename,
		       const CenterSphereGenerator &center_sphere,
		       int level_count,
		       double area_ratio,
		       Aggregation aggregation,
		       const std::string &code) {
  m_output_view->UpdateMorse(code);
}
