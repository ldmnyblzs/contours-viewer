#ifndef SINGLE_FILE_HPP
#define SINGLE_FILE_HPP

#include "computable.hpp"
#include "computation.hpp"

class InputForm;
class MeshView;
class GraphView;
class OutputView;
class wxAuiNotebookEvent;

wxDECLARE_EVENT(wxEVT_SINGLEFILE_LOADED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_SINGLEFILE_COMPUTED, wxThreadEvent);

class SingleFile final : public wxWindow, public wxThreadHelper, public Computable {
  wxAuiManager m_manager;
  InputForm *m_input_form;
  MeshView *m_mesh_view;
  GraphView *m_graph_view;
  OutputView *m_output_view;

  wxString m_fileName;
  Parameters m_parameters;
  std::unique_ptr<Computation> m_computation;
  wxCriticalSection m_model_cs;
	
  wxMutex m_run_mutex;
  wxCondition m_run_condition;

  std::atomic_bool m_cancelled = false;
	
  void Initialize();
  wxThread::ExitCode Entry() final;
  void OnLoaded(wxThreadEvent &event);
  void OnComputed(wxThreadEvent &event);
  bool Cancelled() const;
public:
  template <typename... Args>
  explicit SingleFile(const wxString &fileName, Args&&... args) :
    wxWindow(std::forward<Args>(args)...),
    m_fileName(fileName),
    m_run_condition(m_run_mutex) {
    Initialize();
  }
  void Compute() final;
  void Cancel() final;
  void Save() final;
  bool Destroy() final;
  virtual ~SingleFile() {}
};

#endif // SINGLE_FILE_HPP
