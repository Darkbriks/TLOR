#include <doctest/doctest.h>

#include <tlor/random.hpp>
#include <tlor/registry.hpp>

#include "support/synthetic.hpp"

#include <algorithm>
#include <span>

using tlor::Status;
using tlor::test::status_of;

TEST_CASE("random: engine reference value") {
    tlor::Random r(5489);
    std::uint64_t v = 0;
    for (int i = 0; i < 10000; ++i) {
        v = r.next();
    }
    CHECK(v == 9981545732273789042ULL);
}

TEST_CASE("random: below and shuffle are deterministic") {
    tlor::Random a(7);
    tlor::Random b(7);
    for (int i = 0; i < 1000; ++i) {
        auto x = a.below(10);
        CHECK(x < 10);
        CHECK(x == b.below(10));
    }
    CHECK(tlor::Random(3).below(1) == 0);
    std::vector<int> u{0, 1, 2, 3, 4, 5};
    std::vector<int> v = u;
    tlor::Random(1).shuffle(std::span(u));
    tlor::Random(1).shuffle(std::span(v));
    CHECK(u == v);
    std::vector<int> sorted = u;
    std::ranges::sort(sorted);
    CHECK(sorted == std::vector<int>{0, 1, 2, 3, 4, 5});
}

TEST_CASE("registry: step names") {
    CHECK(tlor::to_string(tlor::Step::schedule) == "schedule");
    CHECK(tlor::to_string(tlor::Step::route) == "route");
    CHECK(tlor::to_string(tlor::Step::improve) == "improve");
}

TEST_CASE("registry: validate_params") {
    tlor::MethodInfo m{"m",
                       "",
                       {{"t", tlor::ParamType::real, 0.85, 0.0, 1.0, ""},
                        {"n", tlor::ParamType::integer, std::int64_t{10}, 1.0, std::nullopt, ""},
                        {"flag", tlor::ParamType::boolean, true, std::nullopt, std::nullopt, ""},
                        {"c", tlor::ParamType::string, std::string("distance"), std::nullopt, std::nullopt, ""}}};
    auto filled = tlor::validate_params(m, {});
    CHECK(filled.get_double("t") == doctest::Approx(0.85));
    CHECK(filled.get_int("n") == 10);
    CHECK(filled.get_bool("flag"));
    CHECK(filled.get_string("c") == "distance");
    tlor::Params p;
    p.set("t", std::int64_t{1});
    auto converted = tlor::validate_params(m, p);
    REQUIRE(std::holds_alternative<double>(*converted.find("t")));
    CHECK(converted.get_double("t") == 1.0);
    p.set("t", 1.5);
    CHECK(status_of([&] { (void)tlor::validate_params(m, p); }) == Status::invalid_param);
    tlor::Params low;
    low.set("n", std::int64_t{0});
    CHECK(status_of([&] { (void)tlor::validate_params(m, low); }) == Status::invalid_param);
    tlor::Params q;
    q.set("unknown", 1.0);
    CHECK(status_of([&] { (void)tlor::validate_params(m, q); }) == Status::invalid_param);
    tlor::Params r;
    r.set("n", std::string("ten"));
    CHECK(status_of([&] { (void)tlor::validate_params(m, r); }) == Status::invalid_param);
    tlor::Params s;
    s.set("n", 2.5);
    CHECK(status_of([&] { (void)tlor::validate_params(m, s); }) == Status::invalid_param);
}

TEST_CASE("registry: add, list, remove") {
    auto& reg = tlor::Registry::global();
    reg.add_route({"test_route", "", {}}, [](const tlor::RouteRequest& r) { return tlor::DayRoutes(r.day); });
    CHECK(status_of([&] { reg.add_route({"test_route", "", {}}, {}); }) == Status::invalid_argument);
    CHECK(reg.info(tlor::Step::route, "test_route").name == "test_route");
    auto names = reg.methods(tlor::Step::route);
    CHECK(std::ranges::any_of(names, [](const tlor::MethodInfo* m) { return m->name == "test_route"; }));
    CHECK(std::ranges::is_sorted(names, {}, [](const tlor::MethodInfo* m) { return m->name; }));
    CHECK(static_cast<bool>(reg.route_fn("test_route")));
    reg.remove(tlor::Step::route, "test_route");
    CHECK(status_of([&] { (void)reg.info(tlor::Step::route, "test_route"); }) == Status::unknown_method);
    CHECK(status_of([&] { reg.remove(tlor::Step::route, "test_route"); }) == Status::unknown_method);
    CHECK(status_of([&] { (void)reg.schedule_fn("nope"); }) == Status::unknown_method);
}

TEST_CASE("registry: builtin methods cannot be removed") {
    auto& reg = tlor::Registry::global();
    tlor::MethodInfo info{"test_builtin", "", {}};
    info.builtin = true;
    reg.add_improve(info, [](const tlor::ImproveRequest& r) { return r.solution; });
    CHECK(status_of([&] { reg.remove(tlor::Step::improve, "test_builtin"); }) == Status::invalid_argument);
}
