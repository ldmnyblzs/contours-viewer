#include "parametersview.hpp"
#include "parameters.hpp"

static constexpr const char* labels[] = {"Sphere volume (%)", "Number of centers", "Number of lines", "Minimum area (%)", "Aggregation method"};
static constexpr const char* types[] = {"double", "long", "long", "double", "string"};
static constexpr std::size_t label_count = sizeof(labels) / sizeof(const char*);

void ParametersView::Initialize() {
  CreateGrid(0, label_count);
  EnableEditing(false);
  for (int col = 0; col < label_count; ++col) {
    SetColLabelValue(col, labels[col]);
    SetColFormatCustom(col, types[col]);
  }
  AutoSize();
}

void ParametersView::Update(const Parameters &parameters) {
	wxGridUpdateLocker lock(this);
	int row = 0;
	for (auto &&c_s : parameters) {
		const auto center_sphere = static_cast<CenterSphereGenerator>(c_s.get<1>());
		for (auto &&l_c : c_s.get<1>()) {
			for (auto &&a_r : l_c.get<1>()) {
				for (auto &&a : a_r.get<1>()) {
					AppendRows();
					SetCellValue(row, 0, wxString::Format("%f", center_sphere.ratio * 100.0));
					SetCellValue(row, 1, wxString::Format("%d", center_sphere.count));
					SetCellValue(row, 2, wxString::Format("%d", static_cast<int>(l_c.get<1>())));
					SetCellValue(row, 3, wxString::Format("%f", a_r.get<1>() * 100.0));
					switch (a.get<1>()) {
					case AVERAGE:
						SetCellValue(row, 4, "average");
						break;
					case SMIN:
						SetCellValue(row, 4, "Smin");
						break;
					case SMAX:
						SetCellValue(row, 4, "Smax");
						break;
					case UMIN:
						SetCellValue(row, 4, "Umin");
						break;
					case UMAX:
						SetCellValue(row, 4, "Umax");
						break;
					default:
						SetCellValue(row, 4, "first");
					}
					++row;
				}
			}
		}
	}
	AutoSize();
}
