#include <doctest/doctest.h>

#include <tlor/check.hpp>

#include "support/synthetic.hpp"

#include <algorithm>
#include <cmath>

using tlor::ViolationKind;
using K = std::vector<ViolationKind>;

namespace {

tlor::Solution clean() {
    tlor::Plan plan(0, 2);
    plan.add(1, 3);
    tlor::Solution s(plan);
    tlor::DayRoutes r(1);
    r.add({0, {5, 3, 5}});
    s.set_routes(r);
    return s;
}

tlor::Solution with_route(tlor::Route route, tlor::Day day = 1) {
    tlor::Solution s = clean();
    tlor::DayRoutes r(day);
    r.add(std::move(route));
    s.set_routes(r);
    return s;
}

std::vector<tlor::Violation> run(const tlor::Solution& s, const tlor::Fleet& fleet = tlor::test::small_fleet(),
                                 const tlor::Instance& inst = tlor::test::small_instance()) {
    return tlor::check(inst, fleet, tlor::State::initial(6), s, inst.demand());
}

K kinds(const std::vector<tlor::Violation>& v) {
    K out;
    for (const auto& x : v) {
        out.push_back(x.kind);
    }
    return out;
}

bool has(const std::vector<tlor::Violation>& v, ViolationKind kind) {
    return std::ranges::any_of(v, [&](const auto& x) { return x.kind == kind; });
}

}

TEST_CASE("check: clean solution has no violation") {
    CHECK(run(clean()).empty());
}

TEST_CASE("check: kind names") {
    CHECK(tlor::to_string(ViolationKind::route_endpoints) == "route_endpoints");
    CHECK(tlor::to_string(ViolationKind::max_days) == "max_days");
}

TEST_CASE("check: route_endpoints") {
    auto v = run(with_route({0, {3, 5}}));
    CHECK(has(v, ViolationKind::route_endpoints));
    CHECK(has(run(with_route({0, {5, 3, 5, 3, 5}})), ViolationKind::route_endpoints));
    CHECK(has(run(with_route({0, {5}})), ViolationKind::route_endpoints));
}

TEST_CASE("check: visit_count") {
    auto v = run(with_route({0, {5, 3, 3, 5}}));
    REQUIRE(kinds(v) == K{ViolationKind::visit_count});
    CHECK(v[0].day == 1);
    CHECK(v[0].point == 3);
    tlor::Plan plan(0, 2);
    plan.add(1, 3);
    CHECK(kinds(run(tlor::Solution(plan))) == K{ViolationKind::visit_count});
}

TEST_CASE("check: unplanned_visit") {
    auto v = run(with_route({0, {5, 3, 0, 5}}));
    REQUIRE(kinds(v) == K{ViolationKind::unplanned_visit});
    CHECK(v[0].point == 0);
    CHECK(v[0].route == 0);
}

TEST_CASE("check: invalid_node") {
    auto v = run(with_route({0, {5, 3, 9, 5}}));
    REQUIRE(has(v, ViolationKind::invalid_node));
    CHECK(std::ranges::any_of(v, [](const auto& x) { return x.kind == ViolationKind::invalid_node && x.point == 9; }));
    auto s = clean();
    s.plan().add(1, 4);
    auto w = run(s);
    CHECK(std::ranges::any_of(w, [](const auto& x) { return x.kind == ViolationKind::invalid_node && x.point == 4; }));
}

TEST_CASE("check: capacity") {
    auto v = run(clean(), tlor::test::small_fleet(90));
    REQUIRE(kinds(v) == K{ViolationKind::capacity});
    CHECK(v[0].route == 0);
    CHECK(v[0].day == 1);
}

TEST_CASE("check: work_time") {
    auto v = run(clean(), tlor::test::small_fleet(150, 200));
    REQUIRE(kinds(v) == K{ViolationKind::work_time});
    CHECK(v[0].route == -1);
    CHECK(v[0].message.find("truck 0") != std::string::npos);
}

TEST_CASE("check: truck_unavailable") {
    CHECK(has(run(with_route({7, {5, 3, 5}})), ViolationKind::truck_unavailable));
    std::vector<std::uint8_t> available(14, 1);
    available[1 * 2 + 0] = 0;
    tlor::Fleet fleet({{150, 3600}, {150, 3600}}, 7, available);
    CHECK(kinds(run(clean(), fleet)) == K{ViolationKind::truck_unavailable});
    tlor::Plan plan(4, 2);
    plan.add(5, 3);
    tlor::Solution s(plan);
    tlor::DayRoutes r(5);
    r.add({0, {5, 3, 5}});
    s.set_routes(r);
    CHECK(kinds(run(s)) == K{ViolationKind::truck_unavailable});
}

TEST_CASE("check: overflow") {
    auto v = run(tlor::Solution(tlor::Plan(0, 3)));
    REQUIRE(kinds(v) == K{ViolationKind::overflow});
    CHECK(v[0].day == 2);
    CHECK(v[0].point == 3);
}

TEST_CASE("check: max_days") {
    auto inst = tlor::test::small_instance();
    inst.set_attribute("max_days", {1, NAN, NAN, NAN, NAN, NAN});
    auto v = run(clean(), tlor::test::small_fleet(), inst);
    REQUIRE(kinds(v) == K{ViolationKind::max_days});
    CHECK(v[0].day == 1);
    CHECK(v[0].point == 0);
}

TEST_CASE("check: rejects an invalid start state") {
    auto inst = tlor::test::small_instance();
    auto state = tlor::State::initial(6);
    state.level_kg[3] = NAN;
    CHECK(tlor::test::status_of([&] { (void)tlor::check(inst, tlor::test::small_fleet(), state, clean(), inst.demand()); }) ==
          tlor::Status::invalid_argument);
}
