#include "parameters.hpp"

std::istream &operator>>(std::istream &stream, Parameters &parameters) {
    parameters.next.clear();

    while(stream.peek() != ';') {
        stream.ignore(std::numeric_limits<std::streamsize>::max(), ';');
		double vol_rat, area_rat;
        int center_count, level_count;
        char dummy;
        stream >> vol_rat >> dummy
                >> center_count >> dummy
                >> level_count >> dummy
                >> area_rat >> dummy;
        const double volume_ratio = vol_rat / 100.0, area_ratio = area_rat / 100.0;
        const CenterSphereGenerator center_sphere{Vector(0, 0, 0),
                    volume_ratio,
                    center_count};

        std::string type;
        std::getline(stream, type);
        Aggregation aggr = FIRST;
        if (type.find("atlag") != std::string::npos) {
            aggr = AVERAGE;
        } else if (type.find("smin") != std::string::npos) {
            aggr = SMIN;
        } else if (type.find("smax") != std::string::npos) {
            aggr = SMAX;
        } else if (type.find("umin") != std::string::npos) {
            aggr = UMIN;
        } else if (type.find("umax") != std::string::npos) {
            aggr = UMAX;
        }

        ++parameters.m_total_count;

        bool insert_c_s = true;
        for (auto &&c_s : parameters) {
            if (static_cast<CenterSphereGenerator>(c_s.get<1>()) == center_sphere) {
                bool insert_l_c = true;
                for (auto &&l_c : c_s.get<1>()) {
                    if (l_c.get<1>() == level_count) {
                        bool insert_a_r = true;
                        for (auto &&a_r : l_c.get<1>()) {
                            if (a_r.get<1>() == area_ratio) {
                                a_r.get<1>().next.push_back(aggr);
                                insert_a_r = false;
                                break;
                            }
                        }
                        if (insert_a_r)
                            l_c.get<1>().next.push_back(AreaRatio(area_ratio,
                                                                  aggr));
                        insert_l_c = false;
                        break;
                    }
                }
                if (insert_l_c)
                    c_s.get<1>().next.push_back(LevelCount(level_count,
                                                           AreaRatio(area_ratio,
                                                                     aggr)));
                insert_c_s = false;
                break;
            }
        }
        if (insert_c_s)
            parameters.next.push_back(CenterSphere(center_sphere,
                                                   LevelCount(level_count,
                                                              AreaRatio(area_ratio,
                                                                        aggr))));
    }
    stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const Parameters &parameters) {
    int count = 0;
    for (auto &&c_s : parameters) {
      const auto center_sphere = static_cast<CenterSphereGenerator>(c_s.get<1>());
        for (auto &&l_c : c_s.get<1>())
            for (auto &&a_r : l_c.get<1>())
                for (auto &&a : a_r.get<1>()) {
                    stream << (++count) << ';'
                           << center_sphere.ratio * 100.0 << ';'
                           << center_sphere.count << ';'
                           << l_c.get<1>() << ';'
                           << a_r.get<1>() * 100.0 << ';';
                    switch(a.get<1>()) {
                    case AVERAGE:
                        stream << "atlag";
                        break;
                    case SMIN:
                        stream << "smin";
                        break;
                    case SMAX:
                        stream << "smax";
                        break;
                    case UMIN:
                        stream << "umin";
                        break;
                    case UMAX:
                        stream << "umax";
                        break;
                    default:
                        stream << "elso";
                    }
                    stream << '\n';
                }
    }
    return stream;
}
