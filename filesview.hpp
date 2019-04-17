#ifndef FILES_VIEW_HPP
#define FILES_VIEW_HPP

#include <wx/grid.h>
#include <wx/thread.h>
#include <vector>

enum Status {
  STATUS_WAITING,
  STATUS_RUNNING,
  STATUS_OK,
  STATUS_ERROR
};

class FilesView final : public wxGrid {
  wxCriticalSection m_critical_section;
  std::vector<wxString> m_table;
void Initialize();
public:
  template <typename... Args>
  explicit FilesView(Args&&... args) :
    wxGrid(std::forward<Args>(args)...) {
    Initialize();
  }
  void UpdateFiles(const std::vector<std::string> &files);
  void SwapFiles();
  void UpdateStatus(std::size_t index, Status status);
};

#endif // FILES_VIEW_HPP
