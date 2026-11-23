/**
 * @file types.hpp
 * @brief Integer types used to name nodes, days and trucks.
 */
#pragma once

#include <cstdint>

namespace tlor {

/** @brief Index of a node: a collection point or the depot. */
using Node = std::int32_t;

/** @brief Index of a day, 0 being the first day of the data. */
using Day = std::int32_t;

/** @brief Index of a truck in a fleet. */
using TruckId = std::int32_t;

}
