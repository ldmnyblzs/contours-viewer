#include "batch.hpp"

#include <fstream>
#include <string>
#include <locale>
#include <codecvt>
#include <boost/algorithm/string/find.hpp>

Batch::Batch(const std::string &file_path, std::function<bool()> cancelled, std::function<void(std::size_t, Status)> status_changed) :
	m_cancelled(std::move(cancelled)),
	m_status_changed(std::move(status_changed)) {
  boost::filesystem::path path(file_path, std::codecvt_utf8<wchar_t>());
  m_directory = path.parent_path();
  
  std::ifstream input;
  input.exceptions(std::istream::failbit);
  
  try {
    input.open(path.native());
    std::string line;
    std::getline(input, line);
    while (!(line.empty() || line.at(0) == ';')) {
      m_data.push_back(line);
      std::getline(input, line);
    }
    // empty line read

    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // II. Vizsgalando parameterek
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // sorszam
    input >> m_parameters;

    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while (input) {
      input.ignore(std::numeric_limits<std::streamsize>::max(), ';');
      std::string filename;
      std::getline(input, filename, ';');
      if (!filename.empty()) {
	std::string filedata;
	std::getline(input, filedata);
	filedata.erase(boost::find_nth(filedata, ";", 9).begin(), filedata.end());
	m_files.push_back(filename);
	m_file_data.push_back(filedata);
	m_statuses.push_back(WAITING);
      }
    }
  } catch (...) {
    if (!input.eof() || m_files.size() == 0)
      throw "Couldn't load batch";
  }
}

bool Batch::run() {
  std::fill_n(m_statuses.begin(), m_files.size(), WAITING);
  m_mesh_data.clear();
  m_su.clear();
  std::size_t index = 0;
  for (auto &&file : m_files) {
    m_started = std::chrono::system_clock::now();
    if (m_cancelled())
      return false;
    m_statuses[index] = RUNNING;
	m_status_changed(index, RUNNING);
    std::ifstream stream((m_directory / file).native());
    Mesh mesh(stream);
    mesh.measure();
    Computation computation(mesh, std::bind(&Batch::cancelled, this));
    if (computation.run(m_parameters)) {
      m_mesh_data.emplace_back(computation.mesh());
      m_su.push_back(computation.su());
      m_statuses[index] = OK;
	  m_status_changed(index, OK);
    } else {
      m_statuses[index] = ERROR_;
	  m_status_changed(index, ERROR_);
    }
    ++index;
  }
  return true;
}

void Batch::save(const std::string &file_path) const {
  boost::filesystem::path path(file_path, std::codecvt_utf8<wchar_t>());
  std::ofstream output(path.native());
  output.imbue(std::locale(output.getloc(),
			   (m_file_data.size() >= 1) && (m_file_data[0].find(',') != std::string::npos) ?
			   reinterpret_cast<std::numpunct<char>*>(new decimal_coma) :
			   reinterpret_cast<std::numpunct<char>*>(new decimal_point)));
  for (auto &&line : m_data)
    output << line << '\n';
  output << ";\n";
  output << "II.;Vizsgalando paremeterek\n";
  output << "sorszam;gomb terfogat aranya [%];kozeppontok szama [db];szintvonalak szama [db];egyensuly terulet aranya [%];osszegzes\n";
  output << m_parameters;
  output << ";\n";
  output << "Parameterek sorszama:;;;0;(kezi meres);;;;;;;;;;;;;;;";
  output << std::string(Ratios::labels.size(), ';').c_str();
  for (std::size_t parameter = 1; parameter <= m_parameters.total_count(); ++parameter)
    output << ";;" << parameter << ';';
  output << '\n';
  output << "Adat megnevzese:;fajl neve;Fragmens;Tomeg;a;b;c;S;U;c/a;b/a;Fordulat;;A;V;a;b;c;K;T";
  for (const auto &label : Ratios::labels)
    output << ';' << label.c_str();
  for (std::size_t parameter = 0; parameter < m_parameters.total_count(); ++parameter)
    output << ";;S;U";
  output << '\n';
  std::size_t skip = 0;
  for (std::size_t file = 0; file < m_files.size(); ++file) {
    output << (file + 1) << ';';
    output << m_files[file] << ';';
    output << m_file_data[file];
    output << ";;";
    const auto mesh = m_mesh_data[file + skip];
    switch(m_statuses[file]) {
    case OK: {
      output << mesh.surface_area << ';'
	     << mesh.volume << ';'
	     << mesh.a << ';'
	     << mesh.b << ';'
	     << mesh.c << ';'
	     << mesh.projected_circumference << ';'
	     << mesh.projected_area;
      for (const auto ratio : mesh.ratios)
	output << ';' << ratio;
      for (const auto su : m_su[file + skip])
	output << ";;" << su.S << ';' << su.U;
    }
      break;
    case ERROR_:
      output << "error";
      //[[fallthrough]];
    default:
      ++skip;
    }
    output << '\n';
  }
}

bool Batch::cancelled() const {
    using namespace std::chrono;
    return m_cancelled() ||
      (duration_cast<seconds>(system_clock::now() - m_started).count() > m_deadline);
}

const std::vector<std::string> &Batch::files() const {
  return m_files;
}

const std::vector<Status> &Batch::statuses() const {
  return m_statuses;
}

const Parameters &Batch::parameters() const {
  return m_parameters;
}
