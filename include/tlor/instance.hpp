/**
 * @file instance.hpp
 * @brief Problem data: nodes, matrices, capacities and predicted demand.
 */
#pragma once

#include <tlor/demand.hpp>
#include <tlor/types.hpp>

#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tlor {

/** @brief Raw arrays an instance is built from. Matrices are row-major, [from * node_count + to]. */
struct InstanceData {
    std::int32_t node_count = 0;
    Node depot = 0;
    std::int32_t day_count = 0;
    std::vector<double> distance_m;
    std::vector<double> duration_s;
    std::vector<double> service_s;
    std::vector<double> capacity_kg;
    std::vector<std::uint8_t> active;
    std::vector<double> demand_kg;
};

/** @brief Validated problem data, plus named per-node attributes for methods. */
class Instance {
public:
    /** @brief Validates and takes the data; throws Status::invalid_argument on inconsistent data. */
    explicit Instance(InstanceData data);

    /** @brief Number of nodes, depot included. */
    [[nodiscard]] std::int32_t node_count() const noexcept;

    /** @brief Index of the depot. */
    [[nodiscard]] Node depot() const noexcept;

    /** @brief Number of days of the predicted demand. */
    [[nodiscard]] std::int32_t day_count() const noexcept;

    /** @brief Road distance from one node to another, in metres. */
    [[nodiscard]] double distance(Node from, Node to) const noexcept;

    /** @brief Travel time from one node to another, in seconds. */
    [[nodiscard]] double duration(Node from, Node to) const noexcept;

    /** @brief Service time at a node, in seconds; for the depot, the unloading time at each route end. */
    [[nodiscard]] double service(Node node) const noexcept;

    /** @brief Container capacity of a node, in kilograms. */
    [[nodiscard]] double capacity(Node node) const noexcept;

    /** @brief Whether a node is active. */
    [[nodiscard]] bool active(Node node) const noexcept;

    /** @brief Whether a node is a collection point: in range, active and not the depot. */
    [[nodiscard]] bool is_point(Node node) const noexcept;

    /** @brief Predicted demand. */
    [[nodiscard]] const Demand& demand() const noexcept;

    /** @brief Distance matrix, row-major. */
    [[nodiscard]] std::span<const double> distance_data() const noexcept;

    /** @brief Duration matrix, row-major. */
    [[nodiscard]] std::span<const double> duration_data() const noexcept;

    /** @brief Service time of every node. */
    [[nodiscard]] std::span<const double> service_data() const noexcept;

    /** @brief Capacity of every node. */
    [[nodiscard]] std::span<const double> capacity_data() const noexcept;

    /** @brief Active flag of every node. */
    [[nodiscard]] std::span<const std::uint8_t> active_data() const noexcept;

    /** @brief Collection points, in index order. */
    [[nodiscard]] std::vector<Node> active_points() const;

    /** @brief Number of collection points. */
    [[nodiscard]] std::int32_t active_point_count() const noexcept;

    /** @brief Sets a named per-node attribute; NaN means unset for that node. */
    void set_attribute(std::string name, std::vector<double> values);

    /** @brief Attribute of this name, or nullptr. */
    [[nodiscard]] const std::vector<double>* attribute(std::string_view name) const noexcept;

private:
    std::int32_t node_count_;
    Node depot_;
    std::vector<double> distance_m_;
    std::vector<double> duration_s_;
    std::vector<double> service_s_;
    std::vector<double> capacity_kg_;
    std::vector<std::uint8_t> active_;
    Demand demand_;
    std::int32_t active_point_count_ = 0;
    std::map<std::string, std::vector<double>, std::less<>> attributes_;
};

}
