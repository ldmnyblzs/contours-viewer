#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

#include <vector>
#include <iostream>
#include <string>

#include <boost/iterator/zip_iterator.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <CGAL/point_generators_3.h>
#include <boost/range/algorithm/transform.hpp>
#include <boost/math/constants/constants.hpp>

#include "primitives.hpp"

struct CenterSphereGenerator {
    Vector offset = Vector(0, 0, 0);
    double ratio = 0.0;
    int count = 1;

    std::vector<Point> operator()(const double volume) const {
        const double radius = std::cbrt(ratio * volume * 3 / (4 * boost::math::constants::pi<double>()));
        std::vector<Point> centers(count);
        CGAL::Random random(0);
        std::copy_n(CGAL::Random_points_on_sphere_3<Point>(radius, random),
                    count,
                    centers.begin());
        boost::transform(centers,
                         centers.begin(),
                         [this](const Point &point) {
            return point + offset;
        });
        return centers;
    }
    bool operator==(const CenterSphereGenerator &other) const {
        return std::tie(offset, ratio, count) == std::tie(other.offset, other.ratio, other.count);
    }
};

enum Aggregation {
    FIRST,
    AVERAGE,
    SMIN,
    SMAX,
    UMIN,
    UMAX
};

class Parameters;

template <typename Type, typename Next>
class Parameter {
    Type value;
    std::vector<Next> next;
public:
    Parameter(Type value, Next next) : value(value) {
        this->next.push_back(next);
    }
    auto begin() {
        return boost::make_zip_iterator(boost::make_tuple(boost::make_counting_iterator(typename std::vector<Next>::size_type(0)), next.begin()));
    }
    auto end() {
        return boost::make_zip_iterator(boost::make_tuple(boost::make_counting_iterator(next.size()), next.end()));
    }
    auto begin() const {
        return boost::make_zip_iterator(boost::make_tuple(boost::make_counting_iterator(typename std::vector<Next>::size_type(0)), next.cbegin()));
    }
    auto end() const {
        return boost::make_zip_iterator(boost::make_tuple(boost::make_counting_iterator(next.size()), next.cend()));
    }
    auto next_count() const {
        return next.size();
    }
    operator Type() const {
        return value;
    }
    friend std::istream &operator>>(std::istream &stream, Parameters &parameters);
    friend std::ostream &operator<<(std::ostream &stream, const Parameters &parameters);
    friend class Parameters;
};

typedef Parameter<double, Aggregation> AreaRatio;
typedef Parameter<int, AreaRatio> LevelCount;
typedef Parameter<CenterSphereGenerator, LevelCount> CenterSphere;

class Parameters {
    std::vector<CenterSphere> next;
    std::size_t m_total_count = 0;
public:
	Parameters(const int count, const double amin, const double x, const double y, const double z) {
        next.push_back(CenterSphere(CenterSphereGenerator{Vector(x, y, z), 0.0, 1} ,LevelCount(count, AreaRatio(amin, FIRST))));
	}
    Parameters() {
        next.push_back(CenterSphere(CenterSphereGenerator() ,LevelCount(100, AreaRatio(0.01, FIRST))));
    }
    auto begin() {
        return boost::make_zip_iterator(boost::make_tuple(boost::make_counting_iterator(typename std::vector<CenterSphere>::size_type(0)), next.begin()));
    }
    auto end() {
        return boost::make_zip_iterator(boost::make_tuple(boost::make_counting_iterator(next.size()), next.end()));
    }
    auto begin() const {
        return boost::make_zip_iterator(boost::make_tuple(boost::make_counting_iterator(typename std::vector<CenterSphere>::size_type(0)), next.cbegin()));
    }
    auto end() const {
        return boost::make_zip_iterator(boost::make_tuple(boost::make_counting_iterator(next.size()), next.cend()));
    }
    auto next_count() const {
        return next.size();
    }
    auto total_count() const {
        return m_total_count;
    }
  auto first_level_count() const {
    return next[0].next[0].next[0].value;
  }
  friend std::istream &operator>>(std::istream &stream, Parameters &parameters);
  friend std::ostream &operator<<(std::ostream &stream, const Parameters &parameters);
};

#endif // PARAMETERS_HPP
