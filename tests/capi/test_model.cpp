#include <doctest/doctest.h>

#include "capi/capi_support.hpp"

#include <cstring>
#include <string>
#include <vector>

extern "C" int tlor_header_check(void);

TEST_CASE("capi: header compiles as C11") { CHECK(tlor_header_check() == 0); }

TEST_CASE("capi: version and status names") {
    CHECK(std::string(tlor_version()) == "0.1.0");
    CHECK(std::string(tlor_status_name(TLOR_ERROR_INVALID_PARAM)) == "invalid_param");
    CHECK(std::string(tlor_status_name(static_cast<tlor_status>(42))) == "unknown");
    tlor_set_error("custom");
    CHECK(std::string(tlor_last_error()) == "custom");
}

TEST_CASE("capi: instance round trip and errors") {
    auto a = small_arrays();
    tlor_instance_desc d{a.node_count, a.depot, a.day_count, a.distance_m.data(), a.duration_s.data(),
                         a.service_s.data(), a.capacity_kg.data(), a.active.data(), a.demand_kg.data()};
    tlor_instance* inst = nullptr;
    REQUIRE(tlor_instance_create(&d, &inst) == TLOR_OK);
    CHECK(std::strlen(tlor_last_error()) == 0);
    CHECK(tlor_instance_node_count(inst) == 6);
    CHECK(tlor_instance_depot(inst) == 5);
    CHECK(tlor_instance_day_count(inst) == 7);
    CHECK(tlor_instance_distance_m(inst)[3 * 6 + 5] == 450);
    CHECK(tlor_instance_duration_s(inst)[5 * 6 + 3] == 40);
    CHECK(tlor_instance_service_s(inst)[5] == 120);
    CHECK(tlor_instance_capacity_kg(inst)[0] == 100);
    CHECK(tlor_instance_active(inst)[4] == 0);
    CHECK(tlor_instance_demand_kg(inst)[6 * 6 + 3] == 50);
    CHECK(tlor_instance_attribute(inst, "threshold") == nullptr);
    std::vector<double> threshold{0.9, 0.8, 0.8, 0.8, 0.8, 0.8};
    CHECK(tlor_instance_set_attribute(inst, "threshold", threshold.data()) == TLOR_OK);
    REQUIRE(tlor_instance_attribute(inst, "threshold") != nullptr);
    CHECK(tlor_instance_attribute(inst, "threshold")[0] == doctest::Approx(0.9));
    CHECK(tlor_instance_set_attribute(inst, nullptr, threshold.data()) == TLOR_ERROR_INVALID_ARGUMENT);
    d.depot = 9;
    tlor_instance* bad = nullptr;
    CHECK(tlor_instance_create(&d, &bad) == TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(bad == nullptr);
    CHECK(std::strlen(tlor_last_error()) > 0);
    CHECK(tlor_instance_create(&d, nullptr) == TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(tlor_instance_create(nullptr, &bad) == TLOR_ERROR_INVALID_ARGUMENT);
    d.depot = 5;
    d.distance_m = nullptr;
    CHECK(tlor_instance_create(&d, &bad) == TLOR_ERROR_INVALID_ARGUMENT);
    tlor_instance_destroy(inst);
    tlor_instance_destroy(nullptr);
}

TEST_CASE("capi: params") {
    tlor_params* p = nullptr;
    REQUIRE(tlor_params_create(&p) == TLOR_OK);
    CHECK(tlor_params_set_int(p, "b", 3) == TLOR_OK);
    CHECK(tlor_params_set_double(p, "a", 0.5) == TLOR_OK);
    CHECK(tlor_params_set_bool(p, "c", 1) == TLOR_OK);
    CHECK(tlor_params_set_string(p, "d", "distance") == TLOR_OK);
    CHECK(tlor_params_size(p) == 4);
    CHECK(std::string(tlor_params_key(p, 0)) == "a");
    CHECK(std::string(tlor_params_key(p, 1)) == "b");
    CHECK(tlor_params_key(p, 4) == nullptr);
    tlor_param_type type = TLOR_PARAM_STRING;
    CHECK(tlor_params_type(p, "b", &type) == TLOR_OK);
    CHECK(type == TLOR_PARAM_INT);
    CHECK(tlor_params_type(p, "zz", &type) == TLOR_ERROR_INVALID_PARAM);
    double real = 0;
    CHECK(tlor_params_get_double(p, "b", &real) == TLOR_OK);
    CHECK(real == 3.0);
    int64_t integer = 0;
    CHECK(tlor_params_get_int(p, "b", &integer) == TLOR_OK);
    CHECK(integer == 3);
    int flag = 0;
    CHECK(tlor_params_get_bool(p, "b", &flag) == TLOR_ERROR_INVALID_PARAM);
    CHECK(tlor_params_get_bool(p, "c", &flag) == TLOR_OK);
    CHECK(flag == 1);
    const char* text = nullptr;
    CHECK(tlor_params_get_string(p, "d", &text) == TLOR_OK);
    CHECK(std::string(text) == "distance");
    CHECK(tlor_params_set_int(nullptr, "x", 1) == TLOR_ERROR_INVALID_ARGUMENT);
    tlor_params_destroy(p);
    tlor_params_destroy(nullptr);
}

TEST_CASE("capi: fleet and state defaults") {
    const double capacity[1] = {150};
    const double work[1] = {3600};
    tlor_fleet* fleet = nullptr;
    REQUIRE(tlor_fleet_create(1, capacity, work, 3, nullptr, &fleet) == TLOR_OK);
    CHECK(tlor_fleet_truck_count(fleet) == 1);
    CHECK(tlor_fleet_day_count(fleet) == 3);
    CHECK(tlor_fleet_capacity_kg(fleet)[0] == 150);
    CHECK(tlor_fleet_max_work_s(fleet)[0] == 3600);
    CHECK(tlor_fleet_available(fleet)[2] == 1);
    tlor_fleet* bad = nullptr;
    CHECK(tlor_fleet_create(0, capacity, work, 3, nullptr, &bad) == TLOR_ERROR_INVALID_ARGUMENT);
    tlor_fleet_destroy(fleet);
    tlor_fleet_destroy(nullptr);
    tlor_state* state = nullptr;
    REQUIRE(tlor_state_create(3, nullptr, nullptr, &state) == TLOR_OK);
    CHECK(tlor_state_node_count(state) == 3);
    CHECK(tlor_state_level_kg(state)[2] == 0);
    CHECK(tlor_state_last_collection(state)[2] == -1);
    tlor_state_destroy(state);
    const double levels[2] = {5, 6};
    const int32_t last[2] = {0, -1};
    REQUIRE(tlor_state_create(2, levels, last, &state) == TLOR_OK);
    CHECK(tlor_state_level_kg(state)[1] == 6);
    CHECK(tlor_state_last_collection(state)[0] == 0);
    tlor_state_destroy(state);
    tlor_state_destroy(nullptr);
}

TEST_CASE("capi: plan, routes, solution") {
    tlor_plan* plan = nullptr;
    REQUIRE(tlor_plan_create(0, 2, &plan) == TLOR_OK);
    CHECK(tlor_plan_add(plan, 1, 3) == TLOR_OK);
    CHECK(tlor_plan_add(plan, 1, 0) == TLOR_OK);
    CHECK(tlor_plan_add(plan, 2, 0) == TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(tlor_plan_start(plan) == 0);
    CHECK(tlor_plan_horizon(plan) == 2);
    const int32_t* points = nullptr;
    size_t count = 0;
    REQUIRE(tlor_plan_points(plan, 1, &points, &count) == TLOR_OK);
    REQUIRE(count == 2);
    CHECK(points[0] == 0);
    CHECK(points[1] == 3);
    CHECK(tlor_plan_remove(plan, 1, 0) == TLOR_OK);
    tlor_plan* copy = nullptr;
    REQUIRE(tlor_plan_create(4, 1, &copy) == TLOR_OK);
    CHECK(tlor_plan_assign(copy, plan) == TLOR_OK);
    CHECK(tlor_plan_start(copy) == 0);
    REQUIRE(tlor_plan_points(copy, 1, &points, &count) == TLOR_OK);
    CHECK(count == 1);

    tlor_day_routes* routes = nullptr;
    REQUIRE(tlor_day_routes_create(1, &routes) == TLOR_OK);
    const int32_t nodes[3] = {5, 3, 5};
    CHECK(tlor_day_routes_add(routes, 0, nodes, 3) == TLOR_OK);
    CHECK(tlor_day_routes_day(routes) == 1);
    CHECK(tlor_day_routes_count(routes) == 1);
    int32_t truck = -1;
    const int32_t* got = nullptr;
    size_t length = 0;
    REQUIRE(tlor_day_routes_get(routes, 0, &truck, &got, &length) == TLOR_OK);
    CHECK(truck == 0);
    CHECK(length == 3);
    CHECK(got[1] == 3);
    CHECK(tlor_day_routes_get(routes, 1, &truck, &got, &length) == TLOR_ERROR_INVALID_ARGUMENT);

    tlor_solution* solution = nullptr;
    REQUIRE(tlor_solution_create(plan, &solution) == TLOR_OK);
    CHECK(tlor_solution_routes(solution, 1) == nullptr);
    CHECK(tlor_solution_set_routes(solution, routes) == TLOR_OK);
    const tlor_day_routes* borrowed = tlor_solution_routes(solution, 1);
    REQUIRE(borrowed != nullptr);
    CHECK(tlor_day_routes_day(borrowed) == 1);
    CHECK(tlor_day_routes_count(borrowed) == 1);
    const tlor_plan* solution_plan = tlor_solution_plan(solution);
    CHECK(tlor_plan_horizon(solution_plan) == 2);
    tlor_solution* other = nullptr;
    REQUIRE(tlor_solution_create(copy, &other) == TLOR_OK);
    CHECK(tlor_solution_assign(other, solution) == TLOR_OK);
    CHECK(tlor_solution_routes(other, 1) != nullptr);
    tlor_day_routes* empty = nullptr;
    REQUIRE(tlor_day_routes_create(1, &empty) == TLOR_OK);
    CHECK(tlor_day_routes_assign(empty, routes) == TLOR_OK);
    CHECK(tlor_day_routes_count(empty) == 1);

    tlor_solution_destroy(other);
    tlor_solution_destroy(solution);
    tlor_day_routes_destroy(empty);
    tlor_day_routes_destroy(routes);
    tlor_plan_destroy(copy);
    tlor_plan_destroy(plan);
    tlor_solution_destroy(nullptr);
    tlor_day_routes_destroy(nullptr);
    tlor_plan_destroy(nullptr);
}

TEST_CASE("capi: fleet arrays are contiguous per field") {
    const double capacity[2] = {150, 90};
    const double work[2] = {3600, 1800};
    tlor_fleet* fleet = nullptr;
    REQUIRE(tlor_fleet_create(2, capacity, work, 1, nullptr, &fleet) == TLOR_OK);
    CHECK(tlor_fleet_capacity_kg(fleet)[1] == 90);
    CHECK(tlor_fleet_max_work_s(fleet)[1] == 1800);
    tlor_fleet_destroy(fleet);
}
