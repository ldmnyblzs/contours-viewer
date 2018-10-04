#ifndef RATIOS_H
#define RATIOS_H

// this must be included before any standard headers
//#include <boost/python.hpp>

#include <vector>
#include <unordered_map>
#include <string>

/*
 * NOTE: The idea was that ratios could be defined in a file using Python syntax.
 * Turns out deploying an embedded interpreter is more hustle than I had time for.
 * At the moment these are just some hard-coded formulas.
 */

class Ratios
{
public:
    static std::vector<std::string> labels;
    //static std::vector<std::string> formulas;

    Ratios() = delete;
    Ratios(const Ratios &) = delete;
    Ratios(Ratios &&) = delete;

    //static void load(const std::string &filePath);
    static std::vector<double> calculate(const std::unordered_map<std::string, double> &inputs);
};

#endif // RATIOS_H
