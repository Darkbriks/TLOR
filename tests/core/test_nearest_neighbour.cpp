#include <doctest/doctest.h>

#include <tlor/check.hpp>
#include <tlor/steps.hpp>

#include "support/synthetic.hpp"

using tlor::Status;
using tlor::test::status_of;
using R = std::vector<std::pair<tlor::TruckId, std::vector<tlor::Node>>>;

namespace {

tlor::Params none;
const std::vector<tlor::Node> points{0, 1, 2, 3};
const std::vector<double> loads{60, 20, 40, 100};

tlor::DayRoutes run(const tlor::Fleet& fleet, tlor::Day day, const tlor::Params& params = none) {
    auto inst = tlor::test::small_instance();
    return tlor::route(inst, fleet, day, points, loads, "nearest_neighbour", params, none, nullptr);
}

R as_pairs(const tlor::DayRoutes& routes) {
    R out;
    for (const auto& r : routes.routes()) {
        out.emplace_back(r.truck, r.nodes);
    }
    return out;
}

}

TEST_CASE("nearest_neighbour: capacity cut and second route on the same truck") {
    CHECK(as_pairs(run(tlor::test::small_fleet(), 1)) == R{{0, {5, 0, 1, 2, 5}}, {0, {5, 3, 5}}});
}

TEST_CASE("nearest_neighbour: time limit moves to the next truck") {
    CHECK(as_pairs(run(tlor::test::small_fleet(150, 400), 1)) == R{{0, {5, 0, 1, 2, 5}}, {1, {5, 3, 5}}});
}

TEST_CASE("nearest_neighbour: trucks exhausted, best effort") {
    CHECK(as_pairs(run(tlor::test::small_fleet(150, 300, 1), 1)) == R{{0, {5, 0, 1, 5}}, {0, {5, 2, 3, 5}}});
}

TEST_CASE("nearest_neighbour: day without truck") {
    CHECK(as_pairs(run(tlor::test::small_fleet(), 5)) == R{{0, {5, 0, 1, 2, 5}}, {1, {5, 3, 5}}});
}

TEST_CASE("nearest_neighbour: empty day") {
    auto inst = tlor::test::small_instance();
    auto routes = tlor::route(inst, tlor::test::small_fleet(), 1, {}, {}, "nearest_neighbour", none, none, nullptr);
    CHECK(routes.size() == 0);
    CHECK(routes.day() == 1);
}

TEST_CASE("nearest_neighbour: criterion parameter") {
    tlor::Params duration;
    duration.set("criterion", std::string("duration"));
    CHECK(as_pairs(run(tlor::test::small_fleet(), 1, duration)) == R{{0, {5, 0, 1, 2, 5}}, {0, {5, 3, 5}}});
    tlor::Params bad;
    bad.set("criterion", std::string("time"));
    CHECK(status_of([&] { (void)run(tlor::test::small_fleet(), 1, bad); }) == Status::invalid_param);
}

TEST_CASE("nearest_neighbour: output passes the checker") {
    auto inst = tlor::test::small_instance();
    auto fleet = tlor::test::small_fleet();
    auto routes = run(fleet, 1);
    tlor::Plan plan(1, 1);
    for (tlor::Node p : points) {
        plan.add(1, p);
    }
    tlor::Solution solution(plan);
    solution.set_routes(routes);
    std::vector<double> demand(7 * 6, 0.0);
    for (std::size_t i = 0; i < points.size(); ++i) {
        demand[static_cast<std::size_t>(6 + points[i])] = loads[i];
    }
    auto violations = tlor::check(inst, fleet, tlor::State::initial(6), solution, tlor::Demand(7, 6, demand));
    CHECK(violations.empty());
}
