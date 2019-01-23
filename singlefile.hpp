#ifndef SINGLE_FILE_HPP
#define SINGLE_FILE_HPP

#include <wx/string.h>
#include <wx/strconv.h>
#include <wx/msgqueue.h>
#include "computable.hpp"
#include "model/parameters.hpp"

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

  const std::string m_fileName;
  //Parameters m_parameters;

  enum Event {
    LOAD,
    RUN,
    EXIT
  };
  wxMessageQueue<std::pair<Event, std::optional<Parameters> > > m_queue;

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
    m_fileName(fileName.ToStdString(wxConvUTF8)) {
    Initialize();
  }
  void Compute() final;
  void Cancel() final;
  void Save() final;
  bool Destroy() final;
  virtual ~SingleFile() {}

  // Saver concept
  /*void mesh_properties(const std::string &filename,
		       double area,
		       double volume);*/
  void level_graph(const std::string &filename,
		   const CenterSphereGenerator &center_sphere,
		   int level_count,
		   const Graph &graph,
		   const std::vector<GraphEdge> &stable_edges,
		   const std::vector<GraphEdge> &unstable_edges);
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

#endif // SINGLE_FILE_HPP
