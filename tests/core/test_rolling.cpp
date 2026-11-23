#include <doctest/doctest.h>

#include <tlor/pipeline.hpp>

#include "support/synthetic.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>

using tlor::Status;
using tlor::test::status_of;
using V = std::vector<tlor::Node>;

namespace {

tlor::RollingOptions options(tlor::Day first, tlor::Day last, std::int32_t horizon, std::int32_t step) {
    tlor::RollingOptions o;
    o.first_day = first;
    o.last_day = last;
    o.horizon = horizon;
    o.step = step;
    o.schedule = {"threshold", {}};
    o.route = {"nearest_neighbour", {}};
    return o;
}

tlor::RollingResult run(const tlor::RollingOptions& o, const tlor::Demand* realized = nullptr,
                        tlor::Report* report = nullptr) {
    auto inst = tlor::test::small_instance();
    return tlor::run_rolling(inst, tlor::test::small_fleet(), tlor::State::initial(6), o, realized, report);
}

}

TEST_CASE("rolling: day by day equals the full threshold plan") {
    auto result = run(options(0, 6, 3, 1));
    const auto& plan = result.solution.plan();
    CHECK(plan.start() == 0);
    CHECK(plan.horizon() == 7);
    CHECK(std::ranges::equal(plan.points(0), V{}));
    CHECK(std::ranges::equal(plan.points(1), V{3}));
    CHECK(std::ranges::equal(plan.points(2), V{0}));
    CHECK(std::ranges::equal(plan.points(3), V{3}));
    CHECK(std::ranges::equal(plan.points(4), V{0, 2, 3}));
    CHECK(std::ranges::equal(plan.points(5), V{}));
    CHECK(std::ranges::equal(plan.points(6), V{3}));
    CHECK(result.violations.empty());
    CHECK(result.evaluation.total.at("overflow_point_days") == 0);
    for (tlor::Day day = 0; day < 7; ++day) {
        if (!plan.points(day).empty()) {
            CHECK(result.solution.routes(day) != nullptr);
        }
    }
    CHECK(result.end_state.last_collection[3] == 6);
}

TEST_CASE("rolling: realized demand above the forecast") {
    auto inst = tlor::test::small_instance();
    std::vector<double> more(inst.demand().data().begin(), inst.demand().data().end());
    for (auto& v : more) {
        v *= 1.2;
    }
    tlor::Demand realized(7, 6, more);
    auto result = run(options(0, 6, 3, 1), &realized);
    CHECK_FALSE(result.violations.empty());
    for (const auto& v : result.violations) {
        CHECK((v.kind == tlor::ViolationKind::overflow || v.kind == tlor::ViolationKind::capacity));
    }
    CHECK(result.evaluation.total.at("overflow_point_days") >= 1);
}

TEST_CASE("rolling: improve never worsens a single window") {
    tlor::Params weights;
    weights.set("distance_m", 1.0);
    weights.set("overflow_point_days", 1e6);
    auto plain = options(0, 6, 7, 7);
    plain.weights = weights;
    auto improved = plain;
    improved.improve = tlor::MethodChoice{"shift_day", {}};
    const double without = tlor::objective(run(plain).evaluation.total, weights);
    const double with = tlor::objective(run(improved).evaluation.total, weights);
    CHECK(with <= without);
}

TEST_CASE("rolling: invalid ranges") {
    CHECK(status_of([] { (void)run(options(0, 7, 3, 1)); }) == Status::invalid_argument);
    CHECK(status_of([] { (void)run(options(3, 2, 3, 1)); }) == Status::invalid_argument);
    CHECK(status_of([] { (void)run(options(0, 6, 3, 0)); }) == Status::invalid_argument);
    CHECK(status_of([] { (void)run(options(0, 6, 2, 3)); }) == Status::invalid_argument);
    CHECK(status_of([] { (void)run(options(-1, 6, 3, 1)); }) == Status::invalid_argument);
    tlor::Demand wrong(3, 6, std::vector<double>(18, 1.0));
    CHECK(status_of([&] { (void)run(options(0, 6, 3, 1), &wrong); }) == Status::invalid_argument);
}

TEST_CASE("rolling: one call record") {
    tlor::Report report;
    auto o = options(0, 6, 3, 1);
    o.check_each_step = true;
    (void)run(o, nullptr, &report);
    auto j = nlohmann::json::parse(report.to_json());
    REQUIRE(j["calls"].size() == 1);
    auto call = j["calls"][0];
    CHECK(call["step"] == "rolling");
    CHECK(call["method"] == "threshold+nearest_neighbour");
    CHECK(call["params"]["horizon"] == 3);
    CHECK(call["status"] == "ok");
    CHECK(call["indicators"]["overflow_point_days"] == 0.0);
    nlohmann::json schedule;
    for (const auto& p : call["phases"]) {
        if (p["name"] == "schedule") {
            schedule = p;
        }
    }
    REQUIRE_FALSE(schedule.is_null());
    CHECK(schedule["children"][0]["name"] == "call:schedule/threshold");
    CHECK(schedule["children"][0]["count"] == 7);
}

TEST_CASE("rolling: call record holds method parameters, weights and seed") {
    auto o = options(0, 6, 7, 7);
    o.route.params.set("criterion", std::string("duration"));
    tlor::Params improve;
    improve.set("max_iterations", std::int64_t{5});
    improve.set("seed", std::int64_t{3});
    o.improve = tlor::MethodChoice{"shift_day", improve};
    o.weights.set("distance_m", 1.0);
    tlor::Report report;
    (void)run(o, nullptr, &report);
    auto call = nlohmann::json::parse(report.to_json())["calls"][0];
    CHECK(call["method"] == "threshold+nearest_neighbour+shift_day");
    CHECK(call["params"]["schedule.threshold"] == 0.85);
    CHECK(call["params"]["route.criterion"] == "duration");
    CHECK(call["params"]["improve.max_iterations"] == 5);
    CHECK(call["params"]["improve.max_shift"] == 1);
    CHECK(call["params"]["weight.distance_m"] == 1.0);
    CHECK(call["seed"] == 3);
}

TEST_CASE("rolling: bad method parameters fail before any step") {
    auto o = options(0, 6, 3, 1);
    o.schedule.params.set("threshold", 2.0);
    tlor::Report report;
    CHECK(status_of([&] { (void)run(o, nullptr, &report); }) == Status::invalid_param);
    CHECK(nlohmann::json::parse(report.to_json())["calls"].empty());
    auto unknown = options(0, 6, 3, 1);
    unknown.route = {"nope", {}};
    CHECK(status_of([&] { (void)run(unknown); }) == Status::unknown_method);
    auto weights = options(0, 6, 3, 1);
    weights.weights.set("distance_km", 1.0);
    CHECK(status_of([&] { (void)run(weights); }) == Status::invalid_param);
}
