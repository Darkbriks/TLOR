#include <doctest/doctest.h>

#include "capi/capi_support.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Handles {
    tlor_instance* inst = small_instance_handle();
    tlor_fleet* fleet = small_fleet_handle();
    ~Handles() {
        tlor_fleet_destroy(fleet);
        tlor_instance_destroy(inst);
    }
};

std::filesystem::path temp_dir() {
    std::random_device rd;
    auto dir = std::filesystem::temp_directory_path() / ("tlor-capi-" + std::to_string(rd()));
    std::filesystem::create_directories(dir);
    return dir;
}

std::vector<int32_t> plan_points(const tlor_plan* plan, int32_t day) {
    const int32_t* points = nullptr;
    size_t count = 0;
    REQUIRE(tlor_plan_points(plan, day, &points, &count) == TLOR_OK);
    return std::vector<int32_t>(points, points + count);
}

tlor_solution* week_solution() {
    tlor_plan* plan = nullptr;
    tlor_plan_create(0, 7, &plan);
    tlor_plan_add(plan, 2, 0);
    tlor_solution* solution = nullptr;
    tlor_solution_create(plan, &solution);
    tlor_day_routes* routes = nullptr;
    tlor_day_routes_create(2, &routes);
    const int32_t nodes[3] = {5, 0, 5};
    tlor_day_routes_add(routes, 0, nodes, 3);
    tlor_solution_set_routes(solution, routes);
    tlor_day_routes_destroy(routes);
    tlor_plan_destroy(plan);
    return solution;
}

}

TEST_CASE("capi: route nearest_neighbour") {
    Handles h;
    const int32_t points[4] = {0, 1, 2, 3};
    const double loads[4] = {60, 20, 40, 100};
    tlor_day_routes* routes = nullptr;
    REQUIRE(tlor_route(h.inst, h.fleet, 1, points, loads, 4, "nearest_neighbour", nullptr, nullptr, nullptr, &routes) ==
            TLOR_OK);
    REQUIRE(tlor_day_routes_count(routes) == 2);
    int32_t truck = -1;
    const int32_t* nodes = nullptr;
    size_t count = 0;
    REQUIRE(tlor_day_routes_get(routes, 0, &truck, &nodes, &count) == TLOR_OK);
    CHECK(truck == 0);
    CHECK(std::vector<int32_t>(nodes, nodes + count) == std::vector<int32_t>{5, 0, 1, 2, 5});
    tlor_day_routes_destroy(routes);
}

TEST_CASE("capi: unknown method") {
    Handles h;
    const int32_t points[1] = {0};
    const double loads[1] = {10};
    tlor_day_routes* routes = nullptr;
    CHECK(tlor_route(h.inst, h.fleet, 1, points, loads, 1, "nope", nullptr, nullptr, nullptr, &routes) ==
          TLOR_ERROR_UNKNOWN_METHOD);
    CHECK(std::string(tlor_last_error()).find("nope") != std::string::npos);
    CHECK(routes == nullptr);
    CHECK(tlor_route(h.inst, h.fleet, 1, points, loads, 1, nullptr, nullptr, nullptr, nullptr, &routes) ==
          TLOR_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("capi: schedule threshold") {
    Handles h;
    tlor_plan* plan = nullptr;
    REQUIRE(tlor_schedule(h.inst, h.fleet, nullptr, 0, 7, "threshold", nullptr, nullptr, nullptr, &plan) == TLOR_OK);
    CHECK(plan_points(plan, 4) == std::vector<int32_t>{0, 2, 3});
    tlor_params* params = nullptr;
    tlor_params_create(&params);
    tlor_params_set_double(params, "threshold", 2.0);
    tlor_plan* bad = nullptr;
    CHECK(tlor_schedule(h.inst, h.fleet, nullptr, 0, 7, "threshold", params, nullptr, nullptr, &bad) ==
          TLOR_ERROR_INVALID_PARAM);
    tlor_params_destroy(params);
    tlor_plan_destroy(plan);
}

TEST_CASE("capi: improve shift_day") {
    Handles h;
    tlor_plan* plan = nullptr;
    tlor_plan_create(0, 2, &plan);
    tlor_plan_add(plan, 0, 0);
    tlor_plan_add(plan, 1, 1);
    tlor_solution* input = nullptr;
    tlor_solution_create(plan, &input);
    tlor_params* weights = nullptr;
    tlor_params_create(&weights);
    tlor_params_set_double(weights, "distance_m", 1.0);
    tlor_solution* output = nullptr;
    REQUIRE(tlor_improve(h.inst, h.fleet, nullptr, input, "nearest_neighbour", nullptr, "shift_day", nullptr, weights,
                         nullptr, &output) == TLOR_OK);
    const tlor_plan* result = tlor_solution_plan(output);
    CHECK(plan_points(result, 0).size() + plan_points(result, 1).size() == 2);
    CHECK((plan_points(result, 0).empty() || plan_points(result, 1).empty()));
    tlor_solution_destroy(output);
    tlor_params_destroy(weights);
    tlor_solution_destroy(input);
    tlor_plan_destroy(plan);
}

TEST_CASE("capi: simulate levels") {
    Handles h;
    tlor_plan* plan = nullptr;
    tlor_plan_create(0, 7, &plan);
    tlor_plan_add(plan, 2, 0);
    std::vector<double> levels(7 * 6, -1.0);
    tlor_state* end = nullptr;
    REQUIRE(tlor_simulate(h.inst, nullptr, 0, 7, plan, nullptr, &end, levels.data()) == TLOR_OK);
    CHECK(levels[2 * 6 + 0] == 90);
    CHECK(tlor_state_level_kg(end)[3] == 350);
    CHECK(tlor_state_last_collection(end)[0] == 2);
    std::vector<double> doubled(tlor_instance_demand_kg(h.inst), tlor_instance_demand_kg(h.inst) + 42);
    for (auto& v : doubled) {
        v *= 2;
    }
    REQUIRE(tlor_simulate(h.inst, nullptr, 0, 3, plan, doubled.data(), nullptr, levels.data()) == TLOR_OK);
    CHECK(levels[2 * 6 + 0] == 180);
    tlor_state_destroy(end);
    tlor_plan_destroy(plan);
}

TEST_CASE("capi: evaluate and objective") {
    Handles h;
    tlor_solution* solution = week_solution();
    tlor_evaluation* evaluation = nullptr;
    REQUIRE(tlor_evaluate(h.inst, h.fleet, nullptr, solution, nullptr, nullptr, &evaluation) == TLOR_OK);
    const tlor_indicators* total = tlor_evaluation_total(evaluation);
    double value = 0;
    REQUIRE(tlor_indicators_get(total, "distance_m", &value) == TLOR_OK);
    CHECK(value == 250);
    CHECK(tlor_indicators_get(total, "distance_km", &value) == TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(tlor_evaluation_start_day(evaluation) == 0);
    CHECK(tlor_evaluation_day_count(evaluation) == 7);
    REQUIRE(tlor_indicators_get(tlor_evaluation_day(evaluation, 2), "collections", &value) == TLOR_OK);
    CHECK(value == 1);
    CHECK(tlor_evaluation_day(evaluation, 7) == nullptr);
    tlor_params* weights = nullptr;
    tlor_params_create(&weights);
    tlor_params_set_double(weights, "distance_m", 1.0);
    tlor_params_set_int(weights, "overflow_point_days", 1000);
    double objective = 0;
    REQUIRE(tlor_objective(total, weights, &objective) == TLOR_OK);
    CHECK(objective == 8250);
    CHECK(tlor_indicator_count() == 14);
    CHECK(std::string(tlor_indicator_name(0)) == "distance_m");
    CHECK(tlor_indicator_name(14) == nullptr);
    tlor_params_destroy(weights);
    tlor_evaluation_destroy(evaluation);
    tlor_solution_destroy(solution);
}

TEST_CASE("capi: check") {
    Handles h;
    tlor_plan* plan = nullptr;
    tlor_plan_create(0, 3, &plan);
    tlor_solution* solution = nullptr;
    tlor_solution_create(plan, &solution);
    tlor_violations* violations = nullptr;
    REQUIRE(tlor_check(h.inst, h.fleet, nullptr, solution, nullptr, &violations) == TLOR_OK);
    REQUIRE(tlor_violations_count(violations) == 1);
    tlor_violation v{};
    REQUIRE(tlor_violations_get(violations, 0, &v) == TLOR_OK);
    CHECK(std::string(tlor_violation_kind_name(v.kind)) == "overflow");
    CHECK(v.day == 2);
    CHECK(v.point == 3);
    CHECK(v.route == -1);
    CHECK(std::string(v.message).size() > 0);
    CHECK(tlor_violations_get(violations, 1, &v) == TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(std::string(tlor_violation_kind_name(99)) == "unknown");
    tlor_violations_destroy(violations);
    tlor_solution_destroy(solution);
    tlor_plan_destroy(plan);
}

TEST_CASE("capi: run_rolling") {
    Handles h;
    tlor_rolling_options options{};
    options.first_day = 0;
    options.last_day = 6;
    options.horizon = 3;
    options.step = 1;
    options.schedule_method = "threshold";
    options.route_method = "nearest_neighbour";
    tlor_rolling_result* result = nullptr;
    REQUIRE(tlor_run_rolling(h.inst, h.fleet, nullptr, &options, nullptr, nullptr, &result) == TLOR_OK);
    CHECK(tlor_violations_count(tlor_rolling_result_violations(result)) == 0);
    CHECK(plan_points(tlor_solution_plan(tlor_rolling_result_solution(result)), 4) == std::vector<int32_t>{0, 2, 3});
    double overflow = -1;
    REQUIRE(tlor_indicators_get(tlor_evaluation_total(tlor_rolling_result_evaluation(result)), "overflow_point_days",
                                &overflow) == TLOR_OK);
    CHECK(overflow == 0);
    CHECK(tlor_state_last_collection(tlor_rolling_result_end_state(result))[3] == 6);
    tlor_rolling_result_destroy(result);
    options.horizon = 0;
    CHECK(tlor_run_rolling(h.inst, h.fleet, nullptr, &options, nullptr, nullptr, &result) == TLOR_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("capi: report") {
    Handles h;
    tlor_report* report = nullptr;
    REQUIRE(tlor_report_create(&report) == TLOR_OK);
    CHECK(tlor_report_set_context(report, "experiment", "capi") == TLOR_OK);
    CHECK(tlor_report_phase_begin(report, "mine") == TLOR_OK);
    CHECK(tlor_report_count(report, "steps", 2) == TLOR_OK);
    CHECK(tlor_report_phase_end(report) == TLOR_OK);
    CHECK(tlor_report_phase_end(report) == TLOR_ERROR_INVALID_ARGUMENT);
    const int32_t points[1] = {0};
    const double loads[1] = {60};
    tlor_day_routes* routes = nullptr;
    REQUIRE(tlor_route(h.inst, h.fleet, 1, points, loads, 1, "nearest_neighbour", nullptr, nullptr, report, &routes) ==
            TLOR_OK);
    auto dir = temp_dir();
    auto path = dir / "report.json";
    REQUIRE(tlor_report_write_json(report, path.string().c_str()) == TLOR_OK);
    std::stringstream text;
    text << std::ifstream(path).rdbuf();
    CHECK(text.str().find("\"schema_version\": 1") != std::string::npos);
    CHECK(text.str().find("\"step\": \"route\"") != std::string::npos);
    CHECK(text.str().find("\"experiment\": \"capi\"") != std::string::npos);
    auto missing = dir / "missing" / "report.json";
    CHECK(tlor_report_write_json(report, missing.string().c_str()) == TLOR_ERROR_IO);
    CHECK(tlor_report_phase_begin(nullptr, "x") == TLOR_OK);
    CHECK(tlor_report_phase_end(nullptr) == TLOR_OK);
    CHECK(tlor_report_count(nullptr, "x", 1) == TLOR_OK);
    tlor_day_routes_destroy(routes);
    tlor_report_destroy(report);
    tlor_report_destroy(nullptr);
    std::filesystem::remove_all(dir);
}

TEST_CASE("capi: report paths are utf-8") {
    tlor_report* report = nullptr;
    REQUIRE(tlor_report_create(&report) == TLOR_OK);
    auto dir = temp_dir() / std::filesystem::path(u8"clément");
    std::filesystem::create_directories(dir);
    const std::u8string file = (dir / "report.json").u8string();
    REQUIRE(tlor_report_write_json(report, reinterpret_cast<const char*>(file.c_str())) == TLOR_OK);
    CHECK(std::filesystem::exists(dir / "report.json"));
    const std::u8string missing = (dir / "absent" / "report.json").u8string();
    CHECK(tlor_report_write_json(report, reinterpret_cast<const char*>(missing.c_str())) == TLOR_ERROR_IO);
    CHECK(std::string(tlor_last_error()).find("cl\xc3\xa9ment") != std::string::npos);
    tlor_report_destroy(report);
    std::filesystem::remove_all(dir.parent_path());
}
