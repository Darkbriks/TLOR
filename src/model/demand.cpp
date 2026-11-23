#include <tlor/demand.hpp>

#include <tlor/error.hpp>

#include <cmath>
#include <string>

namespace tlor {

Demand::Demand(std::int32_t day_count, std::int32_t node_count, std::vector<double> kg)
    : day_count_(day_count), node_count_(node_count), kg_(std::move(kg)) {
    if (day_count_ < 1 || node_count_ < 1) {
        throw Error(Status::invalid_argument, "demand needs at least one day and one node");
    }
    if (kg_.size() != static_cast<std::size_t>(day_count_) * static_cast<std::size_t>(node_count_)) {
        throw Error(Status::invalid_argument,
                    "demand has " + std::to_string(kg_.size()) + " values, expected day_count * node_count");
    }
    for (double v : kg_) {
        if (!std::isfinite(v) || v < 0) {
            throw Error(Status::invalid_argument, "demand values must be finite and non-negative");
        }
    }
}

double Demand::at(Day day, Node node) const noexcept {
    return kg_[static_cast<std::size_t>(day) * static_cast<std::size_t>(node_count_) + static_cast<std::size_t>(node)];
}

std::int32_t Demand::day_count() const noexcept { return day_count_; }

std::int32_t Demand::node_count() const noexcept { return node_count_; }

std::span<const double> Demand::data() const noexcept { return kg_; }

}
