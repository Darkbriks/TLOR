#include <tlor/indicators.hpp>

#include <tlor/error.hpp>

#include <algorithm>

namespace tlor {

bool is_indicator_name(std::string_view name) noexcept { return std::ranges::find(indicator_names, name) != indicator_names.end(); }

double Indicators::at(std::string_view name) const {
    auto it = values.find(name);
    if (it == values.end()) {
        throw Error(Status::invalid_argument, "indicator '" + std::string(name) + "' is absent");
    }
    return it->second;
}

}
