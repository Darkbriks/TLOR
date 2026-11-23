/**
 * @file fleet.hpp
 * @brief Trucks and their availability day by day.
 */
#pragma once

#include <tlor/types.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace tlor {

/** @brief One truck. */
struct Truck {
    /** @brief Load limit of one route, in kilograms. */
    double capacity_kg = 0;
    /** @brief Maximum work time in a day, in seconds. */
    double max_work_s = 0;
};

/** @brief Trucks and which of them are available on each day. */
class Fleet {
public:
    /** @brief Builds a fleet; available is [day * truck_count + truck], empty meaning always available. */
    Fleet(std::vector<Truck> trucks, std::int32_t day_count, std::vector<std::uint8_t> available);

    /** @brief Number of trucks. */
    [[nodiscard]] std::int32_t truck_count() const noexcept;

    /** @brief Number of days. */
    [[nodiscard]] std::int32_t day_count() const noexcept;

    /** @brief Truck of this index; throws Status::invalid_argument when out of range. */
    [[nodiscard]] const Truck& truck(TruckId truck) const;

    /** @brief Whether a truck is available on a day. */
    [[nodiscard]] bool available(Day day, TruckId truck) const noexcept;

    /** @brief Number of trucks available on a day. */
    [[nodiscard]] std::int32_t available_count(Day day) const noexcept;

    /** @brief Trucks available on a day, in index order. */
    [[nodiscard]] std::vector<TruckId> available_trucks(Day day) const;

    /** @brief Availability of every truck on every day, day-major. */
    [[nodiscard]] std::span<const std::uint8_t> available_data() const noexcept;

private:
    std::vector<Truck> trucks_;
    std::int32_t day_count_;
    std::vector<std::uint8_t> available_;
};

}
