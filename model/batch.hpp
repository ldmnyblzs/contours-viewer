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
