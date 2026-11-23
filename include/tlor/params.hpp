/**
 * @file params.hpp
 * @brief Named values used for method parameters and objective weights.
 */
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>

namespace tlor {

/** @brief Value of a parameter. */
using Value = std::variant<std::int64_t, double, bool, std::string>;

/** @brief Set of named values, sorted by name. */
class Params {
public:
    /** @brief Sets a value, replacing any previous one of the same name. */
    void set(std::string name, Value value);

    /** @brief Whether a value of this name exists. */
    [[nodiscard]] bool contains(std::string_view name) const;

    /** @brief Value of this name, or nullptr. */
    [[nodiscard]] const Value* find(std::string_view name) const;

    /** @brief Integer value; throws Status::invalid_param when missing or of another type. */
    [[nodiscard]] std::int64_t get_int(std::string_view name) const;

    /** @brief Real value, an integer being converted; throws Status::invalid_param otherwise. */
    [[nodiscard]] double get_double(std::string_view name) const;

    /** @brief Boolean value; throws Status::invalid_param when missing or of another type. */
    [[nodiscard]] bool get_bool(std::string_view name) const;

    /** @brief String value; throws Status::invalid_param when missing or of another type. */
    [[nodiscard]] const std::string& get_string(std::string_view name) const;

    /** @brief All values, sorted by name. */
    [[nodiscard]] const std::map<std::string, Value, std::less<>>& values() const noexcept;

private:
    std::map<std::string, Value, std::less<>> values_;
};

}
