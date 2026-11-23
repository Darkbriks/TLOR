#include "support/small_arrays.hpp"

#include <array>
#include <cstdlib>

SmallArrays small_arrays() {
    constexpr std::array<int, 6> x{1, 2, 3, 4, 5, 0};
    constexpr std::array<double, 6> daily{30, 10, 20, 50, 0, 0};
    SmallArrays a;
    a.node_count = 6;
    a.depot = 5;
    a.day_count = 7;
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            double d = 0;
            if (i != j) {
                d = 100.0 * std::abs(x[i] - x[j]) + (x[j] < x[i] ? 50.0 : 0.0);
            }
            a.distance_m.push_back(d);
            a.duration_s.push_back(d / 10.0);
        }
    }
    a.service_s = {60, 60, 60, 60, 60, 120};
    a.capacity_kg = {100, 100, 100, 100, 100, 0};
    a.active = {1, 1, 1, 1, 0, 0};
    for (int day = 0; day < a.day_count; ++day) {
        a.demand_kg.insert(a.demand_kg.end(), daily.begin(), daily.end());
    }
    return a;
}
