#include <wx/grid.h>
#include <wx/filename.h>

#include <gdfontl.h>

#include "outputview.hpp"
#include "ratios.hpp"
#include "parameters.hpp"

/*!
 * Returns true if a is shorter than b.
 */
constexpr bool is_shorter(const char *a, const char *b) {
  if (*a == '\0')
    if (*b == '\0')
      return false; // same length
    else
      return true; // a is shorter than b
  else if (*b == '\0')
    return false; // a is longer than b
  else
    return is_shorter(a + 1, b + 1);
}

constexpr std::size_t string_length(const char *str, const std::size_t length) {
	return (*str == '\0') ? length : string_length(str + 1, length + 1);
}

static constexpr const char* labels[] = {"Name", "Number of lines", "Minimum area", "X offset", "Y offset", "Z offset", "S", "U", "Xmin", "Xmax", "Ymin", "Ymax", "Zmin", "Zmax", "Surface area", "Volume", "a", "b", "c", "Largest circumference", "Largest area"};
static constexpr std::size_t label_count = sizeof(labels) / sizeof(const char*);
static constexpr std::size_t longest_label = string_length(*std::max_element(std::begin(labels), std::end(labels), is_shorter), 0);

void OutputView::Initialize(const wxString &fileName) {
	CreateGrid(label_count + Ratios::labels.size(), 1);
	EnableEditing(false);
	SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
	HideColLabels();
	
	int row = 0;
	for (const auto &label : labels)
		SetRowLabelValue(row++, label);
	
	for (const auto &label : Ratios::labels)
		SetRowLabelValue(row++, label);
	
	SetRowLabelSize(wxGRID_AUTOSIZE);
	DisableDragGridSize();

	row = 0;
	SetCellRenderer(row++, 0, new wxGridCellStringRenderer());
	SetCellRenderer(row++, 0, new wxGridCellNumberRenderer());
	SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	SetCellRenderer(row++, 0, new wxGridCellNumberRenderer());
	SetCellRenderer(row++, 0, new wxGridCellNumberRenderer());
	for (;row < label_count + Ratios::labels.size();)
	  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));
	
	wxFileName name(fileName);
	SetCellValue(0, 0, name.GetName());
	AutoSize();
}

void OutputView::UpdateParameters(const Parameters &parameters) {
  const auto c_s = parameters.begin();
  const auto offset = static_cast<CenterSphereGenerator>(c_s->get<1>()).offset;
  const auto l_c = c_s->get<1>().begin();
  const auto a_r = l_c->get<1>().begin();
  
  std::size_t row = 1;
  SetCellValue(row++, 0, wxString::Format("%d", static_cast<int>(l_c->get<1>())));
  SetCellValue(row++, 0, wxString::Format("%f", static_cast<double>(a_r->get<1>())));
  SetCellValue(row++, 0, wxString::Format("%f", offset.x()));
  SetCellValue(row++, 0, wxString::Format("%f", offset.y()));
  SetCellValue(row++, 0, wxString::Format("%f", offset.z()));
  AutoSize();
}

void OutputView::UpdateMeshData(const Mesh &mesh) {
	std::size_t row = 8;
	SetCellValue(row++, 0, wxString::Format("%f", mesh.xmin()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.xmax()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.ymin()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.ymax()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.zmin()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.zmax()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.surface_area()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.volume()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.a()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.b()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.c()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.projected_circumference()));
	SetCellValue(row++, 0, wxString::Format("%f", mesh.projected_area()));
	for (const auto &ratio : mesh.ratios())
		SetCellValue(row++, 0, wxString::Format("%f", ratio));
	
	AutoSize();
}

void OutputView::UpdateSU(const SUPair &su) {
	SetCellValue(6, 0, wxString::Format("%d", static_cast<int>(su.S)));
	SetCellValue(7, 0, wxString::Format("%d", static_cast<int>(su.U)));
	
	AutoSize();
}

GD::Image OutputView::Screenshot() const {
  const int font_width = 8;
  const int font_height = 16;

  std::size_t longest_value = 0;
  for (int row = 0; row < GetNumberRows(); ++row)
    longest_value = std::max(GetCellValue(row, 0).Len(), longest_value);
  
  auto font = gdFontGetLarge();
  auto color = GD::TrueColor(0, 0, 0).Int();
  GD::Image image((longest_label + longest_value) * font_width, GetNumberRows() * font_height, true);
  image.Fill(0, 0, GD::TrueColor(255, 255, 255).Int());
  
  for (std::size_t i = 0; i < label_count; ++i)
    image.String(font, 0, i * font_height, labels[i], color);
  for (std::size_t i = 0; i < Ratios::labels.size(); ++i)
	  image.String(font, 0, (i + label_count) * font_height, Ratios::labels[i], color);
  
  const auto second_column = longest_label * font_width;
  for (std::size_t i = 0; i < GetNumberRows(); ++i)
    image.String(font, second_column, i * font_height, static_cast<const char *>(GetCellValue(i, 0).c_str()), color);
  return image;
}
