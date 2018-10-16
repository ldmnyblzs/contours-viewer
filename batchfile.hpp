#ifndef BATCH_FILE_HPP
#define BATCH_FILE_HPP

#include "computable.hpp"
#include "batch.hpp"

class ParametersView;
class FilesView;
class wxAuiNotebookEvent;

wxDECLARE_EVENT(wxEVT_BATCHFILE_LOADED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_BATCHFILE_COMPUTED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_BATCHFILE_STATUS_CHANGED, wxThreadEvent);

class BatchFile final : public wxWindow, public wxThreadHelper, public Computable {
  wxString m_fileName;
  std::unique_ptr<Batch> m_batch;

  ParametersView *m_parameters_view;
  FilesView *m_files_view;
  wxCriticalSection m_model_cs;
  
  wxMutex m_run_mutex;
  wxCondition m_run_condition;

  std::atomic_bool m_cancelled = false;
  
  void Initialize();
  wxThread::ExitCode Entry() final;
  void OnLoaded(wxThreadEvent &event);
  void OnComputed(wxThreadEvent &event);
  void OnStatusChanged(wxThreadEvent &event);
  bool Cancelled() const;
  void StatusChanged(std::size_t index, Status status) const;
public:
  template <typename... Args>
  explicit BatchFile(const wxString &fileName, Args&&... args) :
    wxWindow(std::forward<Args>(args)...),
    m_fileName(fileName),
    m_run_condition(m_run_mutex) {
    Initialize();
  }
  void Compute() final;
  void Cancel() final;
  void Save() final;
  bool Destroy() final;
  virtual ~BatchFile() {
  }
};

#endif // BATCH_FILE_HPP
