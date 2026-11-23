/**
 * @file version.hpp
 * @brief Version and build information of the TLOR library.
 */
#pragma once

#include <string_view>

namespace tlor {

/** @brief Version of the library, such as "0.1.0". */
std::string_view library_version();

/** @brief Short git commit the library was configured from, or "unknown". */
std::string_view git_commit();

/** @brief Build type, such as "Debug" or "Release". */
std::string_view build_type();

/** @brief Compiler identifier and version. */
std::string_view compiler();

/** @brief Name of the operating system the library was built for. */
std::string_view os_name();

}
