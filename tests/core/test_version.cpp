#include <doctest/doctest.h>

#include <tlor/version.hpp>

TEST_CASE("version: build information is filled") {
    CHECK(tlor::library_version() == "0.1.0");
    CHECK_FALSE(tlor::git_commit().empty());
    CHECK_FALSE(tlor::build_type().empty());
    CHECK_FALSE(tlor::compiler().empty());
    CHECK_FALSE(tlor::os_name().empty());
}
