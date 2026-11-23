#include <doctest/doctest.h>

#include <tlor/steps.hpp>

#include "support/synthetic.hpp"

#include <nlohmann/json.hpp>

#include <cmath>

using tlor::Status;
using tlor::test::ScopedMethod;
using tlor::test::status_of;

namespace {

tlor::Params no_params;

tlor::DayRoutes one_per_point(const tlor::RouteRequest& r) {
    tlor::DayRoutes out(r.day);
    for (tlor::Node p : r.points) {
        out.add({0, {r.instance.depot(), p, r.instance.depot()}});
    }
    return out;
}

}

TEST_CASE("steps: route validates inputs") {
    auto& reg = tlor::Registry::global();
    reg.add_route({"test_steps_route", "", {}}, one_per_point);
    ScopedMethod guard(tlor::Step::route, "test_steps_route");
    auto inst = tlor::test::small_instance();
    auto fleet = tlor::test::small_fleet();
    auto run = [&](tlor::Day day, std::vector<tlor::Node> points, std::vector<double> loads) {
        return status_of([&] {
            (void)tlor::route(inst, fleet, day, points, loads, "test_steps_route", no_params, no_params, nullptr);
        });
    };
    CHECK(run(1, {0, 1}, {10, 10}) == Status::ok);
    CHECK(run(1, {4}, {10}) == Status::invalid_argument);
    CHECK(run(1, {5}, {10}) == Status::invalid_argument);
    CHECK(run(1, {0, 0}, {10, 10}) == Status::invalid_argument);
    CHECK(run(1, {0, 1}, {10}) == Status::invalid_argument);
    CHECK(run(1, {0}, {NAN}) == Status::invalid_argument);
    CHECK(run(1, {0}, {-1}) == Status::invalid_argument);
    CHECK(run(7, {0}, {10}) == Status::invalid_argument);
    CHECK(run(-1, {0}, {10}) == Status::invalid_argument);
}

TEST_CASE("steps: route passes validated params") {
    std::int64_t seen = -1;
    tlor::Registry::global().add_route(
        {"test_steps_params", "", {{"k", tlor::ParamType::integer, std::int64_t{2}, std::nullopt, std::nullopt, ""}}},
        [&](const tlor::RouteRequest& r) {
            seen = r.params.get_int("k");
            tlor::DayRoutes out(r.day);
            out.add({0, {5, 0, 5}});
            return out;
        });
    ScopedMethod guard(tlor::Step::route, "test_steps_params");
    auto inst = tlor::test::small_instance();
    std::vector<tlor::Node> points{0};
    std::vector<double> loads{60};
    auto routes = tlor::route(inst, tlor::test::small_fleet(), 1, points, loads, "test_steps_params", no_params, no_params,
                              nullptr);
    CHECK(seen == 2);
    CHECK(routes.size() == 1);
    tlor::Params bad;
    bad.set("k", 1.5);
    CHECK(status_of([&] {
              (void)tlor::route(inst, tlor::test::small_fleet(), 1, points, loads, "test_steps_params", bad, no_params,
                                nullptr);
          }) == Status::invalid_param);
    tlor::Params bad_weights;
    bad_weights.set("distance_km", 1.0);
    CHECK(status_of([&] {
              (void)tlor::route(inst, tlor::test::small_fleet(), 1, points, loads, "test_steps_params", no_params,
                                bad_weights, nullptr);
          }) == Status::invalid_param);
}

TEST_CASE("steps: unknown method") {
    auto inst = tlor::test::small_instance();
    std::vector<tlor::Node> points{0};
    std::vector<double> loads{60};
    CHECK(status_of([&] {
              (void)tlor::route(inst, tlor::test::small_fleet(), 1, points, loads, "nope", no_params, no_params, nullptr);
          }) == Status::unknown_method);
}

TEST_CASE("steps: schedule clips the horizon") {
    tlor::Registry::global().add_schedule({"test_steps_schedule", "", {}}, [](const tlor::ScheduleRequest& r) {
        return tlor::Plan(r.start_day, r.horizon);
    });
    ScopedMethod guard(tlor::Step::schedule, "test_steps_schedule");
    auto inst = tlor::test::small_instance();
    auto plan = tlor::schedule(inst, tlor::test::small_fleet(), tlor::State::initial(6), 5, 7, "test_steps_schedule",
                               no_params, no_params, nullptr);
    CHECK(plan.start() == 5);
    CHECK(plan.horizon() == 2);
    CHECK(status_of([&] {
              (void)tlor::schedule(inst, tlor::test::small_fleet(), tlor::State::initial(6), 7, 1, "test_steps_schedule",
                                   no_params, no_params, nullptr);
          }) == Status::invalid_argument);
    CHECK(status_of([&] {
              (void)tlor::schedule(inst, tlor::test::small_fleet(), tlor::State::initial(6), 0, 0, "test_steps_schedule",
                                   no_params, no_params, nullptr);
          }) == Status::invalid_argument);
}

TEST_CASE("steps: fleet must match the instance") {
    tlor::Registry::global().add_schedule({"test_steps_fleet", "", {}}, [](const tlor::ScheduleRequest& r) {
        return tlor::Plan(r.start_day, r.horizon);
    });
    ScopedMethod guard(tlor::Step::schedule, "test_steps_fleet");
    auto inst = tlor::test::small_instance();
    tlor::Fleet short_fleet({{150, 3600}}, 3, {});
    CHECK(status_of([&] {
              (void)tlor::schedule(inst, short_fleet, tlor::State::initial(6), 0, 2, "test_steps_fleet", no_params,
                                   no_params, nullptr);
          }) == Status::invalid_argument);
}

TEST_CASE("steps: call recorded with indicators") {
    tlor::Registry::global().add_route(
        {"test_steps_report", "", {{"k", tlor::ParamType::integer, std::int64_t{2}, std::nullopt, std::nullopt, ""}}},
        [](const tlor::RouteRequest& r) {
            tlor::Phase p(r.report, "construction");
            tlor::DayRoutes out(r.day);
            out.add({0, {5, 0, 5}});
            return out;
        });
    ScopedMethod guard(tlor::Step::route, "test_steps_report");
    auto inst = tlor::test::small_instance();
    std::vector<tlor::Node> points{0};
    std::vector<double> loads{60};
    tlor::Report report;
    (void)tlor::route(inst, tlor::test::small_fleet(), 1, points, loads, "test_steps_report", no_params, no_params, &report);
    auto j = nlohmann::json::parse(report.to_json());
    REQUIRE(j["calls"].size() == 1);
    auto call = j["calls"][0];
    CHECK(call["step"] == "route");
    CHECK(call["method"] == "test_steps_report");
    CHECK(call["params"]["k"] == 2);
    CHECK(call["instance"]["nodes"] == 6);
    CHECK(call["instance"]["active_points"] == 4);
    CHECK(call["instance"]["days"] == 7);
    CHECK(call["instance"]["trucks"] == 2);
    CHECK(call["status"] == "ok");
    CHECK(call["indicators"]["distance_m"] == 250.0);
    CHECK(call["phases"][0]["name"] == "construction");
}

TEST_CASE("steps: method error ends the call") {
    tlor::Registry::global().add_route({"test_steps_error", "", {}}, [](const tlor::RouteRequest& r) -> tlor::DayRoutes {
        tlor::Phase p(r.report, "construction");
        throw tlor::Error(Status::infeasible, "x");
    });
    ScopedMethod guard(tlor::Step::route, "test_steps_error");
    auto inst = tlor::test::small_instance();
    std::vector<tlor::Node> points{0};
    std::vector<double> loads{60};
    tlor::Report report;
    CHECK(status_of([&] {
              (void)tlor::route(inst, tlor::test::small_fleet(), 1, points, loads, "test_steps_error", no_params,
                                no_params, &report);
          }) == Status::infeasible);
    CHECK_FALSE(report.in_call());
    auto call = nlohmann::json::parse(report.to_json())["calls"][0];
    CHECK(call["status"] == "infeasible");
    CHECK(call["indicators"].is_null());
}

TEST_CASE("steps: output range is checked") {
    tlor::Registry::global().add_schedule({"test_steps_range", "", {}},
                                          [](const tlor::ScheduleRequest&) { return tlor::Plan(0, 1); });
    ScopedMethod guard(tlor::Step::schedule, "test_steps_range");
    auto inst = tlor::test::small_instance();
    CHECK(status_of([&] {
              (void)tlor::schedule(inst, tlor::test::small_fleet(), tlor::State::initial(6), 2, 3, "test_steps_range",
                                   no_params, no_params, nullptr);
          }) == Status::internal_error);
}

TEST_CASE("steps: improve calls nested routing") {
    tlor::Registry::global().add_route({"test_steps_inner", "", {}}, one_per_point);
    ScopedMethod route_guard(tlor::Step::route, "test_steps_inner");
    tlor::Registry::global().add_improve({"test_steps_improve", "", {}}, [](const tlor::ImproveRequest& r) {
        tlor::Solution out = r.solution;
        const tlor::Plan& plan = out.plan();
        for (tlor::Day day = plan.start(); day < plan.end(); ++day) {
            auto points = plan.points(day);
            std::vector<double> loads(points.size(), 10.0);
            out.set_routes(tlor::route(r.instance, r.fleet, day, points, loads, r.route_method, r.route_params, r.weights,
                                       r.report));
        }
        return out;
    });
    ScopedMethod improve_guard(tlor::Step::improve, "test_steps_improve");
    auto inst = tlor::test::small_instance();
    tlor::Plan plan(0, 2);
    plan.add(1, 3);
    tlor::Report report;
    auto result = tlor::improve(inst, tlor::test::small_fleet(), tlor::State::initial(6), tlor::Solution(plan),
                                "test_steps_inner", no_params, "test_steps_improve", no_params, no_params, &report);
    REQUIRE(result.routes(1) != nullptr);
    CHECK(result.routes(1)->size() == 1);
    auto call = nlohmann::json::parse(report.to_json())["calls"][0];
    CHECK(call["step"] == "improve");
    CHECK(call["phases"][0]["name"] == "call:route/test_steps_inner");
    CHECK(call["phases"][0]["count"] == 2);
    CHECK(call["indicators"]["distance_m"] == 850.0);
    CHECK(status_of([&] {
              (void)tlor::improve(inst, tlor::test::small_fleet(), tlor::State::initial(6), tlor::Solution(plan), "nope",
                                  no_params, "test_steps_improve", no_params, no_params, nullptr);
          }) == Status::unknown_method);
}
