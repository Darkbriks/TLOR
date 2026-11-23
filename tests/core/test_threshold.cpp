#include <doctest/doctest.h>

#include <tlor/steps.hpp>

#include "support/synthetic.hpp"

#include <algorithm>
#include <cmath>

using tlor::Status;
using tlor::test::status_of;
using V = std::vector<tlor::Node>;

namespace {

tlor::Params none;

tlor::Plan run(const tlor::Instance& inst, const tlor::State& start, tlor::Day day, std::int32_t horizon,
               const tlor::Params& params = none) {
    return tlor::schedule(inst, tlor::test::small_fleet(), start, day, horizon, "threshold", params, none, nullptr);
}

V days_of(const tlor::Plan& plan, tlor::Node point) {
    V days;
    for (tlor::Day d = plan.start(); d < plan.end(); ++d) {
        if (plan.collected(d, point)) {
            days.push_back(d);
        }
    }
    return days;
}

}

TEST_CASE("threshold: week of the small instance") {
    auto inst = tlor::test::small_instance();
    auto plan = run(inst, tlor::State::initial(6), 0, 7);
    CHECK(std::ranges::equal(plan.points(0), V{}));
    CHECK(std::ranges::equal(plan.points(1), V{3}));
    CHECK(std::ranges::equal(plan.points(2), V{0}));
    CHECK(std::ranges::equal(plan.points(3), V{3}));
    CHECK(std::ranges::equal(plan.points(4), V{0, 2, 3}));
    CHECK(std::ranges::equal(plan.points(5), V{}));
    CHECK(std::ranges::equal(plan.points(6), V{3}));
}

TEST_CASE("threshold: point attribute overrides the parameter") {
    auto inst = tlor::test::small_instance();
    inst.set_attribute("threshold", {NAN, 0.3, NAN, NAN, NAN, NAN});
    CHECK(days_of(run(inst, tlor::State::initial(6), 0, 7), 1) == V{2, 4});
    tlor::Params off;
    off.set("use_point_threshold", false);
    CHECK(days_of(run(inst, tlor::State::initial(6), 0, 7, off), 1).empty());
}

TEST_CASE("threshold: max_days attribute") {
    auto inst = tlor::test::small_instance();
    inst.set_attribute("max_days", {NAN, 3, NAN, NAN, NAN, NAN});
    CHECK(days_of(run(inst, tlor::State::initial(6), 0, 7), 1) == V{2, 4});
}

TEST_CASE("threshold: start state") {
    auto inst = tlor::test::small_instance();
    auto start = tlor::State::initial(6);
    start.level_kg[0] = 80;
    CHECK(days_of(run(inst, start, 0, 7), 0).front() == 0);
}

TEST_CASE("threshold: no truck before the due day") {
    auto inst = tlor::test::small_instance();
    auto start = tlor::State::initial(6);
    start.level_kg[3] = 50;
    CHECK(days_of(run(inst, start, 5, 2), 3) == V{6});
}

TEST_CASE("threshold: parameter bounds") {
    auto inst = tlor::test::small_instance();
    tlor::Params high;
    high.set("threshold", 1.5);
    CHECK(status_of([&] { (void)run(inst, tlor::State::initial(6), 0, 7, high); }) == Status::invalid_param);
    tlor::Params zero;
    zero.set("threshold", 0.0);
    CHECK(status_of([&] { (void)run(inst, tlor::State::initial(6), 0, 7, zero); }) == Status::invalid_param);
}

TEST_CASE("threshold: listed with its parameters") {
    const auto& info = tlor::Registry::global().info(tlor::Step::schedule, "threshold");
    CHECK(info.builtin);
    REQUIRE(info.params.size() == 2);
    CHECK(info.params[0].name == "threshold");
    CHECK(std::get<double>(info.params[0].default_value) == doctest::Approx(0.85));
    CHECK(info.params[1].name == "use_point_threshold");
}
