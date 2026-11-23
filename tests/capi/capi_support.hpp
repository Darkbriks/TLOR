#pragma once

#include <tlor/tlor.h>

#include "support/small_arrays.hpp"

inline tlor_instance* small_instance_handle() {
    static SmallArrays a = small_arrays();
    tlor_instance_desc d{a.node_count, a.depot, a.day_count, a.distance_m.data(), a.duration_s.data(),
                         a.service_s.data(), a.capacity_kg.data(), a.active.data(), a.demand_kg.data()};
    tlor_instance* inst = nullptr;
    tlor_instance_create(&d, &inst);
    return inst;
}

inline tlor_fleet* small_fleet_handle(double capacity_kg = 150, double max_work_s = 3600) {
    const double capacity[2] = {capacity_kg, capacity_kg};
    const double work[2] = {max_work_s, max_work_s};
    uint8_t available[14];
    for (int i = 0; i < 14; ++i) {
        available[i] = (i / 2 == 5) ? 0 : 1;
    }
    tlor_fleet* fleet = nullptr;
    tlor_fleet_create(2, capacity, work, 7, available, &fleet);
    return fleet;
}
