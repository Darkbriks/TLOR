#include <doctest/doctest.h>

#include <tlor/simulate.hpp>

#include "support/synthetic.hpp"

#include <cmath>

using tlor::Status;
using tlor::test::status_of;

TEST_CASE("simulate: collection empties the container") {
    auto inst = tlor::test::small_instance();
    tlor::Plan plan(0, 7);
    plan.add(2, 0);
    auto sim = tlor::simulate(inst, tlor::State::initial(6), 0, 7, plan, inst.demand());
    CHECK(sim.start_day == 0);
    CHECK(sim.days == 7);
    CHECK(sim.level(0, 0) == 30);
    CHECK(sim.level(2, 0) == 90);
    CHECK(sim.was_collected(2, 0));
    CHECK_FALSE(sim.was_collected(3, 0));
    CHECK(sim.level(3, 0) == 30);
    CHECK(sim.end.level_kg[0] == 120);
    CHECK(sim.end.last_collection[0] == 2);
    CHECK(sim.end.last_collection[3] == -1);
    CHECK(sim.end.level_kg[3] == 350);
    CHECK(tlor::day_loads(sim, plan, 2) == std::vector<double>{90});
}

TEST_CASE("simulate: partial horizon and given demand") {
    auto inst = tlor::test::small_instance();
    tlor::Plan plan(0, 7);
    plan.add(2, 0);
    auto three = tlor::simulate(inst, tlor::State::initial(6), 0, 3, plan, inst.demand());
    CHECK(three.end.level_kg[0] == 0);
    CHECK(three.end.level_kg[3] == 150);
    std::vector<double> doubled(inst.demand().data().begin(), inst.demand().data().end());
    for (auto& v : doubled) {
        v *= 2;
    }
    auto sim = tlor::simulate(inst, tlor::State::initial(6), 0, 3, plan, tlor::Demand(7, 6, doubled));
    CHECK(sim.level(2, 0) == 180);
    CHECK(status_of([&] { tlor::simulate(inst, tlor::State::initial(6), 5, 3, plan, inst.demand()); }) ==
          Status::invalid_argument);
}

TEST_CASE("simulate: start state and days outside the plan") {
    auto inst = tlor::test::small_instance();
    tlor::Plan plan(2, 2);
    plan.add(2, 3);
    auto start = tlor::State::initial(6);
    start.level_kg[3] = 40;
    start.last_collection[3] = 0;
    auto sim = tlor::simulate(inst, start, 1, 4, plan, inst.demand());
    CHECK(sim.level(1, 3) == 90);
    CHECK(sim.level(2, 3) == 140);
    CHECK(sim.was_collected(2, 3));
    CHECK(sim.level(4, 3) == 100);
    CHECK(sim.end.last_collection[3] == 2);
    CHECK(status_of([&] { tlor::simulate(inst, tlor::State::initial(5), 0, 1, plan, inst.demand()); }) ==
          Status::invalid_argument);
    CHECK(status_of([&] { tlor::simulate(inst, tlor::State::initial(6), 0, -1, plan, inst.demand()); }) ==
          Status::invalid_argument);
}

TEST_CASE("simulate: day_loads rejects days and nodes outside the simulation") {
    auto inst = tlor::test::small_instance();
    tlor::Plan plan(0, 7);
    plan.add(5, 0);
    auto sim = tlor::simulate(inst, tlor::State::initial(6), 0, 3, plan, inst.demand());
    CHECK(status_of([&] { (void)tlor::day_loads(sim, plan, 5); }) == Status::invalid_argument);
    tlor::Plan wide(0, 7);
    wide.add(1, 40);
    auto sim2 = tlor::simulate(inst, tlor::State::initial(6), 0, 3, wide, inst.demand());
    CHECK(status_of([&] { (void)tlor::day_loads(sim2, wide, 1); }) == Status::invalid_argument);
}

TEST_CASE("simulate: rejects an invalid start state") {
    auto inst = tlor::test::small_instance();
    tlor::Plan plan(0, 7);
    auto nan_state = tlor::State::initial(6);
    nan_state.level_kg[3] = NAN;
    CHECK(status_of([&] { tlor::simulate(inst, nan_state, 0, 3, plan, inst.demand()); }) == Status::invalid_argument);
    auto negative = tlor::State::initial(6);
    negative.level_kg[0] = -1;
    CHECK(status_of([&] { tlor::simulate(inst, negative, 0, 3, plan, inst.demand()); }) == Status::invalid_argument);
    auto future = tlor::State::initial(6);
    future.last_collection[0] = 2;
    CHECK(status_of([&] { tlor::simulate(inst, future, 2, 3, plan, inst.demand()); }) == Status::invalid_argument);
    auto below = tlor::State::initial(6);
    below.last_collection[0] = -2;
    CHECK(status_of([&] { tlor::simulate(inst, below, 0, 3, plan, inst.demand()); }) == Status::invalid_argument);
}
