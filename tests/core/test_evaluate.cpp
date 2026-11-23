#include <doctest/doctest.h>

#include <tlor/evaluate.hpp>

#include "support/synthetic.hpp"

using tlor::Status;
using tlor::test::status_of;

namespace {

tlor::Solution week_solution() {
    tlor::Plan plan(0, 7);
    plan.add(2, 0);
    tlor::Solution sol(plan);
    tlor::DayRoutes r(2);
    r.add({0, {5, 0, 5}});
    sol.set_routes(r);
    return sol;
}

}

TEST_CASE("evaluate: hand-computed week") {
    auto inst = tlor::test::small_instance();
    auto e = tlor::evaluate(inst, tlor::test::small_fleet(), tlor::State::initial(6), week_solution(), inst.demand(), {});
    CHECK(e.start_day == 0);
    CHECK(e.total.values.size() == 14);
    CHECK(e.total.at("distance_m") == 250);
    CHECK(e.total.at("travel_s") == 25);
    CHECK(e.total.at("service_s") == 180);
    CHECK(e.total.at("work_s") == 205);
    CHECK(e.total.at("overtime_s") == 0);
    CHECK(e.total.at("routes") == 1);
    CHECK(e.total.at("trucks_used") == 1);
    CHECK(e.total.at("collections") == 1);
    CHECK(e.total.at("collected_kg") == 90);
    CHECK(e.total.at("mean_fill_at_collection") == doctest::Approx(0.9));
    CHECK(e.total.at("low_fill_collections") == 0);
    CHECK(e.total.at("overflow_point_days") == 8);
    CHECK(e.total.at("overflow_kg_days") == 830);
    CHECK(e.total.at("workload_imbalance_s") == 205);
    REQUIRE(e.per_day.size() == 7);
    CHECK(e.per_day[2].at("distance_m") == 250);
    CHECK(e.per_day[2].at("collections") == 1);
    CHECK(e.per_day[0].at("distance_m") == 0);
    CHECK(e.per_day[0].at("mean_fill_at_collection") == 0);
    CHECK(e.per_day[6].at("overflow_point_days") == 3);
    CHECK(e.per_day[2].values.size() == 14);
    tlor::Params w;
    w.set("distance_m", 1.0);
    w.set("overflow_point_days", std::int64_t{1000});
    CHECK(tlor::objective(e.total, w) == 8250);
}

TEST_CASE("evaluate: low fill threshold parameter") {
    auto inst = tlor::test::small_instance();
    tlor::Params p;
    p.set("low_fill_threshold", 0.95);
    auto e = tlor::evaluate(inst, tlor::test::small_fleet(), tlor::State::initial(6), week_solution(), inst.demand(), p);
    CHECK(e.total.at("low_fill_collections") == 1);
}

TEST_CASE("evaluate: overtime") {
    auto inst = tlor::test::small_instance();
    auto e = tlor::evaluate(inst, tlor::test::small_fleet(150, 200), tlor::State::initial(6), week_solution(),
                            inst.demand(), {});
    CHECK(e.total.at("overtime_s") == 5);
}

TEST_CASE("route_indicators: two routes on one truck") {
    auto inst = tlor::test::small_instance();
    tlor::DayRoutes r(1);
    r.add({0, {5, 0, 1, 2, 5}});
    r.add({0, {5, 3, 5}});
    auto i = tlor::route_indicators(inst, tlor::test::small_fleet(), r);
    CHECK(i.at("distance_m") == 1500);
    CHECK(i.at("travel_s") == 150);
    CHECK(i.at("service_s") == 480);
    CHECK(i.at("work_s") == 630);
    CHECK(i.at("routes") == 2);
    CHECK(i.at("trucks_used") == 1);
}

TEST_CASE("objective: unknown weight") {
    tlor::Params w;
    w.set("distance_km", 1.0);
    CHECK(status_of([&] { (void)tlor::objective({}, w); }) == Status::invalid_param);
    tlor::Params b;
    b.set("distance_m", true);
    CHECK(status_of([&] { (void)tlor::objective({}, b); }) == Status::invalid_param);
    tlor::Params known;
    known.set("routes", 2.0);
    CHECK(tlor::objective({}, known) == 0);
}
