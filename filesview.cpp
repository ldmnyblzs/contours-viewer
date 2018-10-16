#include "filesview.hpp"

static constexpr const char* labels[] = {"File name", "Status"};
static constexpr const char* types[] = {"string", "string"};
static constexpr std::size_t label_count = sizeof(labels) / sizeof(const char*);

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
  wxGridUpdateLocker lock(this);
  ClearGrid();
  AppendRows(files.size());
  for (int row = 0; row < files.size(); ++row) {
    SetCellValue(row, 0, files[row]);
  }
  AutoSize();
}

void FilesView::UpdateStatuses(const std::vector<Status> &statuses) {
  wxGridUpdateLocker lock(this);
  for (int row = 0; row < statuses.size(); ++row) {
    switch (statuses[row]) {
	case WAITING:
		SetCellValue(row, 1, "Waiting");
		break;
	case RUNNING:
		SetCellValue(row, 1, "Running");
		break;
	case OK:
		SetCellValue(row, 1, "OK");
		break;
	default:
		SetCellValue(row, 1, "Error");
	}
  }
  AutoSize();
}

void FilesView::UpdateStatus(std::size_t index, Status status) {
	switch (status) {
	case WAITING:
		SetCellValue(index, 1, "Waiting");
		break;
	case RUNNING:
		SetCellValue(index, 1, "Running");
		break;
	case OK:
		SetCellValue(index, 1, "OK");
		break;
	default:
		SetCellValue(index, 1, "Error");
	}
}
