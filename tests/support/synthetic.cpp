#include "support/synthetic.hpp"

#include "support/small_arrays.hpp"

namespace tlor::test {

Instance small_instance() {
    auto a = small_arrays();
    return Instance(InstanceData{
        .node_count = a.node_count,
        .depot = a.depot,
        .day_count = a.day_count,
        .distance_m = a.distance_m,
        .duration_s = a.duration_s,
        .service_s = a.service_s,
        .capacity_kg = a.capacity_kg,
        .active = a.active,
        .demand_kg = a.demand_kg,
    });
}

Fleet small_fleet(double capacity_kg, double max_work_s, std::int32_t trucks) {
    constexpr std::int32_t days = 7;
    std::vector<std::uint8_t> available(static_cast<std::size_t>(days * trucks), 1);
    for (std::int32_t t = 0; t < trucks; ++t) {
        available[static_cast<std::size_t>(5 * trucks + t)] = 0;
    }
    return Fleet(std::vector<Truck>(static_cast<std::size_t>(trucks), Truck{capacity_kg, max_work_s}), days,
                 std::move(available));
}

}
