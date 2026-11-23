/**
 * @file plan.hpp
 * @brief Which points are collected on each day of a horizon.
 */
#pragma once

#include <tlor/types.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace tlor {

/** @brief Points collected on each day from start to start + horizon - 1. */
class Plan {
public:
    /** @brief Empty plan; throws Status::invalid_argument when horizon < 1 or start < 0. */
    Plan(Day start, std::int32_t horizon);

    /** @brief First day of the plan. */
    [[nodiscard]] Day start() const noexcept;

    /** @brief Number of days. */
    [[nodiscard]] std::int32_t horizon() const noexcept;

    /** @brief Day after the last one. */
    [[nodiscard]] Day end() const noexcept;

    /** @brief Whether a day belongs to the plan. */
    [[nodiscard]] bool contains(Day day) const noexcept;

    /** @brief Collects a point on a day; adding a present point does nothing. */
    void add(Day day, Node point);

    /** @brief Stops collecting a point on a day. */
    void remove(Day day, Node point);

    /** @brief Whether a point is collected on a day. */
    [[nodiscard]] bool collected(Day day, Node point) const;

    /** @brief Points collected on a day, sorted. */
    [[nodiscard]] std::span<const Node> points(Day day) const;

    /** @brief Number of collections over the whole plan. */
    [[nodiscard]] std::int32_t collection_count() const noexcept;

    /** @brief Same days and same points. */
    bool operator==(const Plan&) const = default;

private:
    [[nodiscard]] std::size_t offset(Day day) const;

    Day start_;
    std::vector<std::vector<Node>> days_;
};

}
