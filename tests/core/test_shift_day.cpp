#include <doctest/doctest.h>

#include <tlor/evaluate.hpp>
#include <tlor/steps.hpp>

#include "support/synthetic.hpp"

#include <nlohmann/json.hpp>

namespace {

tlor::Params none;

tlor::Solution run(const tlor::Solution& input, const tlor::State& start, const tlor::Params& weights,
                   const tlor::Params& params = none, tlor::Report* report = nullptr) {
    auto inst = tlor::test::small_instance();
    return tlor::improve(inst, tlor::test::small_fleet(), start, input, "nearest_neighbour", none, "shift_day", params,
                         weights, report);
}

tlor::Solution two_days() {
    tlor::Plan plan(0, 2);
    plan.add(0, 0);
    plan.add(1, 1);
    return tlor::Solution(plan);
}

nlohmann::json phase(const nlohmann::json& phases, const std::string& name) {
    for (const auto& p : phases) {
        if (p["name"] == name) {
            return p;
        }
    }
    return nullptr;
}

}

TEST_CASE("shift_day: merges two days when it saves distance") {
    tlor::Params weights;
    weights.set("distance_m", 1.0);
    auto result = run(two_days(), tlor::State::initial(6), weights);
    const auto& plan = result.plan();
    const bool on_day_0 = plan.collected(0, 0) && plan.collected(0, 1);
    const bool on_day_1 = plan.collected(1, 0) && plan.collected(1, 1);
    CHECK((on_day_0 || on_day_1));
    CHECK(plan.collection_count() == 2);
    auto inst = tlor::test::small_instance();
    auto e = tlor::evaluate(inst, tlor::test::small_fleet(), tlor::State::initial(6), result, inst.demand(), {});
    CHECK(e.total.at("distance_m") == 450);
}

TEST_CASE("shift_day: refuses a move that adds an overflow") {
    tlor::Plan plan(0, 2);
    plan.add(0, 0);
    auto start = tlor::State::initial(6);
    start.level_kg[0] = 80;
    tlor::Params weights;
    weights.set("distance_m", 1.0);
    weights.set("overflow_point_days", 1e6);
    tlor::Report report;
    auto result = run(tlor::Solution(plan), start, weights, none, &report);
    CHECK(result.plan() == plan);
    auto call = nlohmann::json::parse(report.to_json())["calls"][0];
    auto search = phase(call["phases"], "search");
    REQUIRE_FALSE(search.is_null());
    CHECK(search["counters"]["moves_evaluated"] == 1);
    CHECK(search["counters"]["moves_accepted"] == 0);
    auto initial = phase(call["phases"], "initial_routing");
    REQUIRE_FALSE(initial.is_null());
    CHECK(initial["children"][0]["name"] == "call:route/nearest_neighbour");
}

TEST_CASE("shift_day: never targets a day without truck") {
    tlor::Plan plan(4, 2);
    plan.add(4, 0);
    tlor::Params weights;
    weights.set("distance_m", 1.0);
    tlor::Report report;
    auto result = run(tlor::Solution(plan), tlor::State::initial(6), weights, none, &report);
    CHECK(result.plan() == plan);
    auto search = phase(nlohmann::json::parse(report.to_json())["calls"][0]["phases"], "search");
    CHECK(search["counters"]["moves_evaluated"] == 0);
}

TEST_CASE("shift_day: same seed, same result") {
    tlor::Params weights;
    weights.set("distance_m", 1.0);
    tlor::Params seeded;
    seeded.set("seed", std::int64_t{5});
    auto a = run(two_days(), tlor::State::initial(6), weights, seeded);
    auto b = run(two_days(), tlor::State::initial(6), weights, seeded);
    CHECK(a.plan() == b.plan());
}

TEST_CASE("shift_day: routes every planned day") {
    tlor::Params weights;
    weights.set("distance_m", 1.0);
    tlor::Params frozen;
    frozen.set("max_iterations", std::int64_t{0});
    auto result = run(two_days(), tlor::State::initial(6), weights, frozen);
    CHECK(result.plan() == two_days().plan());
    REQUIRE(result.routes(0) != nullptr);
    REQUIRE(result.routes(1) != nullptr);
    CHECK(result.routes(0)->routes()[0].nodes == std::vector<tlor::Node>{5, 0, 5});
}
