#include <doctest/doctest.h>

#include <tlor/report.hpp>
#include <tlor/version.hpp>

#include "support/synthetic.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <random>
#include <regex>

using tlor::Status;
using tlor::test::status_of;

namespace {

nlohmann::json parse(const tlor::Report& r) { return nlohmann::json::parse(r.to_json()); }

std::filesystem::path fresh_temp_dir() {
    std::random_device rd;
    auto dir = std::filesystem::temp_directory_path() / ("tlor-report-" + std::to_string(rd()));
    std::filesystem::create_directories(dir);
    return dir;
}

}

TEST_CASE("report: aggregates repeated phases") {
    tlor::Report r;
    for (int i = 0; i < 3; ++i) {
        tlor::Phase p(&r, "a");
    }
    auto j = parse(r);
    CHECK(j["schema_version"] == 1);
    REQUIRE(j["phases"].size() == 1);
    CHECK(j["phases"][0]["name"] == "a");
    CHECK(j["phases"][0]["count"] == 3);
    CHECK(j["phases"][0]["min_ns"] <= j["phases"][0]["max_ns"]);
    CHECK(j["phases"][0]["total_ns"] >= j["phases"][0]["max_ns"]);
    CHECK(j["phases"][0]["min_ns"] >= 0);
    CHECK(j["calls"].empty());
}

TEST_CASE("report: nested phases and counters") {
    tlor::Report r;
    {
        tlor::Phase a(&r, "a");
        tlor::count(&r, "moves", 5);
        tlor::count(&r, "moves", 5);
        tlor::Phase b(&r, "b");
    }
    tlor::count(&r, "outside");
    auto j = parse(r);
    CHECK(j["phases"][0]["counters"]["moves"] == 10);
    CHECK(j["phases"][0]["children"][0]["name"] == "b");
    CHECK(j["phases"][0]["children"][0]["count"] == 1);
    CHECK(j["counters"]["outside"] == 1);
}

TEST_CASE("report: call record") {
    tlor::Report r;
    tlor::Params p;
    p.set("seed", std::int64_t{3});
    p.set("criterion", std::string("distance"));
    r.begin_call({"route", "nn", p, {6, 4, 7, 2}});
    CHECK(r.in_call());
    {
        tlor::Phase c(&r, "construction");
    }
    tlor::count(&r, "routes_built", 2);
    r.begin_call({"route", "inner", {}, {6, 4, 7, 2}});
    r.end_call(Status::ok, std::nullopt);
    r.end_call(Status::ok, tlor::Indicators{{{"distance_m", 250.0}}});
    CHECK_FALSE(r.in_call());
    auto j = parse(r);
    REQUIRE(j["calls"].size() == 1);
    auto call = j["calls"][0];
    CHECK(call["step"] == "route");
    CHECK(call["method"] == "nn");
    CHECK(call["params"]["seed"] == 3);
    CHECK(call["params"]["criterion"] == "distance");
    CHECK(call["seed"] == 3);
    CHECK(call["instance"]["nodes"] == 6);
    CHECK(call["instance"]["active_points"] == 4);
    CHECK(call["instance"]["days"] == 7);
    CHECK(call["instance"]["trucks"] == 2);
    CHECK(call["status"] == "ok");
    CHECK(call["wall_ns"] >= 0);
    CHECK(call["phases"][0]["name"] == "construction");
    CHECK(call["phases"][1]["name"] == "call:route/inner");
    CHECK(call["phases"][1]["count"] == 1);
    CHECK(call["counters"]["routes_built"] == 2);
    CHECK(call["indicators"]["distance_m"] == 250.0);
    CHECK(call["series"].is_object());
    CHECK(call["series"].empty());
    CHECK(j["phases"].empty());
}

TEST_CASE("report: call without seed or indicators") {
    tlor::Report r;
    r.begin_call({"schedule", "threshold", {}, {6, 4, 7, 2}});
    r.end_call(Status::infeasible, std::nullopt);
    auto call = parse(r)["calls"][0];
    CHECK(call["seed"].is_null());
    CHECK(call["indicators"].is_null());
    CHECK(call["status"] == "infeasible");
}

TEST_CASE("report: context") {
    tlor::Report r;
    r.set_context("experiment", "x");
    auto c = parse(r)["context"];
    CHECK(c["library_version"] == "0.1.0");
    CHECK(c["git_commit"] == std::string(tlor::git_commit()));
    CHECK_FALSE(c["build_type"].get<std::string>().empty());
    CHECK_FALSE(c["compiler"].get<std::string>().empty());
    CHECK_FALSE(c["os"].get<std::string>().empty());
    CHECK(std::regex_match(c["started_at"].get<std::string>(), std::regex(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$)")));
    CHECK(c["user"]["experiment"] == "x");
}

TEST_CASE("report: null report is a no-op") {
    tlor::Phase p(nullptr, "a");
    tlor::count(nullptr, "x");
    CHECK(true);
}

TEST_CASE("report: end_phase without phase") {
    tlor::Report r;
    CHECK(status_of([&] { r.end_phase(); }) == Status::invalid_argument);
    CHECK(status_of([&] { r.end_call(Status::ok, std::nullopt); }) == Status::invalid_argument);
    r.begin_call({"route", "nn", {}, {6, 4, 7, 2}});
    CHECK(status_of([&] { r.end_phase(); }) == Status::invalid_argument);
}

TEST_CASE("report: write to a missing directory") {
    tlor::Report r;
    tlor::Phase p(&r, "a");
    auto dir = fresh_temp_dir();
    auto missing = dir / "missing-dir" / "report.json";
    try {
        r.write_json(missing);
        FAIL("write_json should have thrown");
    } catch (const tlor::Error& e) {
        CHECK(e.status() == Status::io_error);
        CHECK(std::string(e.what()).find("missing-dir") != std::string::npos);
    }
    auto ok = dir / "report.json";
    r.write_json(ok);
    std::ifstream in(ok);
    auto j = nlohmann::json::parse(in);
    CHECK(j["schema_version"] == 1);
    std::filesystem::remove_all(dir);
}
