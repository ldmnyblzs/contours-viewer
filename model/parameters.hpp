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

#ifndef MODEL_PARAMETERS_HPP
#define MODEL_PARAMETERS_HPP 1

#include <CGAL/point_generators_3.h>
#include <boost/math/constants/constants.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <vector>

#include "primitives.hpp"

template <typename Type, typename Next> struct Parameter {
  Type value;
  std::vector<Next> next;
};

struct CenterSphereGenerator {
  Vector offset = Vector(0, 0, 0);
  double ratio = 0.0;
  int count = 1;

  std::vector<Point> operator()(const double volume) const {
    using namespace std;
    using namespace boost;
    using namespace boost::math::double_constants;
    using namespace CGAL;

    const double radius = cbrt(ratio * volume * 3 / (4 * pi));
    vector<Point> centers(count);
    Random random(0);
    std::copy_n(Random_points_on_sphere_3<Point>(radius, random), count,
                centers.begin());
    transform(centers, centers.begin(),
              [this](const Point &point) { return point + offset; });
    return centers;
  }
  bool operator==(const CenterSphereGenerator &other) const {
    return std::tie(offset, ratio, count) ==
           std::tie(other.offset, other.ratio, other.count);
  }
};

// Don't rearrange because ParametersView uses Aggregation as an index!
enum Aggregation { FIRST, AVERAGE, SMIN, SMAX, UMIN, UMAX };

using AreaRatio = Parameter<double, Aggregation>;
using LevelCount = Parameter<int, AreaRatio>;
using CenterSphere = Parameter<CenterSphereGenerator, LevelCount>;
using Parameters = std::vector<CenterSphere>;

#endif // MODEL_PARAMETERS_HPP
