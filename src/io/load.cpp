#include <tlor/io.hpp>

#include <tlor/error.hpp>

#include "io/csv.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <map>
#include <utility>
#include <vector>

namespace tlor {

namespace {

[[noreturn]] void invalid(const std::string& message) { throw Error(Status::invalid_argument, message); }

struct Points {
    std::int32_t count = 0;
    Node depot = 0;
    std::vector<double> capacity_kg;
    std::vector<std::uint8_t> active;
};

Node point_id(const io::CsvTable& table, std::size_t row, std::size_t column, std::int32_t node_count) {
    const auto id = table.integer(row, column);
    if (id < 0 || id >= node_count) {
        invalid(table.where(row, column) + ": point " + std::to_string(id) + " is out of range [0, " +
                std::to_string(node_count) + ")");
    }
    return static_cast<Node>(id);
}

Points read_points(const std::filesystem::path& file) {
    const auto table = io::CsvTable::read(file);
    const auto id = table.column("point_id");
    const auto capacity = table.column("capacity_kg");
    const auto depot = table.column("is_depot");
    const auto active = table.column("active");
    if (table.row_count() == 0 || table.row_count() > static_cast<std::size_t>(std::numeric_limits<Node>::max())) {
        invalid(io::shown(file) + " has " + std::to_string(table.row_count()) + " points");
    }
    Points points;
    points.count = static_cast<std::int32_t>(table.row_count());
    points.capacity_kg.assign(table.row_count(), 0);
    points.active.assign(table.row_count(), 0);
    std::vector<bool> seen(table.row_count(), false);
    std::int32_t depots = 0;
    for (std::size_t row = 0; row < table.row_count(); ++row) {
        const auto node = point_id(table, row, id, points.count);
        const auto index = static_cast<std::size_t>(node);
        if (seen[index]) {
            invalid(table.where(row, id) + ": point " + std::to_string(node) + " appears twice");
        }
        seen[index] = true;
        points.capacity_kg[index] = table.number_or(row, capacity, 0);
        points.active[index] = table.boolean(row, active) ? 1 : 0;
        if (table.boolean(row, depot)) {
            points.depot = node;
            ++depots;
        }
    }
    if (depots != 1) {
        invalid(io::shown(file) + " has " + std::to_string(depots) + " depots, expected exactly one");
    }
    return points;
}

struct DailyDemand {
    std::chrono::sys_days first_date;
    std::int32_t day_count = 0;
    std::vector<double> kg;
};

DailyDemand read_demand(const std::filesystem::path& file, const Points& points) {
    const auto table = io::CsvTable::read(file);
    const auto id = table.column("point_id");
    const auto date = table.column("date");
    const auto kg = table.column("daily_kg");
    if (table.row_count() == 0) {
        invalid(io::shown(file) + " has no rows");
    }
    std::vector<std::chrono::sys_days> dates(table.row_count());
    for (std::size_t row = 0; row < table.row_count(); ++row) {
        try {
            dates[row] = parse_date(table.cell(row, date));
        } catch (const Error& e) {
            invalid(table.where(row, date) + ": " + e.what());
        }
    }
    std::vector<std::chrono::sys_days> days = dates;
    std::ranges::sort(days);
    days.erase(std::unique(days.begin(), days.end()), days.end());
    for (std::size_t i = 1; i < days.size(); ++i) {
        if (days[i] - days[i - 1] != std::chrono::days{1}) {
            invalid(io::shown(file) + ": dates are not contiguous, " + format_date(days[i - 1]) + " is followed by " +
                    format_date(days[i]));
        }
    }
    if (days.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max() / std::max(points.count, 1))) {
        invalid(io::shown(file) + " spans too many days");
    }
    DailyDemand demand{days.front(), static_cast<std::int32_t>(days.size()), {}};
    const auto n = static_cast<std::size_t>(points.count);
    demand.kg.assign(static_cast<std::size_t>(demand.day_count) * n, 0);
    std::vector<bool> cell_seen(demand.kg.size(), false);
    for (std::size_t row = 0; row < table.row_count(); ++row) {
        const auto node = point_id(table, row, id, points.count);
        if (points.active[static_cast<std::size_t>(node)] == 0) {
            invalid(table.where(row, id) + ": point " + std::to_string(node) + " is not an active point");
        }
        const auto day = static_cast<std::size_t>((dates[row] - demand.first_date).count());
        const auto index = day * n + static_cast<std::size_t>(node);
        if (cell_seen[index]) {
            invalid(table.where(row) + ": point " + std::to_string(node) + " appears twice on " +
                    format_date(dates[row]));
        }
        cell_seen[index] = true;
        demand.kg[index] = table.number(row, kg);
    }
    return demand;
}

std::vector<double> read_matrix(const std::filesystem::path& file, std::int32_t node_count) {
    const std::string text = io::read_text(file);
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(text);
    } catch (const nlohmann::json::exception& e) {
        invalid(io::shown(file) + " is not valid JSON: " + e.what());
    }
    const auto n = static_cast<std::size_t>(node_count);
    const auto expected = std::to_string(node_count) + "x" + std::to_string(node_count);
    if (!json.is_array() || json.size() != n) {
        invalid(io::shown(file) + " must be a " + expected + " matrix");
    }
    std::vector<double> values;
    values.reserve(n * n);
    for (std::size_t i = 0; i < n; ++i) {
        const auto& row = json[i];
        if (!row.is_array() || row.size() != n) {
            invalid(io::shown(file) + " must be a " + expected + " matrix, row " + std::to_string(i) + " is not");
        }
        for (const auto& value : row) {
            if (!value.is_number()) {
                invalid(io::shown(file) + " row " + std::to_string(i) + " holds a value that is not a number");
            }
            values.push_back(value.get<double>());
        }
    }
    return values;
}

std::vector<double> read_service(const std::filesystem::path& file, std::int32_t node_count) {
    const auto table = io::CsvTable::read(file);
    const auto column = table.column("service_time");
    if (table.row_count() != static_cast<std::size_t>(node_count)) {
        invalid(io::shown(file) + " has " + std::to_string(table.row_count()) + " values, expected " +
                std::to_string(node_count));
    }
    std::vector<double> service(table.row_count());
    for (std::size_t row = 0; row < table.row_count(); ++row) {
        service[row] = table.number(row, column);
    }
    return service;
}

std::vector<double> read_thresholds(const std::filesystem::path& file, std::int32_t node_count) {
    const auto table = io::CsvTable::read(file);
    const auto id = table.column("point_id");
    const auto threshold = table.column("threshold");
    std::vector<double> values(static_cast<std::size_t>(node_count), std::numeric_limits<double>::quiet_NaN());
    for (std::size_t row = 0; row < table.row_count(); ++row) {
        const auto node = point_id(table, row, id, node_count);
        values[static_cast<std::size_t>(node)] =
            table.number_or(row, threshold, std::numeric_limits<double>::quiet_NaN());
    }
    return values;
}

}

LoadedInstance load_instance(const LoadOptions& options) {
    auto points = read_points(options.tables_dir / "active_points.csv");
    auto demand = read_demand(options.tables_dir / "daily_demand.csv", points);
    auto distance = read_matrix(options.map_data_dir / "distances.json", points.count);
    auto duration = read_matrix(options.map_data_dir / "durations.json", points.count);
    const auto service_file = options.service_time_csv.empty()
                                  ? options.map_data_dir / "exemple_points_service_time.csv"
                                  : options.service_time_csv;
    auto service = read_service(service_file, points.count);
    std::vector<double> thresholds;
    if (options.load_point_policy) {
        thresholds = read_thresholds(options.tables_dir / "point_policy.csv", points.count);
    }
    Instance instance(InstanceData{
        .node_count = points.count,
        .depot = points.depot,
        .day_count = demand.day_count,
        .distance_m = std::move(distance),
        .duration_s = std::move(duration),
        .service_s = std::move(service),
        .capacity_kg = std::move(points.capacity_kg),
        .active = std::move(points.active),
        .demand_kg = std::move(demand.kg),
    });
    if (options.load_point_policy) {
        instance.set_attribute("threshold", std::move(thresholds));
    }
    return {std::move(instance), demand.first_date};
}

}
