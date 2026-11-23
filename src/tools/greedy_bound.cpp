#include <tlor/error.hpp>
#include <tlor/evaluate.hpp>
#include <tlor/fleet.hpp>
#include <tlor/io.hpp>
#include <tlor/params.hpp>
#include <tlor/steps.hpp>

#include <filesystem>
#include <iostream>
#include <vector>

namespace {

constexpr double truck_capacity_kg = 10000.0;

std::filesystem::path find_project_root() {
    std::filesystem::path directory = std::filesystem::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        if (std::filesystem::exists(directory / "data" / "tables" / "active_points.csv") &&
            std::filesystem::exists(directory / "Maps" / "offline map" / "data" / "distances.json")) {
            return directory;
        }
        if (!directory.has_parent_path() || directory.parent_path() == directory) {
            break;
        }
        directory = directory.parent_path();
    }
    throw tlor::Error(tlor::Status::io_error, "unable to find data/tables/active_points.csv and "
                                              "Maps/offline map/data/distances.json above the current directory");
}

}

int main() {
    try {
        const std::filesystem::path root = find_project_root();
        const auto loaded = tlor::load_instance({root / "data" / "tables", root / "Maps" / "offline map" / "data", {}, false});
        const tlor::Instance& instance = loaded.instance;
        const tlor::Fleet fleet({tlor::Truck{truck_capacity_kg, 1e9}}, instance.day_count(), {});
        const std::vector<tlor::Node> points = instance.active_points();
        std::vector<double> full;
        for (tlor::Node point : points) {
            full.push_back(instance.capacity(point));
        }
        const tlor::Params none;
        const tlor::DayRoutes routes = tlor::route(instance, fleet, 0, points, full, "greedy", none, none, nullptr);
        const double distance_m = tlor::route_indicators(instance, fleet, routes).at("distance_m");
        std::cout << "Active points: " << points.size() << '\n';
        std::cout << "Depot: " << instance.depot() << '\n';
        std::cout << "Number of tours: " << routes.size() << '\n';
        std::cout << "Total distance: " << distance_m / 1000.0 << " km\n";
    } catch (const tlor::Error& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
