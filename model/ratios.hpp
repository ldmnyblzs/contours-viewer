/*
  Copyright 2019 Balázs Ludmány

  This file is part of contours-viewer.

  contours-viewer is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  contours-viewer is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with contours-viewer.  If not, see <https://www.gnu.org/licenses/>.
*/

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
