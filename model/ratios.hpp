#ifndef MODEL_RATIOS_HPP
#define MODEL_RATIOS_HPP

#include <array>

constexpr const char *ratio_labels[] = {"c/a",   "b/a",        "Ibody",
                                        "Iproj", "Iellipsoid", "Iellipse"};
constexpr std::size_t ratio_label_count =
    sizeof(ratio_labels) / sizeof(const char *);

std::array<double, 6>
calculate_ratios(const std::array<double, 13> &properties);

#endif // MODEL_RATIOS_HPP
