#pragma once

#include <cstdint>
#include <vector>

struct SmallArrays {
    std::int32_t node_count = 0;
    std::int32_t depot = 0;
    std::int32_t day_count = 0;
    std::vector<double> distance_m;
    std::vector<double> duration_s;
    std::vector<double> service_s;
    std::vector<double> capacity_kg;
    std::vector<double> demand_kg;
    std::vector<std::uint8_t> active;
};

SmallArrays small_arrays();
