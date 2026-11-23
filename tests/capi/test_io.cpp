#include <doctest/doctest.h>

#include "capi/capi_support.hpp"
#include "support/temp_dir.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string tiny() { return (std::filesystem::path(TLOR_SOURCE_DIR) / "tests/fixtures/tiny").string(); }

std::string text_of(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

struct WeekSolution {
    tlor_instance* inst = small_instance_handle();
    tlor_solution* solution = nullptr;
    tlor_day_routes* routes = nullptr;

    WeekSolution() {
        tlor_plan* plan = nullptr;
        tlor_plan_create(0, 3, &plan);
        tlor_plan_add(plan, 1, 3);
        tlor_solution_create(plan, &solution);
        tlor_plan_destroy(plan);
        const int32_t nodes[] = {5, 3, 5};
        tlor_day_routes_create(1, &routes);
        tlor_day_routes_add(routes, 0, nodes, 3);
        tlor_solution_set_routes(solution, routes);
    }
    ~WeekSolution() {
        tlor_day_routes_destroy(routes);
        tlor_solution_destroy(solution);
        tlor_instance_destroy(inst);
    }
};

}

TEST_CASE("capi: load tiny fixture") {
    const std::string dir = tiny();
    const std::string service = dir + "/service_time.csv";
    tlor_instance* inst = nullptr;
    char first_date[11] = "xxxxxxxxxx";
    REQUIRE(tlor_instance_load(dir.c_str(), dir.c_str(), service.c_str(), 1, &inst, first_date) == TLOR_OK);
    CHECK(std::string(first_date) == "2023-04-14");
    CHECK(tlor_instance_node_count(inst) == 5);
    tlor_instance_destroy(inst);

    REQUIRE(tlor_instance_load(dir.c_str(), dir.c_str(), service.c_str(), 0, &inst, nullptr) == TLOR_OK);
    tlor_instance_destroy(inst);

    tlor_instance* kept = small_instance_handle();
    inst = kept;
    const std::string nowhere = dir + "/nowhere";
    CHECK(tlor_instance_load(nowhere.c_str(), dir.c_str(), service.c_str(), 1, &inst, first_date) == TLOR_ERROR_IO);
    CHECK(inst == nullptr);
    CHECK(std::string(tlor_last_error()).find("active_points.csv") != std::string::npos);
    tlor_instance_destroy(kept);
    CHECK(tlor_instance_load(nullptr, dir.c_str(), nullptr, 1, &inst, first_date) == TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(tlor_instance_load(dir.c_str(), dir.c_str(), nullptr, 1, nullptr, first_date) ==
          TLOR_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("capi: write solution") {
    WeekSolution w;
    tlor::test::TempDir tmp;
    const std::string out = (tmp.path() / "out").string();
    REQUIRE(tlor_write_solution(out.c_str(), w.inst, nullptr, w.solution, nullptr, "2023-04-14") == TLOR_OK);
    CHECK(std::filesystem::exists(tmp.path() / "out" / "routes_2023-04-15.json"));
    CHECK(text_of(tmp.path() / "out" / "collection_schedule.csv") ==
          "date;point_id;collected_kg;fill_rate\n2023-04-15;3;100;1\n");
    CHECK(std::filesystem::exists(tmp.path() / "out" / "route_statistics.csv"));

    CHECK(tlor_write_solution(out.c_str(), w.inst, nullptr, w.solution, nullptr, "14/04/2023") ==
          TLOR_ERROR_INVALID_ARGUMENT);
    CHECK(tlor_write_solution(out.c_str(), w.inst, nullptr, w.solution, nullptr, nullptr) ==
          TLOR_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("capi: write routes json") {
    WeekSolution w;
    tlor::test::TempDir tmp;
    const std::string file = (tmp.path() / "routes.json").string();
    REQUIRE(tlor_write_routes_json(file.c_str(), w.routes) == TLOR_OK);
    CHECK(text_of(tmp.path() / "routes.json") == "[\n    [5, 3, 5]\n]\n");
    const std::string bad = (tmp.path() / "missing" / "dir" / "routes.json").string();
    CHECK(tlor_write_routes_json(bad.c_str(), w.routes) == TLOR_ERROR_IO);
}
