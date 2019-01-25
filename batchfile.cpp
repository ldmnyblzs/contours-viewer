#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/filedlg.h>
#include <wx/sizer.h>
#endif

#include "batchfile.hpp"
#include "filesview.hpp"
#include "model/execute.hpp"
#include "model/ratios.hpp"
#include "parametersview.hpp"
#include <tbb/parallel_for_each.h>

wxDEFINE_EVENT(wxEVT_BATCHFILE_LOADED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_BATCHFILE_COMPUTED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_BATCHFILE_STATUS_CHANGED, wxThreadEvent);

void BatchFile::Initialize() {
  using namespace std::string_literals;
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

  m_queue.Post(std::make_pair(LOAD, ""s));
}

void BatchFile::Compute() {
  using namespace std::string_literals;
  m_queue.Post(std::make_pair(RUN, ""s));
  SetRunning();
}

void BatchFile::Cancel() {
  m_cancelled = true;
  SetRunning(false);
}

void BatchFile::Save() {
  wxFileDialog dialog(this, "Save", "", "", "Batch file (*.csv)|*.csv",
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (dialog.ShowModal() == wxID_CANCEL)
    return;

  m_queue.Post(std::make_pair(SAVE, dialog.GetPath().ToStdString(wxConvUTF8)));
}

bool BatchFile::Cancelled() const { return m_cancelled; }

wxThread::ExitCode BatchFile::Entry() {
  using boost::irange;
  using boost::adaptors::indexed;
  using boost::filesystem::path;

  Parameters parameters;
  std::vector<std::string> files;

  const auto directory =
      path(m_fileName, std::codecvt_utf8<wchar_t>()).parent_path();

  auto set_status = [this](std::size_t index, Status status) {
    auto event = new wxThreadEvent(wxEVT_BATCHFILE_STATUS_CHANGED);
    event->SetPayload(std::make_pair(index, status));
    wxQueueEvent(GetEventHandler(), event);
  };

  std::pair<Event, std::string> event;
  while (m_queue.Receive(event) == wxMSGQUEUE_NO_ERROR) {
    switch (event.first) {
    case LOAD:
      load_batch_file(m_fileName, parameters, files);
      m_parameters_view->Update(parameters);
      m_files_view->UpdateFiles(files);
      m_results.reserve(files.size());
      for (const auto &file : files)
        m_results[file];
      wxQueueEvent(GetEventHandler(),
                   new wxThreadEvent(wxEVT_BATCHFILE_LOADED));
      break;
    case RUN:
      for (const auto &index : irange(0ul, files.size()))
        set_status(index, STATUS_WAITING);
      tbb::parallel_for_each(files | indexed(), [&](const auto &file) {
        set_status(file.index(), STATUS_RUNNING);
        try {
          Mesh mesh;
          load_mesh(file.value(), mesh, directory);
          const auto properties = mesh_properties(mesh);
	  m_results.at(file.value()).area = properties[0];
	  m_results.at(file.value()).volume = properties[1];
	  m_results.at(file.value()).a = properties[2];
	  m_results.at(file.value()).b = properties[3];
	  m_results.at(file.value()).c = properties[4];
	  m_results.at(file.value()).proj_circumference = properties[5];
	  m_results.at(file.value()).proj_area = properties[6];
	  m_results.at(file.value()).ratios = calculate_ratios(properties);
          execute(file.value(), mesh, properties[0], properties[1], parameters,
                  *this);
          set_status(file.index(), STATUS_OK);
        } catch (const std::exception& e) {
	  std::cout << e.what() << std::endl;
          set_status(file.index(), STATUS_ERROR);
        } catch (...) {
	  std::cout << "wtf" << std::endl;
          set_status(file.index(), STATUS_ERROR);
        }
      });
      wxQueueEvent(GetEventHandler(),
                   new wxThreadEvent(wxEVT_BATCHFILE_COMPUTED));
      break;
    case SAVE:
      save_batch_file(m_fileName, event.second, m_results);
      break;
    case EXIT:
      return wxThread::ExitCode(0);
    }
  }
  return wxThread::ExitCode(0);
}

void BatchFile::OnLoaded(wxThreadEvent &WXUNUSED(event)) {
  m_parameters_view->Swap();
  m_files_view->SwapFiles();
  GetSizer()->Layout();
  SetRunning(false);
}

void BatchFile::OnComputed(wxThreadEvent &WXUNUSED(event)) {
  GetSizer()->Layout();
  SetRunning(false);
}

void BatchFile::OnStatusChanged(wxThreadEvent &event) {
  const auto status = event.GetPayload<std::pair<std::size_t, Status>>();
  m_files_view->UpdateStatus(status.first, status.second);
}

bool BatchFile::Destroy() {
  using namespace std::string_literals;
  m_queue.Post(std::make_pair(EXIT, ""s));
  GetThread()->Delete(nullptr, wxTHREAD_WAIT_BLOCK);
  return wxWindow::Destroy();
}

void BatchFile::su(const std::string &filename,
                   const CenterSphereGenerator &center_sphere, int level_count,
                   double area_ratio, Aggregation aggregation,
                   std::pair<float, float> &su) {
  const ParameterSignature signature(center_sphere.ratio, center_sphere.count,
                                     level_count, area_ratio, aggregation);
  m_results.at(filename).surm[signature].stable = su.first;
  m_results.at(filename).surm[signature].unstable = su.second;
}

void BatchFile::reeb(const std::string &filename,
                     const CenterSphereGenerator &center_sphere,
                     int level_count, double area_ratio,
                     Aggregation aggregation, const Graph &graph,
                     const std::string &code) {
  const ParameterSignature signature(center_sphere.ratio, center_sphere.count,
                                     level_count, area_ratio, aggregation);
  m_results.at(filename).surm[signature].reeb = code;
}

void BatchFile::morse(const std::string &filename,
                      const CenterSphereGenerator &center_sphere,
                      int level_count, double area_ratio,
                      Aggregation aggregation, const std::string &code) {
  const ParameterSignature signature(center_sphere.ratio, center_sphere.count,
                                     level_count, area_ratio, aggregation);
  m_results.at(filename).surm[signature].morse = code;
}
