#define _USE_MATH_DEFINES 1

#include "ratios.hpp"
#include <cmath>

std::vector<std::string> Ratios::labels = {"c/a",   "b/a",        "Ibody",
                                           "Iproj", "Iellipsoid", "Iellipse"};

std::vector<double>
Ratios::calculate(const std::unordered_map<std::string, double> &inputs) {
  const auto ellipsoidVolume =
      4.0 / 3.0 * M_PI * inputs.at("a") * inputs.at("b") * inputs.at("c");
  const double p = 1.6;
  const auto ap = std::pow(inputs.at("a"), p);
  const auto bp = std::pow(inputs.at("b"), p);
  const auto cp = std::pow(inputs.at("c"), p);
  const auto ellipsoidSurfaceArea =
      4.0 * M_PI * std::pow((ap * bp + ap * cp + bp * cp) / 3.0, 1.0 / p);

  const auto ellipseCircumference =
      M_PI * (3.0 * (inputs.at("a") + inputs.at("b")) -
              std::sqrt((3.0 * inputs.at("a") + inputs.at("b")) *
                        (inputs.at("a") + 3.0 * inputs.at("b"))));
  const auto ellipseArea = M_PI * inputs.at("a") * inputs.at("b");

  return std::vector<double>{
      inputs.at("c") / inputs.at("a"),
      inputs.at("b") / inputs.at("a"),
      36.0 * M_PI * std::pow(inputs.at("volume"), 2.0) /
          std::pow(inputs.at("surface_area"), 3.0),
      4.0 * M_PI * inputs.at("projected_area") /
          std::pow(inputs.at("projected_circumference"), 2.0),
      36.0 * M_PI * std::pow(ellipsoidVolume, 2.0) /
          std::pow(ellipsoidSurfaceArea, 3.0),
      4.0 * M_PI * ellipseArea / std::pow(ellipseCircumference, 2.0)};
}
