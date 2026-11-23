#include <doctest/doctest.h>

#include <tlor/error.hpp>
#include <tlor/io.hpp>
#include <tlor/solution.hpp>
#include <tlor/state.hpp>

#include "support/synthetic.hpp"
#include "support/temp_dir.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using tlor::Status;
using tlor::test::status_of;
using tlor::test::TempDir;

namespace {

std::vector<std::string> lines_of(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    std::vector<std::string> lines;
    for (std::string line; std::getline(in, line);) {
        lines.push_back(line);
    }
    return lines;
}

nlohmann::json json_of(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    return nlohmann::json::parse(in);
}

}

TEST_CASE("io: routes json follows the contract") {
    TempDir tmp;
    tlor::DayRoutes routes(1);
    routes.add({0, {5, 0, 1, 5}});
    routes.add({1, {5, 3, 5}});
    tlor::write_routes_json(tmp.path() / "routes.json", routes);
    CHECK(json_of(tmp.path() / "routes.json") == nlohmann::json::parse("[[5,0,1,5],[5,3,5]]"));

    tlor::write_routes_json(tmp.path() / "empty.json", tlor::DayRoutes(2));
    CHECK(json_of(tmp.path() / "empty.json") == nlohmann::json::array());
}

TEST_CASE("io: write_solution") {
    TempDir tmp;
    auto instance = tlor::test::small_instance();
    tlor::Plan plan(0, 3);
    plan.add(1, 3);
    tlor::Solution solution(plan);
    tlor::DayRoutes routes(1);
    routes.add({0, {5, 3, 5}});
    solution.set_routes(routes);
    const auto out = tmp.path() / "out";
    tlor::write_solution(out, instance, tlor::State::initial(instance.node_count()), solution, instance.demand(),
                         tlor::parse_date("2023-04-14"));
    CHECK(std::filesystem::exists(out / "routes_2023-04-15.json"));
    CHECK_FALSE(std::filesystem::exists(out / "routes_2023-04-14.json"));
    CHECK(json_of(out / "routes_2023-04-15.json") == nlohmann::json::parse("[[5,3,5]]"));
    CHECK(lines_of(out / "collection_schedule.csv") ==
          std::vector<std::string>{"date;point_id;collected_kg;fill_rate", "2023-04-15;3;100;1"});
    CHECK(lines_of(out / "route_statistics.csv") ==
          std::vector<std::string>{"date;route;truck;distance;duration;load_kg;points", "2023-04-15;0;0;850;265;100;1"});
}

TEST_CASE("io: route statistics count each collected point once per day") {
    TempDir tmp;
    auto instance = tlor::test::small_instance();
    tlor::Plan plan(2, 1);
    plan.add(2, 0);
    plan.add(2, 2);
    plan.add(2, 4);
    tlor::Solution solution(plan);
    tlor::DayRoutes routes(2);
    routes.add({1, {5, 0, 1, 5}});
    routes.add({0, {5, 0, 2, 5}});
    solution.set_routes(routes);
    tlor::write_solution(tmp.path(), instance, tlor::State::initial(instance.node_count()), solution,
                         instance.demand(), tlor::parse_date("2023-04-14"));
    CHECK(lines_of(tmp.path() / "collection_schedule.csv") ==
          std::vector<std::string>{"date;point_id;collected_kg;fill_rate", "2023-04-16;0;30;0.3",
                                   "2023-04-16;2;20;0.2"});
    CHECK(lines_of(tmp.path() / "route_statistics.csv") ==
          std::vector<std::string>{"date;route;truck;distance;duration;load_kg;points",
                                   "2023-04-16;0;1;450;285;30;2", "2023-04-16;1;0;650;305;20;2"});
}

TEST_CASE("io: write to an impossible path") {
    TempDir tmp;
    {
        std::ofstream file(tmp.path() / "file");
        file << "x";
    }
    auto instance = tlor::test::small_instance();
    tlor::Solution solution(tlor::Plan(0, 1));
    const auto bad = tmp.path() / "file" / "out";
    CHECK(status_of([&] {
              tlor::write_solution(bad, instance, tlor::State::initial(instance.node_count()), solution,
                                   instance.demand(), tlor::parse_date("2023-04-14"));
          }) == Status::io_error);
    CHECK(status_of([&] { tlor::write_routes_json(bad / "routes.json", tlor::DayRoutes(0)); }) == Status::io_error);
}

TEST_CASE("io: write_solution removes the routes files of an earlier run") {
    TempDir tmp;
    auto instance = tlor::test::small_instance();
    const auto first = tlor::parse_date("2023-04-14");
    const auto state = tlor::State::initial(instance.node_count());
    tlor::Plan plan(0, 3);
    plan.add(1, 3);
    plan.add(2, 0);
    tlor::Solution longer(plan);
    tlor::DayRoutes day1(1);
    day1.add({0, {5, 3, 5}});
    tlor::DayRoutes day2(2);
    day2.add({0, {5, 0, 5}});
    longer.set_routes(day1);
    longer.set_routes(day2);
    tlor::write_solution(tmp.path(), instance, state, longer, instance.demand(), first);
    REQUIRE(std::filesystem::exists(tmp.path() / "routes_2023-04-16.json"));
    std::ofstream(tmp.path() / "routes_notes.json") << "[]";
    std::ofstream(tmp.path() / "notes.txt") << "kept";

    tlor::Plan short_plan(0, 2);
    short_plan.add(1, 3);
    tlor::Solution shorter(short_plan);
    shorter.set_routes(day1);
    tlor::write_solution(tmp.path(), instance, state, shorter, instance.demand(), first);
    CHECK(std::filesystem::exists(tmp.path() / "routes_2023-04-15.json"));
    CHECK_FALSE(std::filesystem::exists(tmp.path() / "routes_2023-04-16.json"));
    CHECK(std::filesystem::exists(tmp.path() / "routes_notes.json"));
    CHECK(std::filesystem::exists(tmp.path() / "notes.txt"));
}
