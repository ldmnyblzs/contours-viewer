#include "singlefile.hpp"
#include "inputform.hpp"
#include "meshview.hpp"
#include "graphview.hpp"
#include "outputview.hpp"

#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/stdstream.h>

wxDEFINE_EVENT(wxEVT_SINGLEFILE_LOADED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SINGLEFILE_COMPUTED, wxThreadEvent);

void SingleFile::Initialize() {
  m_manager.SetManagedWindow(this);
  m_input_form = new InputForm(this);
  
  wxGLAttributes dispAttrs;
  dispAttrs.PlatformDefaults().DoubleBuffer().RGBA().BufferSize(32).MinRGBA(8, 8, 8, 8).Depth(16).SampleBuffers(0).Stencil(0).EndList();
  m_mesh_view = new MeshView(this, dispAttrs);

  m_graph_view = new GraphView(this, dispAttrs);
  
  m_output_view = new OutputView(m_fileName, this, wxID_ANY);

  m_manager.InsertPane(m_input_form,
		       wxAuiPaneInfo()
		       .Caption("Input")
		       .MinSize(m_input_form->GetMinSize())
		       .Fixed()
		       .Left());
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
}

void SingleFile::Compute() {
  m_parameters = m_input_form->GetParameters();
  m_output_view->UpdateParameters(m_parameters);

  m_manager.GetPane(m_output_view).MinSize(m_output_view->GetBestSize());
  m_manager.Update();
  
  wxMutexLocker mutex_lock(m_run_mutex);
  m_run_condition.Signal();
  
  SetRunning();
}

void SingleFile::Cancel() {
  m_cancelled = true;
  SetRunning(false);
}

void SingleFile::Save() {
	wxFileDialog dialog(this, "Save", "", "", "PNG image (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	
	if (dialog.ShowModal() == wxID_CANCEL)
		return;
	
	const auto output = m_output_view->Screenshot();
	const auto mesh = m_mesh_view->Screenshot();
	const auto graph = m_graph_view->Screenshot();
	
	const auto width = output.Width() + mesh.Width() + graph.Width();
	const auto height = std::max(std::max(output.Height(), mesh.Height()), graph.Height());
  
	GD::Image image(width, height, true);
	image.Fill(0, 0, GD::TrueColor(255, 255, 255).Int());
	image.Copy(output, 0, 0, 0, 0, output.Width(), output.Height());
	image.Copy(mesh, output.Width(), 0, 0, 0, mesh.Width(), mesh.Height());
	image.Copy(graph, output.Width() + mesh.Width(), 0, 0, 0, graph.Width(), graph.Height());
	wxFileOutputStream file_stream(dialog.GetPath());
	wxStdOutputStream std_stream(file_stream);
	image.Png(std_stream, -1);
}

bool SingleFile::Cancelled() const {
  return m_cancelled;
}

wxThread::ExitCode SingleFile::Entry() {
  wxMutexLocker mutex_lock(m_run_mutex);

  {
    wxCriticalSectionLocker lock(m_model_cs);
    wxFileStream fileStream(m_fileName);
    wxStdInputStream stream(fileStream);
    Mesh mesh(stream);
    mesh.measure();
    m_computation = std::make_unique<Computation>(std::move(mesh), std::bind(&SingleFile::Cancelled, this));
    wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_SINGLEFILE_LOADED));
  }

  while (!GetThread()->TestDestroy()) {
    if (m_run_condition.WaitTimeout(100) == wxCOND_NO_ERROR) {
      wxCriticalSectionLocker lock(m_model_cs);
      m_cancelled = false;
      if (m_computation->run(m_parameters))
		  wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_SINGLEFILE_COMPUTED));
    }
  }
  return wxThread::ExitCode(0);
}

void SingleFile::OnLoaded(wxThreadEvent & WXUNUSED(event)) {
  wxCriticalSectionLocker lock(m_model_cs);
  m_mesh_view->UpdateMesh(m_computation->mesh());
  m_output_view->UpdateMeshData(m_computation->mesh());

  m_manager.GetPane(m_output_view).MinSize(m_output_view->GetBestSize());
  m_manager.Update();
  
  SetRunning(false);
}

void SingleFile::OnComputed(wxThreadEvent & WXUNUSED(event)) {
  wxCriticalSectionLocker lock(m_model_cs);
  m_mesh_view->UpdateTexture(m_computation->level_graphs()[0]);
  m_graph_view->UpdateGraph(m_computation->reeb_graphs()[0]);
  m_output_view->UpdateSU(m_computation->su()[0]);
  
  m_manager.GetPane(m_output_view).MinSize(m_output_view->GetBestSize());
  m_manager.Update();
  
  SetRunning(false);
}

bool SingleFile::Destroy() {
  Cancel();
  GetThread()->Delete(nullptr, wxTHREAD_WAIT_BLOCK);
  m_manager.UnInit();
  return wxWindow::Destroy();
}
