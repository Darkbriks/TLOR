#include <tlor/check.hpp>

int main() {
    tlor::Instance instance(tlor::InstanceData{
        .node_count = 2,
        .depot = 1,
        .day_count = 1,
        .distance_m = {0, 10, 10, 0},
        .duration_s = {0, 1, 1, 0},
        .service_s = {60, 0},
        .capacity_kg = {100, 0},
        .active = {1, 0},
        .demand_kg = {5, 0},
    });
    tlor::Fleet fleet({{100, 3600}}, 1, {});
    tlor::Plan plan(0, 1);
    plan.add(0, 0);
    tlor::Solution solution(plan);
    tlor::DayRoutes routes(0);
    routes.add({0, {1, 0, 1}});
    solution.set_routes(routes);
    return tlor::check(instance, fleet, tlor::State::initial(2), solution, instance.demand()).empty() ? 0 : 1;
}
