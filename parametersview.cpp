#include "parametersview.hpp"
#include <boost/range/adaptor/indexed.hpp>

static constexpr const char *labels[] = {
    "Sphere volume (%)", "Number of centers", "Number of lines",
    "Minimum area (%)", "Aggregation method"};
static constexpr const char *types[] = {"double", "long", "long", "double",
                                        "string"};
static constexpr std::size_t label_count =
    sizeof(labels) / sizeof(const char *);
static constexpr const char *aggr_types[] = {"first", "average", "Smin",
                                             "Smax",  "Umin",    "Umax"};

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
  wxCriticalSectionLocker lock(m_critical_section);

  for (const auto &center_sphere : parameters) {
    for (const auto &level_count : center_sphere.next) {
      for (const auto &area_ratio : level_count.next) {
        for (const auto &aggregation : area_ratio.next) {
          m_table.push_back(
              {wxString::Format("%f", center_sphere.value.ratio * 100.0),
               wxString::Format("%d", center_sphere.value.count),
               wxString::Format("%d", level_count.value),
               wxString::Format("%f", area_ratio.value * 100.0),
               wxString(aggr_types[aggregation])});
        }
      }
    }
  }
}

void ParametersView::Swap() {
  using boost::adaptors::indexed;

  wxGridUpdateLocker lock(this);
  wxCriticalSectionLocker lock2(m_critical_section);

  AppendRows(m_table.size());
  for (const auto &row : m_table | indexed())
    for (const auto &cell : row.value() | indexed())
      SetCellValue(row.index(), cell.index(), cell.value());
  AutoSize();
}
