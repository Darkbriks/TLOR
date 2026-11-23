#include <tlor/io.hpp>

#include <tlor/error.hpp>
#include <tlor/simulate.hpp>

#include "io/csv.hpp"

#include <charconv>
#include <fstream>
#include <set>
#include <string_view>
#include <system_error>
#include <vector>

namespace tlor {

namespace {

std::string number(double value) {
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof buffer, value);
    return {buffer, result.ptr};
}

void write_text(const std::filesystem::path& file, const std::string& text) {
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw Error(Status::io_error, "cannot open " + io::shown(file) + " for writing");
    }
    out << text;
    out.close();
    if (!out) {
        throw Error(Status::io_error, "cannot write " + io::shown(file));
    }
}

void make_directory(const std::filesystem::path& dir) {
    std::error_code error;
    std::filesystem::create_directories(dir, error);
    std::error_code ignored;
    if (error || !std::filesystem::is_directory(dir, ignored)) {
        throw Error(Status::io_error,
                    "cannot create directory " + io::shown(dir) + (error ? ": " + error.message() : std::string{}));
    }
}

bool is_routes_file(std::string_view name) {
    if (name.size() != 22 || !name.starts_with("routes_") || !name.ends_with(".json")) {
        return false;
    }
    try {
        (void)parse_date(name.substr(7, 10));
        return true;
    } catch (const Error&) {
        return false;
    }
}

void remove_routes_files(const std::filesystem::path& dir) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file() && is_routes_file(io::shown(entry.path().filename()))) {
                std::filesystem::remove(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        throw Error(Status::io_error, "cannot remove old routes files from " + io::shown(dir) + ": " + e.what());
    }
}

struct RouteStatistics {
    double distance = 0;
    double duration = 0;
    double load = 0;
    int points = 0;
};

RouteStatistics statistics_of(const Instance& instance, const Plan& plan, const SimulationResult& simulation, Day day,
                              const Route& route, std::vector<bool>& collected) {
    RouteStatistics statistics;
    const auto in_range = [&](Node node) { return node >= 0 && node < instance.node_count(); };
    const auto& nodes = route.nodes;
    std::set<Node> visited;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const Node node = nodes[i];
        if (!in_range(node)) {
            continue;
        }
        if (i + 1 < nodes.size() && in_range(nodes[i + 1])) {
            statistics.distance += instance.distance(node, nodes[i + 1]);
            statistics.duration += instance.duration(node, nodes[i + 1]);
        }
        if (i == 0 || i + 1 == nodes.size()) {
            continue;
        }
        statistics.duration += instance.service(node);
        if (!instance.is_point(node) || !visited.insert(node).second) {
            continue;
        }
        ++statistics.points;
        const auto at = static_cast<std::size_t>(node);
        if (plan.collected(day, node) && !collected[at]) {
            collected[at] = true;
            statistics.load += simulation.level(day, node);
        }
    }
    if (!nodes.empty()) {
        statistics.duration += instance.service(instance.depot());
    }
    return statistics;
}

}

void write_routes_json(const std::filesystem::path& file, const DayRoutes& routes) {
    std::string text = "[";
    for (std::size_t r = 0; r < routes.size(); ++r) {
        text += r == 0 ? "\n    [" : ",\n    [";
        const auto& nodes = routes.routes()[r].nodes;
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            text += (i == 0 ? "" : ", ") + std::to_string(nodes[i]);
        }
        text += "]";
    }
    text += routes.size() == 0 ? "]\n" : "\n]\n";
    write_text(file, text);
}

void write_solution(const std::filesystem::path& out_dir, const Instance& instance, const State& start,
                    const Solution& solution, const Demand& demand, std::chrono::sys_days first_date) {
    const Plan& plan = solution.plan();
    const auto simulation = simulate(instance, start, plan.start(), plan.horizon(), plan, demand);
    make_directory(out_dir);
    remove_routes_files(out_dir);
    std::string schedule = "date;point_id;collected_kg;fill_rate\n";
    std::string statistics = "date;route;truck;distance;duration;load_kg;points\n";
    for (Day day = plan.start(); day < plan.end(); ++day) {
        const std::string date = format_date(first_date + std::chrono::days{day});
        for (Node point : plan.points(day)) {
            if (!instance.is_point(point)) {
                continue;
            }
            const double kg = simulation.level(day, point);
            schedule += date + ';' + std::to_string(point) + ';' + number(kg) + ';' +
                        number(kg / instance.capacity(point)) + '\n';
        }
        const DayRoutes* routes = solution.routes(day);
        if (routes == nullptr) {
            continue;
        }
        write_routes_json(out_dir / ("routes_" + date + ".json"), *routes);
        std::vector<bool> collected(static_cast<std::size_t>(instance.node_count()), false);
        for (std::size_t r = 0; r < routes->size(); ++r) {
            const Route& route = routes->routes()[r];
            const auto s = statistics_of(instance, plan, simulation, day, route, collected);
            statistics += date + ';' + std::to_string(r) + ';' + std::to_string(route.truck) + ';' +
                          number(s.distance) + ';' + number(s.duration) + ';' + number(s.load) + ';' +
                          std::to_string(s.points) + '\n';
        }
    }
    write_text(out_dir / "collection_schedule.csv", schedule);
    write_text(out_dir / "route_statistics.csv", statistics);
}

}
