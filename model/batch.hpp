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

#ifndef MODEL_BATCH_HPP
#define MODEL_BATCH_HPP 1

#include <boost/filesystem/path.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/tokenizer.hpp>
#include <codecvt>
#include <fstream>
#include <string>
#include <vector>

#include "parameters.hpp"

using ParameterSignature = std::tuple<double, int, int, double, Aggregation>;
struct SURM {
  float stable;
  float unstable;
  std::string reeb;
  std::string morse;
};
struct FileResults {
  double area, volume;
  double a, b, c;
  double proj_circumference, proj_area;
  std::array<double, 6> ratios;
  std::map<ParameterSignature, SURM> surm;
};
using Results = std::unordered_map<std::string, FileResults>;

void load_batch_file(const std::string &batch_file,
		     Parameters &parameters,
		     std::vector<std::string> &files);
void save_batch_file(const std::string &original_file,
		     const std::string &new_file,
		     const Results &results);

#endif // MODEL_BATCH_HPP
