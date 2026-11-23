#include <tlor/version.hpp>

#include <tlor/detail/build_info.hpp>

namespace tlor {

std::string_view library_version() { return detail::library_version; }

std::string_view git_commit() { return detail::git_commit; }

std::string_view build_type() { return TLOR_BUILD_TYPE; }

std::string_view compiler() { return detail::compiler; }

std::string_view os_name() { return detail::os_name; }

}
