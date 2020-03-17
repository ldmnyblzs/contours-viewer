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

#include "ratios.hpp"

#include <boost/math/constants/constants.hpp>
#include <cmath>

std::array<double, 6>
calculate_ratios(const std::array<double, 13> &properties) {
  using namespace std;
  using namespace boost::math::double_constants;

  const auto &surface_area = properties[0];
  const auto &volume = properties[1];
  const auto &a = properties[2];
  const auto &b = properties[3];
  const auto &c = properties[4];
  const auto &circ = properties[5];
  const auto &area = properties[6];

  const auto p = 1.6;
  const auto ap = pow(a, p);
  const auto bp = pow(b, p);
  const auto cp = pow(c, p);

  const auto ellipsoidVolume = 4.0 / 3.0 * pi * a * b * c;
  const auto ellipsoidSurfaceArea =
      4.0 * pi * pow((ap * bp + ap * cp + bp * cp) / 3.0, 1.0 / p);
  const auto ellipseCircumference =
      pi * (3.0 * (a + b) - sqrt((3.0 * a + b) * (a + 3.0 * b)));
  const auto ellipseArea = pi * a * b;

  return {c / a,
          b / a,
          36.0 * pi * pow(volume, 2.0) / pow(surface_area, 3.0),
          4.0 * pi * area / pow(circ, 2.0),
          36.0 * pi * pow(ellipsoidVolume, 2.0) /
              pow(ellipsoidSurfaceArea, 3.0),
          4.0 * pi * ellipseArea / pow(ellipseCircumference, 2.0)};
}
