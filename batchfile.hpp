#ifndef BATCH_FILE_HPP
#define BATCH_FILE_HPP

#include <atomic>
#include <wx/msgqueue.h>
#include "computable.hpp"
#include "model/primitives.hpp"
#include "model/batch.hpp"

class ParametersView;
class FilesView;
class wxAuiNotebookEvent;

wxDECLARE_EVENT(wxEVT_BATCHFILE_LOADED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_BATCHFILE_STATUS_CHANGED, wxThreadEvent);

class BatchFile final : public wxWindow, public wxThreadHelper, public Computable {
  const std::string m_fileName;

  // accessed from background thread only
  Results m_results;

  ParametersView *m_parameters_view;
  FilesView *m_files_view;
  
  enum Event {
    LOAD,
    RUN,
    SAVE,
    EXIT
  };
  wxMessageQueue<std::pair<Event, std::string> > m_queue;

  std::atomic_bool m_cancelled = false;
  
  void Initialize();
  wxThread::ExitCode Entry() final;
  void OnLoaded(wxThreadEvent &event);
  void OnStatusChanged(wxThreadEvent &event);
  bool Cancelled() const;
public:
  template <typename... Args>
  explicit BatchFile(const wxString &fileName, Args&&... args) :
    wxWindow(std::forward<Args>(args)...),
    m_fileName(fileName) {
    Initialize();
  }
  void Compute() final;
  void Cancel() final;
  void Save() final;
  bool Destroy() final;
  virtual ~BatchFile() {
  }

  // Saver concept
  /*void mesh_properties(const std::string &filename,
		       double area,
		       double volume);*/
  void level_graph(const std::string &filename,
		   const CenterSphereGenerator &center_sphere,
		   int level_count,
		   const Graph &graph,
		   const std::vector<GraphEdge> &stable_edges,
		   const std::vector<GraphEdge> &unstable_edges) {};
  void su(const std::string &filename,
	  const CenterSphereGenerator &center_sphere,
	  int level_count,
	  double area_ratio,
	  Aggregation aggregation,
	  std::pair<float, float> &su);
  void reeb(const std::string &filename,
	    const CenterSphereGenerator &center_sphere,
	    int level_count,
	    double area_ratio,
	    Aggregation aggregation,
	    const Graph &graph,
	    const std::string &code);
  void morse(const std::string &filename,
	     const CenterSphereGenerator &center_sphere,
	     int level_count,
	     double area_ratio,
	     Aggregation aggregation,
	     const std::string &code);
};

#endif // BATCH_FILE_HPP
