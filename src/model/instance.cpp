#include <tlor/instance.hpp>

#include <tlor/error.hpp>

#include <cmath>

namespace tlor {

namespace {

[[noreturn]] void invalid(const std::string& message) { throw Error(Status::invalid_argument, message); }

void check_size(const char* name, std::size_t actual, std::size_t expected) {
    if (actual != expected) {
        invalid(std::string(name) + " has " + std::to_string(actual) + " values, expected " + std::to_string(expected));
    }
}

void check_values(const char* name, const std::vector<double>& values) {
    for (double v : values) {
        if (!std::isfinite(v) || v < 0) {
            invalid(std::string(name) + " values must be finite and non-negative");
        }
    }
}

std::vector<double> checked_demand(const InstanceData& data) {
    if (data.node_count < 1) {
        invalid("an instance needs at least one node");
    }
    if (data.day_count < 1) {
        invalid("an instance needs at least one day");
    }
    check_size("demand_kg", data.demand_kg.size(),
               static_cast<std::size_t>(data.day_count) * static_cast<std::size_t>(data.node_count));
    return data.demand_kg;
}

}

Instance::Instance(InstanceData data)
    : node_count_(data.node_count),
      depot_(data.depot),
      distance_m_(std::move(data.distance_m)),
      duration_s_(std::move(data.duration_s)),
      service_s_(std::move(data.service_s)),
      capacity_kg_(std::move(data.capacity_kg)),
      active_(std::move(data.active)),
      demand_(data.day_count, data.node_count, checked_demand(data)) {
    const auto n = static_cast<std::size_t>(node_count_);
    if (depot_ < 0 || depot_ >= node_count_) {
        invalid("depot " + std::to_string(depot_) + " is out of range");
    }
    check_size("distance_m", distance_m_.size(), n * n);
    check_size("duration_s", duration_s_.size(), n * n);
    check_size("service_s", service_s_.size(), n);
    check_size("capacity_kg", capacity_kg_.size(), n);
    check_size("active", active_.size(), n);
    check_values("distance_m", distance_m_);
    check_values("duration_s", duration_s_);
    check_values("service_s", service_s_);
    check_values("capacity_kg", capacity_kg_);
    if (active_[static_cast<std::size_t>(depot_)] != 0) {
        invalid("the depot cannot be an active point");
    }
    for (Node node = 0; node < node_count_; ++node) {
        if (active(node)) {
            if (capacity(node) <= 0) {
                invalid("active point " + std::to_string(node) + " has no capacity");
            }
            ++active_point_count_;
        }
    }
}

std::int32_t Instance::node_count() const noexcept { return node_count_; }

Node Instance::depot() const noexcept { return depot_; }

std::int32_t Instance::day_count() const noexcept { return demand_.day_count(); }

double Instance::distance(Node from, Node to) const noexcept {
    return distance_m_[static_cast<std::size_t>(from) * static_cast<std::size_t>(node_count_) + static_cast<std::size_t>(to)];
}

double Instance::duration(Node from, Node to) const noexcept {
    return duration_s_[static_cast<std::size_t>(from) * static_cast<std::size_t>(node_count_) + static_cast<std::size_t>(to)];
}

double Instance::service(Node node) const noexcept { return service_s_[static_cast<std::size_t>(node)]; }

double Instance::capacity(Node node) const noexcept { return capacity_kg_[static_cast<std::size_t>(node)]; }

bool Instance::active(Node node) const noexcept { return active_[static_cast<std::size_t>(node)] != 0; }

bool Instance::is_point(Node node) const noexcept {
    return node >= 0 && node < node_count_ && node != depot_ && active(node);
}

const Demand& Instance::demand() const noexcept { return demand_; }

std::span<const double> Instance::distance_data() const noexcept { return distance_m_; }

std::span<const double> Instance::duration_data() const noexcept { return duration_s_; }

std::span<const double> Instance::service_data() const noexcept { return service_s_; }

std::span<const double> Instance::capacity_data() const noexcept { return capacity_kg_; }

std::span<const std::uint8_t> Instance::active_data() const noexcept { return active_; }

std::vector<Node> Instance::active_points() const {
    std::vector<Node> points;
    points.reserve(static_cast<std::size_t>(active_point_count_));
    for (Node node = 0; node < node_count_; ++node) {
        if (is_point(node)) {
            points.push_back(node);
        }
    }
    return points;
}

std::int32_t Instance::active_point_count() const noexcept { return active_point_count_; }

void Instance::set_attribute(std::string name, std::vector<double> values) {
    check_size("attribute", values.size(), static_cast<std::size_t>(node_count_));
    attributes_.insert_or_assign(std::move(name), std::move(values));
}

const std::vector<double>* Instance::attribute(std::string_view name) const noexcept {
    auto it = attributes_.find(name);
    return it == attributes_.end() ? nullptr : &it->second;
}

}
