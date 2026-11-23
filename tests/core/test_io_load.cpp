#include <doctest/doctest.h>

#include <tlor/error.hpp>
#include <tlor/io.hpp>

#include "support/synthetic.hpp"
#include "support/temp_dir.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using tlor::Status;
using tlor::test::status_of;
using tlor::test::TempDir;

namespace {

std::filesystem::path tiny() { return std::filesystem::path(TLOR_SOURCE_DIR) / "tests/fixtures/tiny"; }

void copy_tiny(const std::filesystem::path& to) {
    std::filesystem::copy(tiny(), to, std::filesystem::copy_options::recursive);
}

void write_file(const std::filesystem::path& file, const std::string& text) {
    std::ofstream out(file, std::ios::binary);
    out << text;
}

std::string read_file(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

tlor::LoadOptions options_for(const std::filesystem::path& dir) { return {dir, dir, dir / "service_time.csv", true}; }

std::string error_message(const tlor::LoadOptions& options) {
    try {
        (void)tlor::load_instance(options);
    } catch (const tlor::Error& e) {
        return e.what();
    }
    return {};
}

}

TEST_CASE("io: dates") {
    auto d = tlor::parse_date("2023-04-14");
    CHECK(tlor::format_date(d + std::chrono::days{62}) == "2023-06-15");
    CHECK(tlor::format_date(tlor::parse_date("2024-02-29")) == "2024-02-29");
    CHECK(status_of([] { (void)tlor::parse_date("2023-13-01"); }) == Status::invalid_argument);
    CHECK(status_of([] { (void)tlor::parse_date("2023-02-29"); }) == Status::invalid_argument);
    CHECK(status_of([] { (void)tlor::parse_date("2023-4-14"); }) == Status::invalid_argument);
    CHECK(status_of([] { (void)tlor::parse_date("2023-04-14x"); }) == Status::invalid_argument);
    CHECK(status_of([] { (void)tlor::parse_date(""); }) == Status::invalid_argument);
}

TEST_CASE("io: loads the tiny fixture") {
    auto dir = tiny();
    auto loaded = tlor::load_instance({dir, dir, dir / "service_time.csv", true});
    const auto& i = loaded.instance;
    CHECK(i.node_count() == 5);
    CHECK(i.depot() == 4);
    CHECK(i.day_count() == 3);
    CHECK(i.capacity(1) == 200);
    CHECK(i.capacity(3) == 0);
    CHECK_FALSE(i.active(3));
    CHECK(i.active_point_count() == 3);
    CHECK(i.demand().at(2, 1) == 20);
    CHECK(i.demand().at(0, 3) == 0);
    CHECK(i.distance(1, 0) == 110);
    CHECK(i.duration(4, 3) == 40);
    CHECK(i.service(0) == 180);
    CHECK(i.service(4) == 0);
    REQUIRE(i.attribute("threshold") != nullptr);
    CHECK((*i.attribute("threshold"))[0] == doctest::Approx(0.9));
    CHECK(std::isnan((*i.attribute("threshold"))[2]));
    CHECK(tlor::format_date(loaded.first_date) == "2023-04-14");
}

TEST_CASE("io: point policy is optional") {
    auto dir = tiny();
    auto loaded = tlor::load_instance({dir, dir, dir / "service_time.csv", false});
    CHECK(loaded.instance.attribute("threshold") == nullptr);
}

TEST_CASE("io: missing file") {
    auto dir = tiny();
    auto message = error_message({dir, dir / "nowhere", dir / "service_time.csv", true});
    CHECK(message.find("distances.json") != std::string::npos);
    CHECK(status_of([&] { (void)tlor::load_instance({dir, dir / "nowhere", dir / "service_time.csv", true}); }) ==
          Status::io_error);
}

TEST_CASE("io: default service time file sits next to the matrices") {
    TempDir tmp;
    copy_tiny(tmp.path());
    std::filesystem::rename(tmp.path() / "service_time.csv", tmp.path() / "exemple_points_service_time.csv");
    auto loaded = tlor::load_instance({tmp.path(), tmp.path(), {}, true});
    CHECK(loaded.instance.service(0) == 180);
}

TEST_CASE("io: header names are trimmed and CRLF is accepted") {
    TempDir tmp;
    copy_tiny(tmp.path());
    write_file(tmp.path() / "active_points.csv",
               "is_depot ; point_id; active;capacity_kg\r\nFalse;0;True;30\r\nFalse;1;True;200\r\n"
               "False;2;True;50\r\nFalse;3;False;\r\nTrue;4;False;\r\n");
    write_file(tmp.path() / "service_time.csv", "service_time\r\n180\r\n180\r\n180\r\n180\r\n0");
    auto loaded = tlor::load_instance(options_for(tmp.path()));
    CHECK(loaded.instance.capacity(2) == 50);
    CHECK(loaded.instance.depot() == 4);
    CHECK(loaded.instance.service(4) == 0);
}

TEST_CASE("io: dates must be contiguous") {
    TempDir tmp;
    copy_tiny(tmp.path());
    std::istringstream in(read_file(tmp.path() / "daily_demand.csv"));
    std::string kept;
    for (std::string line; std::getline(in, line);) {
        if (line.find("2023-04-15") == std::string::npos) {
            kept += line + "\n";
        }
    }
    write_file(tmp.path() / "daily_demand.csv", kept);
    CHECK(status_of([&] { (void)tlor::load_instance(options_for(tmp.path())); }) == Status::invalid_argument);
}

TEST_CASE("io: matrix size mismatch") {
    TempDir tmp;
    copy_tiny(tmp.path());
    write_file(tmp.path() / "distances.json", "[[0,1,2,3],[1,0,1,2],[2,1,0,1],[3,2,1,0]]");
    CHECK(status_of([&] { (void)tlor::load_instance(options_for(tmp.path())); }) == Status::invalid_argument);
}

TEST_CASE("io: demand for an inactive point is rejected") {
    TempDir tmp;
    copy_tiny(tmp.path());
    write_file(tmp.path() / "daily_demand.csv", read_file(tmp.path() / "daily_demand.csv") + "3;2023-04-14;7\n");
    auto message = error_message(options_for(tmp.path()));
    CHECK(message.find("3") != std::string::npos);
    CHECK(status_of([&] { (void)tlor::load_instance(options_for(tmp.path())); }) == Status::invalid_argument);
}

TEST_CASE("io: exactly one depot") {
    TempDir tmp;
    copy_tiny(tmp.path());
    write_file(tmp.path() / "active_points.csv",
               "point_id;capacity_kg;is_depot;active\n0;30;False;True\n1;200;False;True\n2;50;False;True\n"
               "3;;True;False\n4;;True;False\n");
    CHECK(status_of([&] { (void)tlor::load_instance(options_for(tmp.path())); }) == Status::invalid_argument);
}

TEST_CASE("io: malformed values name the file and the column") {
    TempDir tmp;
    copy_tiny(tmp.path());
    write_file(tmp.path() / "daily_demand.csv", "point_id;date;daily_kg\n0;2023-04-14;ten\n");
    auto message = error_message(options_for(tmp.path()));
    CHECK(message.find("daily_demand.csv") != std::string::npos);
    CHECK(message.find("daily_kg") != std::string::npos);
    CHECK(status_of([&] { (void)tlor::load_instance(options_for(tmp.path())); }) == Status::invalid_argument);

    write_file(tmp.path() / "daily_demand.csv", "point_id;day;daily_kg\n0;2023-04-14;10\n");
    CHECK(error_message(options_for(tmp.path())).find("date") != std::string::npos);
}

TEST_CASE("io: real data loads") {
    auto root = std::filesystem::path(TLOR_SOURCE_DIR);
    if (!std::filesystem::exists(root / "data/tables/daily_demand.csv")) {
        MESSAGE("data/tables is absent, skipped");
        return;
    }
    auto loaded = tlor::load_instance({root / "data/tables", root / "Maps/offline map/data", {}, true});
    const auto& i = loaded.instance;
    CHECK(i.node_count() == 329);
    CHECK(i.depot() == 328);
    CHECK(i.day_count() == 63);
    CHECK(i.active_point_count() == 300);
    CHECK(tlor::format_date(loaded.first_date) == "2023-04-14");
}

TEST_CASE("io: a mistyped date is named before the day range is built") {
    TempDir tmp;
    copy_tiny(tmp.path());
    auto text = read_file(tmp.path() / "daily_demand.csv");
    text.replace(text.find("2023-04-15"), 10, "9023-04-15");
    write_file(tmp.path() / "daily_demand.csv", text);
    auto message = error_message(options_for(tmp.path()));
    CHECK(message.find("9023-04-15") != std::string::npos);
    CHECK(status_of([&] { (void)tlor::load_instance(options_for(tmp.path())); }) == Status::invalid_argument);
}
