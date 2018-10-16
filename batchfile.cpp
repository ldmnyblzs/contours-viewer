#include "batchfile.hpp"
#include "parametersview.hpp"
#include "filesview.hpp"

wxDEFINE_EVENT(wxEVT_BATCHFILE_LOADED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_BATCHFILE_COMPUTED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_BATCHFILE_STATUS_CHANGED, wxThreadEvent);

void BatchFile::Initialize() {
  auto sizer = new wxBoxSizer(wxVERTICAL);
  m_parameters_view = new ParametersView(this, wxID_ANY);
  m_files_view = new FilesView(this, wxID_ANY);
  sizer->Add(m_parameters_view, wxSizerFlags(0));
  sizer->Add(m_files_view, wxSizerFlags(1).Expand());
  SetSizerAndFit(sizer);

  Bind(wxEVT_BATCHFILE_LOADED, &BatchFile::OnLoaded, this);
  Bind(wxEVT_BATCHFILE_COMPUTED, &BatchFile::OnComputed, this);
  Bind(wxEVT_BATCHFILE_STATUS_CHANGED, &BatchFile::OnStatusChanged, this);

  if (CreateThread(wxTHREAD_JOINABLE) == wxTHREAD_NO_ERROR)
    GetThread()->Run();
}

void BatchFile::Compute() {
  wxMutexLocker lock(m_run_mutex);
  m_run_condition.Signal();
  
  SetRunning();
}

void BatchFile::Cancel() {
  m_cancelled = true;
  SetRunning(false);
}

void BatchFile::Save() {
	wxFileDialog dialog(this, "Save", "", "", "Batch file (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	
	if (dialog.ShowModal() == wxID_CANCEL)
		return;
	
	wxCriticalSectionLocker lock(m_model_cs);
	m_batch->save(dialog.GetPath().ToStdString(wxConvUTF8));
}

bool BatchFile::Cancelled() const {
	return m_cancelled;
}
void BatchFile::StatusChanged(std::size_t index, Status status) const {
	auto event = new wxThreadEvent(wxEVT_BATCHFILE_STATUS_CHANGED);
	event->SetPayload(std::make_pair(index, status));
	wxQueueEvent(GetEventHandler(), event);
}

wxThread::ExitCode BatchFile::Entry() {
  wxMutexLocker mutex_lock(m_run_mutex);
  {
    wxCriticalSectionLocker lock(m_model_cs);
    m_batch = std::make_unique<Batch>(m_fileName.ToStdString(wxConvUTF8),
	std::bind(&BatchFile::Cancelled, this),
	std::bind(&BatchFile::StatusChanged, this, std::placeholders::_1, std::placeholders::_2));
    wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_BATCHFILE_LOADED));
  }
  while (!GetThread()->TestDestroy()) {
    if (m_run_condition.WaitTimeout(100) == wxCOND_NO_ERROR) {
      wxCriticalSectionLocker lock(m_model_cs);
      if (m_batch->run())
	wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_BATCHFILE_COMPUTED));
    }
  }
  return wxThread::ExitCode(0);
}

void BatchFile::OnLoaded(wxThreadEvent & WXUNUSED(event)) {
  {
    wxCriticalSectionLocker lock(m_model_cs);
    m_parameters_view->Update(m_batch->parameters());
    m_files_view->UpdateFiles(m_batch->files());
	m_files_view->UpdateStatuses(m_batch->statuses());
  }
  GetSizer()->Layout();
  SetRunning(false);
}

void BatchFile::OnComputed(wxThreadEvent & WXUNUSED(event)) {
  {
    wxCriticalSectionLocker lock(m_model_cs);
    m_files_view->UpdateStatuses(m_batch->statuses());
  }
  GetSizer()->Layout();
  SetRunning(false);
}

void BatchFile::OnStatusChanged(wxThreadEvent &event) {
	const auto status = event.GetPayload<std::pair<std::size_t, Status> >();
	m_files_view->UpdateStatus(status.first, status.second);
}

bool BatchFile::Destroy() {
  Cancel();
  GetThread()->Delete(nullptr, wxTHREAD_WAIT_BLOCK);
  return wxWindow::Destroy();
}
