#ifndef RESULT_HPP
#define RESULT_HPP

struct SUPair {
    double S, U;
    SUPair(const double S, const double U) : S(S), U(U) {
    }
    SUPair(const std::pair<unsigned int, unsigned int> &pair) : S(pair.first), U(pair.second) {
    }
    SUPair(const SUPair &) = default;
    SUPair(SUPair &&) = default;
    SUPair& operator=(const SUPair&) = default;
    SUPair& operator=(SUPair &&) = default;
    SUPair operator+(SUPair &&other) {
        return {S + other.S, U + other.U};
    }
    SUPair operator/(const std::size_t divider) {
        return {S / divider, U / divider};
    }
};

#endif // RESULT_HPP
