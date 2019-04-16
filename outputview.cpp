#include <wx/filename.h>
#include <wx/grid.h>

#include <boost/range/adaptor/indexed.hpp>
#include <gdfontl.h>

#include "outputview.hpp"
#include "model/ratios.hpp"

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

static constexpr const char *labels[] = {"Name",
                                         "Number of lines",
                                         "Minimum area",
                                         "X offset",
                                         "Y offset",
                                         "Z offset",
                                         "S",
                                         "U",
                                         "Surface area",
                                         "Volume",
                                         "a",
                                         "b",
                                         "c",
                                         "Largest circumference",
                                         "Largest area",
                                         "Xmin",
                                         "Xmax",
                                         "Ymin",
                                         "Ymax",
                                         "Zmin",
                                         "Zmax",
                                         "Reeb code",
                                         "Morse code"};
static constexpr std::size_t label_count =
    sizeof(labels) / sizeof(const char *);
static constexpr std::size_t longest_label = string_length(
    *std::max_element(std::begin(labels), std::end(labels), is_shorter), 0);

void OutputView::Initialize(const std::string &fileName) {
  m_table.resize(label_count + ratio_label_count);
  CreateGrid(label_count + ratio_label_count, 1);
  EnableEditing(false);
  SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
  HideColLabels();

  int row = 0;
  for (const auto &label : labels)
    SetRowLabelValue(row++, label);

  for (const auto &label : ratio_labels)
    SetRowLabelValue(row++, label);

  SetRowLabelSize(wxGRID_AUTOSIZE);
  DisableDragGridSize();

  row = 0;
  SetCellRenderer(row++, 0, new wxGridCellStringRenderer()); // name
  SetCellRenderer(row++, 0, new wxGridCellNumberRenderer()); // number of lines
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // minimum area
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // x offset
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // y offset
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // z offset
  SetCellRenderer(row++, 0, new wxGridCellNumberRenderer()); // S
  SetCellRenderer(row++, 0, new wxGridCellNumberRenderer()); // U
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // surface area
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // volume
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // a
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // b
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // c  
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // largest circ
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // largest area
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // Xmin
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // Xmax
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // Ymin
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // Ymax
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // Zmin
  SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4)); // Zmax  
  SetCellRenderer(row++, 0, new wxGridCellStringRenderer()); // Reeb
  SetCellRenderer(row++, 0, new wxGridCellStringRenderer()); // Morse
  for (; row < label_count + ratio_label_count;)
    SetCellRenderer(row++, 0, new wxGridCellFloatRenderer(-1, 4));

  wxFileName name(fileName);
  m_table[0] = name.GetName();
  SetCellValue(0, 0, name.GetName());
  AutoSize();
}

void OutputView::UpdateParameters(const Vector &offset, int level_count,
                                  double area_ratio) {
  wxCriticalSectionLocker lock(m_critical_section);
  int row = 1;
  m_table[row++] = wxString::Format("%d", level_count);
  m_table[row++] = wxString::Format("%f", area_ratio);
  m_table[row++] = wxString::Format("%f", offset.x());
  m_table[row++] = wxString::Format("%f", offset.y());
  m_table[row++] = wxString::Format("%f", offset.z());
}

void OutputView::UpdateMeshData(const std::array<double, 13> &properties) {
  wxCriticalSectionLocker lock(m_critical_section);
  for (int i = 0; i < properties.size(); ++i)
    m_table[8 + i] = wxString::Format("%f", properties[i]);
}
void OutputView::UpdateSU(int S, int U) {
  wxCriticalSectionLocker lock(m_critical_section);
  int row = 6;
  m_table[row++] = wxString::Format("%d", S);
  m_table[row++] = wxString::Format("%d", U);
}
void OutputView::UpdateReeb(const std::string &reeb) {
  wxCriticalSectionLocker lock(m_critical_section);
  m_table[21] = reeb;
}
void OutputView::UpdateMorse(const std::string &morse) {
  wxCriticalSectionLocker lock(m_critical_section);
  m_table[22] = morse;
}

void OutputView::UpdateRatios(const std::array<double, 6> &properties) {
  wxCriticalSectionLocker lock(m_critical_section);
  for (int i = 0; i < properties.size(); ++i)
    m_table[23 + i] = wxString::Format("%f", properties[i]);
}

void OutputView::Swap() {
  using boost::adaptors::indexed;
  wxCriticalSectionLocker lock(m_critical_section);

  for (const auto &row : m_table | indexed())
    SetCellValue(row.index(), 0, row.value());
  AutoSize();
}

#ifdef GD_FOUND
GD::Image OutputView::Screenshot() const {
  const int font_width = 8;
  const int font_height = 16;

  std::size_t longest_value = 0;
  for (int row = 0; row < GetNumberRows(); ++row)
    longest_value = std::max(GetCellValue(row, 0).Len(), longest_value);

  auto font = gdFontGetLarge();
  auto color = GD::TrueColor(0, 0, 0).Int();
  GD::Image image((longest_label + longest_value) * font_width,
                  GetNumberRows() * font_height, true);
  image.Fill(0, 0, GD::TrueColor(255, 255, 255).Int());

  for (std::size_t i = 0; i < label_count; ++i)
    image.String(font, 0, i * font_height, labels[i], color);
  for (std::size_t i = 0; i < ratio_label_count; ++i)
    image.String(font, 0, (i + label_count) * font_height, ratio_labels[i],
                 color);

  const auto second_column = longest_label * font_width;
  for (std::size_t i = 0; i < GetNumberRows(); ++i)
    image.String(font, second_column, i * font_height,
                 static_cast<const char *>(GetCellValue(i, 0).c_str()), color);
  return image;
}
#endif //GD_FOUND
