#include <doctest/doctest.h>

#include <tlor/indicators.hpp>
#include <tlor/plan.hpp>
#include <tlor/routes.hpp>
#include <tlor/solution.hpp>

#include "support/synthetic.hpp"

#include <algorithm>

using tlor::Status;
using tlor::test::status_of;

TEST_CASE("plan: sorted unique points and range") {
    tlor::Plan p(2, 3);
    p.add(3, 4);
    p.add(3, 1);
    p.add(3, 4);
    CHECK(std::ranges::equal(p.points(3), std::vector<tlor::Node>{1, 4}));
    CHECK(p.points(2).empty());
    CHECK(p.start() == 2);
    CHECK(p.horizon() == 3);
    CHECK(p.end() == 5);
    CHECK(p.contains(4));
    CHECK_FALSE(p.contains(5));
    CHECK(p.collection_count() == 2);
    CHECK(p.collected(3, 4));
    CHECK(status_of([&] { p.add(5, 0); }) == Status::invalid_argument);
    CHECK(status_of([&] { p.add(3, -1); }) == Status::invalid_argument);
    CHECK(status_of([&] { (void)p.points(1); }) == Status::invalid_argument);
    CHECK(status_of([] { tlor::Plan(0, 0); }) == Status::invalid_argument);
    p.remove(3, 1);
    CHECK_FALSE(p.collected(3, 1));
    CHECK(p.collection_count() == 1);
}

TEST_CASE("solution: routes per day") {
    tlor::Solution s(tlor::Plan(0, 2));
    CHECK(s.routes(1) == nullptr);
    tlor::DayRoutes r(1);
    r.add({0, {5, 3, 5}});
    CHECK(r.size() == 1);
    CHECK(r.day() == 1);
    s.set_routes(r);
    REQUIRE(s.routes(1) != nullptr);
    CHECK(*s.routes(1) == r);
    CHECK(s.routes(7) == nullptr);
    CHECK(status_of([&] { s.set_routes(tlor::DayRoutes(2)); }) == Status::invalid_argument);
    s.clear_routes(1);
    CHECK(s.routes(1) == nullptr);
    s.plan().add(0, 3);
    CHECK(s.plan().collected(0, 3));
}

TEST_CASE("indicators: names and lookup") {
    CHECK(tlor::indicator_names.size() == 14);
    CHECK(tlor::is_indicator_name("overflow_kg_days"));
    CHECK_FALSE(tlor::is_indicator_name("distance_km"));
    tlor::Indicators i{{{"distance_m", 250.0}}};
    CHECK(i.at("distance_m") == 250.0);
    CHECK(status_of([&] { (void)i.at("routes"); }) == Status::invalid_argument);
}

TEST_CASE("fixture: small instance matches the plan") {
    auto inst = tlor::test::small_instance();
    CHECK(inst.node_count() == 6);
    CHECK(inst.depot() == 5);
    CHECK(inst.day_count() == 7);
    CHECK(inst.distance(5, 0) == 100);
    CHECK(inst.distance(0, 5) == 150);
    CHECK(inst.distance(0, 1) == 100);
    CHECK(inst.distance(1, 5) == 250);
    CHECK(inst.distance(2, 5) == 350);
    CHECK(inst.distance(5, 3) == 400);
    CHECK(inst.distance(3, 5) == 450);
    CHECK(inst.distance(2, 3) == 100);
    CHECK(inst.distance(5, 2) == 300);
    CHECK(inst.duration(5, 3) == 40);
    CHECK(inst.service(0) == 60);
    CHECK(inst.service(5) == 120);
    CHECK(inst.demand().at(6, 3) == 50);
    CHECK(inst.demand().at(0, 0) == 30);
    CHECK_FALSE(inst.is_point(4));
    CHECK(inst.active_point_count() == 4);
    auto fleet = tlor::test::small_fleet();
    CHECK(fleet.truck_count() == 2);
    CHECK(fleet.available_count(5) == 0);
    CHECK(fleet.available_count(4) == 2);
    CHECK(tlor::test::small_fleet(90, 200, 1).truck(0).capacity_kg == 90);
}

TEST_CASE("solution: replacing the plan keeps routes consistent") {
    tlor::Solution s(tlor::Plan(0, 2));
    tlor::DayRoutes early(1);
    early.add({0, {5, 3, 5}});
    s.set_routes(early);
    s.plan() = tlor::Plan(0, 30);
    tlor::DayRoutes late(20);
    late.add({1, {5, 2, 5}});
    s.set_routes(late);
    REQUIRE(s.routes(20) != nullptr);
    CHECK(*s.routes(20) == late);
    s.plan() = tlor::Plan(10, 5);
    CHECK(s.routes(1) == nullptr);
    CHECK(s.routes(20) == nullptr);
    tlor::DayRoutes mid(12);
    s.set_routes(mid);
    CHECK(s.routes(12) != nullptr);
}
