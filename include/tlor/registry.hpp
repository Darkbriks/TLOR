/**
 * @file registry.hpp
 * @brief Methods of each step, chosen by name, with their declared parameters.
 */
#pragma once

#include <tlor/fleet.hpp>
#include <tlor/instance.hpp>
#include <tlor/params.hpp>
#include <tlor/plan.hpp>
#include <tlor/report.hpp>
#include <tlor/routes.hpp>
#include <tlor/solution.hpp>
#include <tlor/state.hpp>

#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tlor {

/** @brief Step a method belongs to. */
enum class Step { schedule, route, improve };

/** @brief Name of a step: "schedule", "route" or "improve". */
std::string_view to_string(Step step);

/** @brief Type of a declared parameter. */
enum class ParamType { integer, real, boolean, string };

/** @brief Declaration of one method parameter. */
struct ParamSpec {
    /** @brief Parameter name. */
    std::string name;
    /** @brief Expected type; an integer is accepted for a real. */
    ParamType type = ParamType::real;
    /** @brief Value used when the caller gives none. */
    Value default_value;
    /** @brief Inclusive lower bound, for numbers. */
    std::optional<double> min;
    /** @brief Inclusive upper bound, for numbers. */
    std::optional<double> max;
    /** @brief Human-readable description. */
    std::string description;
};

/** @brief Name, description and parameters of a method. */
struct MethodInfo {
    /** @brief Method name, unique within its step. */
    std::string name;
    /** @brief Human-readable description. */
    std::string description;
    /** @brief Declared parameters. */
    std::vector<ParamSpec> params;
    /** @brief Whether the method ships with the library; built-in methods cannot be removed. */
    bool builtin = false;
};

/** @brief Inputs of a schedule method. */
struct ScheduleRequest {
    const Instance& instance;
    const Fleet& fleet;
    const State& start_state;
    Day start_day;
    std::int32_t horizon;
    const Params& params;
    const Params& weights;
    Report* report;
};

/** @brief Inputs of a route method: the points of one day and the mass to collect at each. */
struct RouteRequest {
    const Instance& instance;
    const Fleet& fleet;
    Day day;
    std::span<const Node> points;
    std::span<const double> loads_kg;
    const Params& params;
    const Params& weights;
    Report* report;
};

/** @brief Inputs of an improve method, including the route method to re-route with. */
struct ImproveRequest {
    const Instance& instance;
    const Fleet& fleet;
    const State& start_state;
    const Solution& solution;
    std::string_view route_method;
    const Params& route_params;
    const Params& params;
    const Params& weights;
    Report* report;
};

/** @brief A schedule method. */
using ScheduleFn = std::function<Plan(const ScheduleRequest&)>;
/** @brief A route method. */
using RouteFn = std::function<DayRoutes(const RouteRequest&)>;
/** @brief An improve method. */
using ImproveFn = std::function<Solution(const ImproveRequest&)>;

/** @brief Methods of every step, by name. Process-wide and not thread-safe. */
class Registry {
public:
    /** @brief The process-wide registry; its first use registers the built-in methods. */
    static Registry& global();

    /** @brief Adds a schedule method; throws Status::invalid_argument when the name is taken. */
    void add_schedule(MethodInfo info, ScheduleFn fn);

    /** @brief Adds a route method; throws Status::invalid_argument when the name is taken. */
    void add_route(MethodInfo info, RouteFn fn);

    /** @brief Adds an improve method; throws Status::invalid_argument when the name is taken. */
    void add_improve(MethodInfo info, ImproveFn fn);

    /** @brief Removes a method; Status::unknown_method when absent, Status::invalid_argument when built in. */
    void remove(Step step, std::string_view name);

    /** @brief Methods of a step, sorted by name. */
    [[nodiscard]] std::vector<const MethodInfo*> methods(Step step) const;

    /** @brief Description of a method; throws Status::unknown_method when absent. */
    [[nodiscard]] const MethodInfo& info(Step step, std::string_view name) const;

    /** @brief Schedule method of this name; throws Status::unknown_method when absent. */
    [[nodiscard]] const ScheduleFn& schedule_fn(std::string_view name) const;

    /** @brief Route method of this name; throws Status::unknown_method when absent. */
    [[nodiscard]] const RouteFn& route_fn(std::string_view name) const;

    /** @brief Improve method of this name; throws Status::unknown_method when absent. */
    [[nodiscard]] const ImproveFn& improve_fn(std::string_view name) const;

private:
    template <class Fn>
    struct Entry {
        MethodInfo info;
        Fn fn;
    };

    std::map<std::string, Entry<ScheduleFn>, std::less<>> schedule_;
    std::map<std::string, Entry<RouteFn>, std::less<>> route_;
    std::map<std::string, Entry<ImproveFn>, std::less<>> improve_;
};

/** @brief Checks parameters against a method's declarations and fills defaults; throws Status::invalid_param. */
Params validate_params(const MethodInfo& method, const Params& given);

}
