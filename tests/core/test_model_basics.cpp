#include <doctest/doctest.h>

#include <tlor/demand.hpp>
#include <tlor/error.hpp>
#include <tlor/fleet.hpp>
#include <tlor/instance.hpp>
#include <tlor/params.hpp>
#include <tlor/state.hpp>

#include "support/synthetic.hpp"

#include <cmath>
#include <limits>

using tlor::Status;
using tlor::test::status_of;

namespace {

tlor::InstanceData two_nodes() {
    return tlor::InstanceData{
        .node_count = 2,
        .depot = 1,
        .day_count = 1,
        .distance_m = {0, 10, 12, 0},
        .duration_s = {0, 1, 2, 0},
        .service_s = {60, 120},
        .capacity_kg = {100, 0},
        .active = {1, 0},
        .demand_kg = {5, 0},
    };
}

template <class Mutate>
Status build_status(Mutate mutate) {
    auto data = two_nodes();
    mutate(data);
    return status_of([&] { tlor::Instance{std::move(data)}; });
}

}

TEST_CASE("params: typed access") {
    tlor::Params p;
    p.set("k", std::int64_t{3});
    p.set("t", 0.85);
    CHECK(p.get_int("k") == 3);
    CHECK(p.get_double("k") == 3.0);
    CHECK(p.get_double("t") == doctest::Approx(0.85));
    CHECK_THROWS_AS((void)p.get_int("t"), tlor::Error);
    CHECK(status_of([&] { (void)p.get_int("t"); }) == Status::invalid_param);
    CHECK(status_of([&] { (void)p.get_bool("missing"); }) == Status::invalid_param);
    CHECK(p.contains("k"));
    CHECK(p.find("missing") == nullptr);
    p.set("s", std::string("x"));
    CHECK(p.get_string("s") == "x");
    CHECK(p.values().size() == 3);
}

TEST_CASE("error: status names") {
    CHECK(tlor::to_string(Status::ok) == "ok");
    CHECK(tlor::to_string(Status::invalid_argument) == "invalid_argument");
    CHECK(tlor::to_string(Status::internal_error) == "internal_error");
    tlor::Error e(Status::io_error, "boom");
    CHECK(e.status() == Status::io_error);
    CHECK(std::string(e.what()) == "boom");
}

TEST_CASE("demand: layout and validation") {
    tlor::Demand d(2, 3, {1, 2, 3, 4, 5, 6});
    CHECK(d.at(1, 0) == 4);
    CHECK(d.day_count() == 2);
    CHECK(d.node_count() == 3);
    CHECK(status_of([] { tlor::Demand(2, 3, {1, 2}); }) == Status::invalid_argument);
    CHECK(status_of([] { tlor::Demand(1, 1, {-1}); }) == Status::invalid_argument);
}

TEST_CASE("instance: rejects inconsistent data") {
    CHECK(build_status([](auto&) {}) == Status::ok);
    CHECK(build_status([](auto& d) { d.depot = 2; }) == Status::invalid_argument);
    CHECK(build_status([](auto& d) { d.distance_m.pop_back(); }) == Status::invalid_argument);
    CHECK(build_status([](auto& d) { d.duration_s[1] = -1; }) == Status::invalid_argument);
    CHECK(build_status([](auto& d) { d.demand_kg[0] = std::numeric_limits<double>::quiet_NaN(); }) == Status::invalid_argument);
    CHECK(build_status([](auto& d) { d.capacity_kg[0] = 0; }) == Status::invalid_argument);
    CHECK(build_status([](auto& d) { d.active[1] = 1; }) == Status::invalid_argument);
    CHECK(build_status([](auto& d) { d.node_count = 0; }) == Status::invalid_argument);
    CHECK(build_status([](auto& d) { d.day_count = 0; }) == Status::invalid_argument);
    tlor::Instance inst(two_nodes());
    CHECK(status_of([&] { inst.set_attribute("threshold", {0.9}); }) == Status::invalid_argument);
}

TEST_CASE("instance: accessors and attributes") {
    tlor::InstanceData data{
        .node_count = 3,
        .depot = 2,
        .day_count = 1,
        .distance_m = {0, 1, 2, 3, 0, 4, 5, 6, 0},
        .duration_s = {0, 1, 2, 3, 0, 4, 5, 6, 0},
        .service_s = {60, 60, 120},
        .capacity_kg = {100, 100, 0},
        .active = {1, 0, 0},
        .demand_kg = {5, 0, 0},
    };
    tlor::Instance inst(std::move(data));
    CHECK(inst.distance(1, 2) == 4.0);
    CHECK(inst.distance(2, 1) == 6.0);
    CHECK(inst.duration(0, 2) == 2.0);
    CHECK(inst.service(2) == 120);
    CHECK(inst.capacity(0) == 100);
    CHECK(inst.demand().at(0, 0) == 5);
    CHECK(inst.is_point(0));
    CHECK_FALSE(inst.is_point(1));
    CHECK_FALSE(inst.is_point(2));
    CHECK_FALSE(inst.is_point(3));
    CHECK(inst.active_points() == std::vector<tlor::Node>{0});
    CHECK(inst.active_point_count() == 1);
    CHECK(inst.distance_data().size() == 9);
    CHECK(inst.active_data().size() == 3);
    CHECK(inst.attribute("threshold") == nullptr);
    inst.set_attribute("threshold", {0.9, NAN, NAN});
    REQUIRE(inst.attribute("threshold") != nullptr);
    CHECK((*inst.attribute("threshold"))[0] == doctest::Approx(0.9));
    CHECK(std::isnan((*inst.attribute("threshold"))[1]));
}

TEST_CASE("fleet: availability") {
    tlor::Fleet f({{150, 3600}, {150, 3600}}, 2, {1, 0, 0, 0});
    CHECK(f.truck_count() == 2);
    CHECK(f.day_count() == 2);
    CHECK(f.truck(1).capacity_kg == 150);
    CHECK(f.available(0, 0));
    CHECK_FALSE(f.available(0, 1));
    CHECK(f.available_count(0) == 1);
    CHECK(f.available_count(1) == 0);
    CHECK(f.available_trucks(0) == std::vector<tlor::TruckId>{0});
    CHECK(tlor::Fleet({{150, 3600}}, 3, {}).available_count(2) == 1);
    CHECK(status_of([] { tlor::Fleet({{0, 3600}}, 1, {}); }) == Status::invalid_argument);
    CHECK(status_of([] { tlor::Fleet({}, 1, {}); }) == Status::invalid_argument);
    CHECK(status_of([] { tlor::Fleet({{150, 3600}}, 2, {1}); }) == Status::invalid_argument);
}

TEST_CASE("state: initial") {
    auto s = tlor::State::initial(3);
    CHECK(s.level_kg == std::vector<double>{0, 0, 0});
    CHECK(s.last_collection == std::vector<tlor::Day>{-1, -1, -1});
}
