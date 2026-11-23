/**
 * @file random.hpp
 * @brief Random numbers that are identical on every platform for a given seed.
 */
#pragma once

#include <cstdint>
#include <random>
#include <span>
#include <utility>

namespace tlor {

/** @brief std::mt19937_64 with the library's own range conversion, so results do not depend on the standard library. */
class Random {
public:
    /** @brief Engine seeded with seed. */
    explicit Random(std::uint64_t seed);

    /** @brief Next raw 64-bit value. */
    std::uint64_t next();

    /** @brief Uniform value in [0, n); throws Status::invalid_argument when n is 0. */
    std::uint64_t below(std::uint64_t n);

    /** @brief Fisher–Yates shuffle, from the last element down. */
    template <class T>
    void shuffle(std::span<T> values) {
        for (std::size_t i = values.size(); i > 1; --i) {
            const auto j = static_cast<std::size_t>(below(i));
            std::swap(values[i - 1], values[j]);
        }
    }

private:
    std::mt19937_64 engine_;
};

}
