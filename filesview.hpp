#ifndef FILES_VIEW_HPP
#define FILES_VIEW_HPP

#include <wx/grid.h>

#include "batch.hpp"

class FilesView final : public wxGrid {
  void Initialize();
public:
  template <typename... Args>
  explicit FilesView(Args&&... args) :
    wxGrid(std::forward<Args>(args)...) {
    Initialize();
  }
  void UpdateFiles(const std::vector<std::string> &files);
  void UpdateStatuses(const std::vector<Status> &statuses);
  void UpdateStatus(std::size_t index, Status status);
};

#endif // FILES_VIEW_HPP
