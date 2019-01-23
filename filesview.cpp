#include "filesview.hpp"
#include <boost/range/adaptor/indexed.hpp>

static constexpr const char* labels[] = {"File name", "Status"};
static constexpr const char* types[] = {"string", "string"};
static constexpr std::size_t label_count = sizeof(labels) / sizeof(const char*);
static constexpr const char* status_labels[] = {"Waiting", "Running", "OK", "Error"};

void FilesView::Initialize() {
  CreateGrid(0, label_count);
  EnableEditing(false);
  for (int col = 0; col < label_count; ++col) {
    SetColLabelValue(col, labels[col]);
    SetColFormatCustom(col, types[col]);
  }
  AutoSize();
}

void FilesView::UpdateFiles(const std::vector<std::string> &files) {
  wxCriticalSectionLocker lock(m_critical_section);
  
  for (const auto &file : files)
    m_table.push_back(file);
}
void FilesView::SwapFiles() {
  using boost::adaptors::indexed;

  wxGridUpdateLocker lock(this);
  wxCriticalSectionLocker lock2(m_critical_section);
  
  AppendRows(m_table.size());
  for (const auto &row : m_table | indexed())
    SetCellValue(row.index(), 0, row.value());
  AutoSize();
}

void FilesView::UpdateStatus(std::size_t index, Status status) {
  SetCellValue(index, 1, status_labels[status]);
}
