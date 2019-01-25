#include "batch.hpp"

#include <boost/range/adaptor/indexed.hpp>
#include <string>

static std::vector<std::vector<std::string>>
parse_csv(const std::string &file) {
  std::ifstream input(file);
  std::string line;
  std::vector<std::vector<std::string>> table;
  const boost::escaped_list_separator<char> separator('\\', ';', '\"');

  for (std::getline(input, line); input; std::getline(input, line)) {
    boost::tokenizer<boost::escaped_list_separator<char>> tokenizer(line,
                                                                    separator);
    table.emplace_back(tokenizer.begin(), tokenizer.end());
  }

  return table;
}

static void write_csv(const std::string &file,
               const std::vector<std::vector<std::string>> table) {
  std::ofstream output(file);
  for (const auto &row : table) {
    for (const auto &cell : row)
      output << cell << ';';
    output << '\n';
  }
}

class decimal_coma : public std::numpunct<char> {
protected:
  char do_decimal_point() const final {
    return ',';
  }
};

static double stod_coma(const std::string &string) {
  std::istringstream stream(string);
  // "The constructed locale object takes over responsibility for deleting this facet object."
  stream.imbue(std::locale(stream.getloc(), new decimal_coma));
  double d;
  stream >> d;
  return d;
}

static std::string to_string_coma(const double number) {
  std::ostringstream stream;
  stream.imbue(std::locale(stream.getloc(), new decimal_coma));
  stream << number;
  return stream.str();
}

void load_batch_file(const std::string &batch_file, Parameters &parameters,
                     std::vector<std::string> &files) {
  const auto table = parse_csv(batch_file);

  try {
    int row;
    // skip to empty row
    for (row = 0; !(table.at(row).empty() || table.at(row).at(0).empty());
         ++row)
      ;
    row += 3; // skip empty row and header
    // parameters until empty row
    for (; !(table.at(row).empty() || table.at(row).at(0).empty()); ++row) {
      // convert to the right type
      CenterSphereGenerator generator;
      generator.ratio = stod_coma(table.at(row).at(1)) / 100.0;
      generator.count = std::stoi(table.at(row).at(2));
      const auto level_count = std::stoi(table.at(row).at(3));
      const auto area_ratio = stod_coma(table.at(row).at(4)) / 100.0;
      Aggregation aggr = FIRST;
      if (table.at(row).at(5).find("atlag") != std::string::npos)
        aggr = AVERAGE;
      else if (table.at(row).at(5).find("smin") != std::string::npos)
        aggr = SMIN;
      else if (table.at(row).at(5).find("smax") != std::string::npos)
        aggr = SMAX;
      else if (table.at(row).at(5).find("umin") != std::string::npos)
        aggr = UMIN;
      else if (table.at(row).at(5).find("umax") != std::string::npos)
        aggr = UMAX;

      // put it into the Parameters structure
      auto c_s_it = boost::find_if(parameters, [&generator](const auto &g) {
        return g.value == generator;
      });
      if (c_s_it == parameters.end())
        c_s_it = parameters.insert(c_s_it, CenterSphere{generator});

      auto l_c_it = boost::find_if(c_s_it->next, [&level_count](const auto &l) {
        return l.value == level_count;
      });
      if (l_c_it == c_s_it->next.end())
        l_c_it = c_s_it->next.insert(l_c_it, LevelCount{level_count});

      auto a_r_it = boost::find_if(l_c_it->next, [&area_ratio](const auto &a) {
        return a.value == area_ratio;
      });
      if (a_r_it == l_c_it->next.end())
        a_r_it = l_c_it->next.insert(a_r_it, AreaRatio{area_ratio});

      auto aggr_it = boost::find(a_r_it->next, aggr);
      if (aggr_it == a_r_it->next.end())
        a_r_it->next.insert(aggr_it, aggr);
    }
    row += 3; // skip header
    // files until the end
    for (; row < table.size(); ++row) {
      files.push_back(table.at(row).at(1));
    }
  } catch (const std::out_of_range &oor) {
    std::cerr << oor.what() << '\n';
  } catch (const std::invalid_argument &iae) {
    std::cerr << iae.what() << '\n';
  }
}

void save_batch_file(const std::string &original_file,
                     const std::string &new_file, const Results &results) {
  using boost::adaptors::indexed;
  using boost::irange;
  using namespace std::string_literals;
  auto table = parse_csv(original_file);

  try {
    int row;
    // skip to empty row
    for (row = 0; !(table.at(row).empty() || table.at(row).at(0).empty());
         ++row)
      ;
    row += 3; // skip empty row and header
    // parameters until empty row
    std::vector<ParameterSignature> signatures;
    for (; !(table.at(row).empty() || table.at(row).at(0).empty()); ++row) {
      Aggregation aggr = FIRST;
      if (table.at(row).at(5).find("atlag") != std::string::npos)
        aggr = AVERAGE;
      else if (table.at(row).at(5).find("smin") != std::string::npos)
        aggr = SMIN;
      else if (table.at(row).at(5).find("smax") != std::string::npos)
        aggr = SMAX;
      else if (table.at(row).at(5).find("umin") != std::string::npos)
        aggr = UMIN;
      else if (table.at(row).at(5).find("umax") != std::string::npos)
        aggr = UMAX;
      signatures.emplace_back(
          stod_coma(table.at(row).at(1)) / 100.0, std::stoi(table.at(row).at(2)),
          std::stoi(table.at(row).at(3)), stod_coma(table.at(row).at(4)) / 100.0, aggr);
    }
    // look for the first empty column in the header
    std::size_t column_count;
    for (column_count = 0; column_count < table.at(row + 2).size() &&
	   !table.at(row + 2).at(column_count).empty();
         ++column_count)
      ;
    // column count + empty column + 11 mesh properties + (empty column + 4 results) * parameter count
    const auto width = column_count + 12 + 5 * signatures.size();
    row++; // skip empty row

    table.at(row).resize(width);
    for (const auto index : irange(0ul, signatures.size()))
      table.at(row).at(column_count + 12 + 5 * index) = std::to_string(index);
    row++;
    table.at(row).resize(width);
    table.at(row).at(column_count + 1) = "A"s;
    table.at(row).at(column_count + 2) = "V"s;
    table.at(row).at(column_count + 3) = "a"s;
    table.at(row).at(column_count + 4) = "b"s;
    table.at(row).at(column_count + 5) = "c"s;
    table.at(row).at(column_count + 6) = "K"s;
    table.at(row).at(column_count + 7) = "T"s;
    table.at(row).at(column_count + 8) = "c/a"s;
    table.at(row).at(column_count + 9) = "b/a"s;
    table.at(row).at(column_count + 10) = "Ibody"s;
    table.at(row).at(column_count + 11) = "Iproj"s;
    for (const auto index : irange(0ul, signatures.size())) {
      table.at(row).at(column_count + 12 + 5 * index) = "S"s;
      table.at(row).at(column_count + 13 + 5 * index) = "U"s;
      table.at(row).at(column_count + 14 + 5 * index) = "Reeb"s;
      table.at(row).at(column_count + 15 + 5 * index) = "Morse"s;
    }
    row++;
    
    // files
    for (; row < table.size(); ++row) {
      table.at(row).resize(width);
      const auto result = results.find(table.at(row).at(1));
      if (result != results.end()) {
	const auto &data = result->second;
	table.at(row).at(column_count + 1) = to_string_coma(data.area);
	table.at(row).at(column_count + 2) = to_string_coma(data.volume);
	table.at(row).at(column_count + 3) = to_string_coma(data.a);
	table.at(row).at(column_count + 4) = to_string_coma(data.b);
	table.at(row).at(column_count + 5) = to_string_coma(data.c);
	table.at(row).at(column_count + 6) = to_string_coma(data.proj_circumference);
	table.at(row).at(column_count + 7) = to_string_coma(data.proj_area);
      
	for (const auto &s : signatures | indexed()) {
	  const auto surm = data.surm.find(s.value());
	  if (surm != data.surm.end()) {
	    table.at(row).at(column_count + 12 + 5 * s.index()) = std::to_string(surm->second.stable);
	    table.at(row).at(column_count + 13 + 5 * s.index()) = std::to_string(surm->second.unstable);
	    table.at(row).at(column_count + 14 + 5 * s.index()) = surm->second.reeb;
	    table.at(row).at(column_count + 15 + 5 * s.index()) = surm->second.morse;
	  } else {
	    table.at(row).at(column_count + 12 + 5 * s.index()) = "error"s;
	  }
	}
      } else {
	table.at(row).at(column_count + 1) = "error"s;
      }
    }
  } catch (const std::out_of_range &oor) {
    std::cerr << oor.what() << '\n';
  } catch (const std::invalid_argument &iae) {
    std::cerr << iae.what() << '\n';
  }

  write_csv(new_file, table);
}
