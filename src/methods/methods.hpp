#pragma once

#include <tlor/registry.hpp>

namespace tlor {

void register_builtin_methods(Registry& registry);

Plan schedule_threshold(const ScheduleRequest& request);
MethodInfo threshold_info();

DayRoutes route_nearest_neighbour(const RouteRequest& request);
MethodInfo nearest_neighbour_info();

}
