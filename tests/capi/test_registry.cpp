#include <doctest/doctest.h>

#include "capi/capi_support.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace {

struct Calls {
    int count = 0;
    int64_t k = -1;
};

tlor_status one_route_per_point(const tlor_route_request* request, tlor_day_routes* out, void* user_data) {
    auto* calls = static_cast<Calls*>(user_data);
    ++calls->count;
    tlor_params_get_int(tlor_route_request_params(request), "k", &calls->k);
    const int32_t* points = nullptr;
    const size_t count = tlor_route_request_points(request, &points);
    const int32_t depot = tlor_instance_depot(tlor_route_request_instance(request));
    for (size_t i = 0; i < count; ++i) {
        const int32_t nodes[3] = {depot, points[i], depot};
        tlor_day_routes_add(out, 0, nodes, 3);
    }
    return TLOR_OK;
}

tlor_status failing_route(const tlor_route_request*, tlor_day_routes*, void*) {
    tlor_set_error("boom");
    return TLOR_ERROR_CALLBACK;
}

tlor_status point_three_every_day(const tlor_schedule_request* request, tlor_plan* out, void*) {
    const int32_t start = tlor_schedule_request_start_day(request);
    for (int32_t day = start; day < start + tlor_schedule_request_horizon(request); ++day) {
        tlor_plan_add(out, day, 3);
    }
    return TLOR_OK;
}

std::vector<std::string> names(tlor_step step) {
    std::vector<std::string> out;
    for (size_t i = 0; i < tlor_method_count(step); ++i) {
        out.emplace_back(tlor_method_name(step, i));
    }
    return out;
}

bool contains(const std::vector<std::string>& list, const std::string& name) {
    return std::find(list.begin(), list.end(), name) != list.end();
}

tlor_param_spec k_spec() {
    tlor_param_spec spec{};
    spec.name = "k";
    spec.type = TLOR_PARAM_INT;
    spec.default_int = 2;
    spec.description = "test parameter";
    return spec;
}

}

TEST_CASE("capi: builtin methods listed") {
    CHECK(contains(names(TLOR_STEP_SCHEDULE), "threshold"));
    CHECK(contains(names(TLOR_STEP_ROUTE), "nearest_neighbour"));
    CHECK(contains(names(TLOR_STEP_IMPROVE), "shift_day"));
    CHECK(tlor_method_name(TLOR_STEP_ROUTE, 1000) == nullptr);
    CHECK(std::string(tlor_method_description(TLOR_STEP_SCHEDULE, "threshold")).size() > 0);
    CHECK(tlor_method_description(TLOR_STEP_SCHEDULE, "nope") == nullptr);
    size_t count = 0;
    REQUIRE(tlor_method_param_count(TLOR_STEP_ROUTE, "nearest_neighbour", &count) == TLOR_OK);
    REQUIRE(count == 1);
    tlor_param_spec spec{};
    REQUIRE(tlor_method_param(TLOR_STEP_ROUTE, "nearest_neighbour", 0, &spec) == TLOR_OK);
    CHECK(std::string(spec.name) == "criterion");
    CHECK(spec.type == TLOR_PARAM_STRING);
    CHECK(std::string(spec.default_string) == "distance");
    REQUIRE(tlor_method_param(TLOR_STEP_SCHEDULE, "threshold", 0, &spec) == TLOR_OK);
    CHECK(spec.type == TLOR_PARAM_DOUBLE);
    CHECK(spec.default_double == doctest::Approx(0.85));
    CHECK(spec.has_max == 1);
    CHECK(spec.max == 1.0);
    CHECK(tlor_method_param(TLOR_STEP_SCHEDULE, "threshold", 5, &spec) == TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(tlor_method_param_count(TLOR_STEP_SCHEDULE, "nope", &count) == TLOR_ERROR_UNKNOWN_METHOD);
}

TEST_CASE("capi: external route method") {
    Calls calls;
    const tlor_param_spec spec = k_spec();
    REQUIRE(tlor_register_route_method("test_c_route", "one route per point", &spec, 1, one_route_per_point, &calls) ==
            TLOR_OK);
    size_t count = 0;
    REQUIRE(tlor_method_param_count(TLOR_STEP_ROUTE, "test_c_route", &count) == TLOR_OK);
    CHECK(count == 1);
    tlor_instance* inst = small_instance_handle();
    tlor_fleet* fleet = small_fleet_handle();
    const int32_t points[4] = {0, 1, 2, 3};
    const double loads[4] = {60, 20, 40, 100};
    tlor_day_routes* routes = nullptr;
    REQUIRE(tlor_route(inst, fleet, 1, points, loads, 4, "test_c_route", nullptr, nullptr, nullptr, &routes) == TLOR_OK);
    CHECK(tlor_day_routes_count(routes) == 4);
    CHECK(calls.count == 1);
    CHECK(calls.k == 2);
    CHECK(tlor_unregister_method(TLOR_STEP_ROUTE, "test_c_route") == TLOR_OK);
    tlor_day_routes* again = nullptr;
    CHECK(tlor_route(inst, fleet, 1, points, loads, 4, "test_c_route", nullptr, nullptr, nullptr, &again) ==
          TLOR_ERROR_UNKNOWN_METHOD);
    tlor_day_routes_destroy(routes);
    tlor_fleet_destroy(fleet);
    tlor_instance_destroy(inst);
}

TEST_CASE("capi: callback error") {
    REQUIRE(tlor_register_route_method("test_c_failing", "", nullptr, 0, failing_route, nullptr) == TLOR_OK);
    tlor_instance* inst = small_instance_handle();
    tlor_fleet* fleet = small_fleet_handle();
    const int32_t points[1] = {0};
    const double loads[1] = {10};
    tlor_day_routes* routes = nullptr;
    CHECK(tlor_route(inst, fleet, 1, points, loads, 1, "test_c_failing", nullptr, nullptr, nullptr, &routes) ==
          TLOR_ERROR_CALLBACK);
    CHECK(std::string(tlor_last_error()).find("boom") != std::string::npos);
    CHECK(tlor_route(inst, fleet, 1, points, loads, 1, "nearest_neighbour", nullptr, nullptr, nullptr, &routes) ==
          TLOR_OK);
    tlor_day_routes_destroy(routes);
    tlor_unregister_method(TLOR_STEP_ROUTE, "test_c_failing");
    tlor_fleet_destroy(fleet);
    tlor_instance_destroy(inst);
}

TEST_CASE("capi: external route inside shift_day") {
    Calls calls;
    const tlor_param_spec spec = k_spec();
    REQUIRE(tlor_register_route_method("test_c_inner", "", &spec, 1, one_route_per_point, &calls) == TLOR_OK);
    tlor_instance* inst = small_instance_handle();
    tlor_fleet* fleet = small_fleet_handle();
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
    CHECK(tlor_improve(inst, fleet, nullptr, input, "test_c_inner", nullptr, "shift_day", nullptr, weights, nullptr,
                       &output) == TLOR_OK);
    CHECK(calls.count >= 2);
    tlor_solution_destroy(output);
    tlor_params_destroy(weights);
    tlor_solution_destroy(input);
    tlor_plan_destroy(plan);
    tlor_unregister_method(TLOR_STEP_ROUTE, "test_c_inner");
    tlor_fleet_destroy(fleet);
    tlor_instance_destroy(inst);
}

TEST_CASE("capi: external schedule in run_rolling") {
    REQUIRE(tlor_register_schedule_method("test_c_schedule", "", nullptr, 0, point_three_every_day, nullptr) == TLOR_OK);
    tlor_instance* inst = small_instance_handle();
    tlor_fleet* fleet = small_fleet_handle();
    tlor_rolling_options options{};
    options.first_day = 0;
    options.last_day = 2;
    options.horizon = 3;
    options.step = 1;
    options.schedule_method = "test_c_schedule";
    options.route_method = "nearest_neighbour";
    tlor_rolling_result* result = nullptr;
    REQUIRE(tlor_run_rolling(inst, fleet, nullptr, &options, nullptr, nullptr, &result) == TLOR_OK);
    const tlor_plan* plan = tlor_solution_plan(tlor_rolling_result_solution(result));
    for (int32_t day = 0; day <= 2; ++day) {
        const int32_t* points = nullptr;
        size_t count = 0;
        REQUIRE(tlor_plan_points(plan, day, &points, &count) == TLOR_OK);
        REQUIRE(count == 1);
        CHECK(points[0] == 3);
    }
    tlor_rolling_result_destroy(result);
    tlor_unregister_method(TLOR_STEP_SCHEDULE, "test_c_schedule");
    tlor_fleet_destroy(fleet);
    tlor_instance_destroy(inst);
}

TEST_CASE("capi: registration errors") {
    REQUIRE(tlor_register_route_method("test_c_dup", "", nullptr, 0, failing_route, nullptr) == TLOR_OK);
    CHECK(tlor_register_route_method("test_c_dup", "", nullptr, 0, failing_route, nullptr) == TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(tlor_unregister_method(TLOR_STEP_ROUTE, "test_c_dup") == TLOR_OK);
    CHECK(tlor_unregister_method(TLOR_STEP_ROUTE, "nearest_neighbour") == TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(tlor_unregister_method(TLOR_STEP_ROUTE, "nope") == TLOR_ERROR_UNKNOWN_METHOD);
    CHECK(tlor_register_route_method("test_c_null", "", nullptr, 0, nullptr, nullptr) == TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(tlor_unregister_method(static_cast<tlor_step>(9), "x") == TLOR_ERROR_INVALID_ARGUMENT);
}
