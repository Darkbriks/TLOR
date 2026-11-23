#include <doctest/doctest.h>

#include <tlor/evaluate.hpp>
#include <tlor/io.hpp>
#include <tlor/steps.hpp>

#include "support/synthetic.hpp"

#include <filesystem>
#include <utility>
#include <vector>

namespace {

using Routes = std::vector<std::pair<tlor::TruckId, std::vector<tlor::Node>>>;

tlor::Params none;

Routes run(const tlor::Fleet& fleet, tlor::Day day, const std::vector<tlor::Node>& points,
           const std::vector<double>& loads) {
    auto inst = tlor::test::small_instance();
    const auto routes = tlor::route(inst, fleet, day, points, loads, "greedy", none, none, nullptr);
    Routes out;
    for (const auto& r : routes.routes()) {
        out.emplace_back(r.truck, r.nodes);
    }
    return out;
}

}

TEST_CASE("greedy: a tour closes when the nearest remaining point does not fit") {
    CHECK(run(tlor::test::small_fleet(), 1, {0, 1, 2, 3}, {60, 20, 40, 100}) ==
          Routes{{0, {5, 0, 1, 2, 5}}, {0, {5, 3, 5}}});
}

TEST_CASE("greedy: a point heavier than the truck gets a tour of its own") {
    CHECK(run(tlor::test::small_fleet(), 1, {0, 1}, {200, 10}) == Routes{{0, {5, 1, 5}}, {0, {5, 0, 5}}});
}

TEST_CASE("greedy: every tour goes to the first available truck") {
    std::vector<std::uint8_t> available(14, 1);
    available[2] = 0;
    const tlor::Fleet fleet(std::vector<tlor::Truck>(2, tlor::Truck{150, 3600}), 7, available);
    CHECK(run(fleet, 1, {0, 3}, {60, 100}) == Routes{{1, {5, 0, 5}}, {1, {5, 3, 5}}});
    CHECK(run(tlor::test::small_fleet(), 5, {0}, {10}) == Routes{{0, {5, 0, 5}}});
}

TEST_CASE("greedy: empty day") {
    CHECK(run(tlor::test::small_fleet(), 1, {}, {}).empty());
}

TEST_CASE("greedy: real data, one 10 t truck, every container full") {
    const auto root = std::filesystem::path(TLOR_SOURCE_DIR);
    if (!std::filesystem::exists(root / "data/tables/daily_demand.csv")) {
        MESSAGE("data/tables is absent, skipped");
        return;
    }
    const auto loaded = tlor::load_instance({root / "data/tables", root / "Maps/offline map/data", {}, false});
    const auto& inst = loaded.instance;
    const tlor::Fleet fleet({tlor::Truck{10000, 1e9}}, inst.day_count(), {});
    const auto points = inst.active_points();
    std::vector<double> loads;
    for (tlor::Node p : points) {
        loads.push_back(inst.capacity(p));
    }
    const auto routes = tlor::route(inst, fleet, 0, points, loads, "greedy", none, none, nullptr);
    CHECK(routes.size() == 14);
    CHECK(tlor::route_indicators(inst, fleet, routes).at("distance_m") == doctest::Approx(934399.845));
}
