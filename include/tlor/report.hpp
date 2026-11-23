/**
 * @file report.hpp
 * @brief Time measurement of library calls, written as a JSON report.
 *
 * Every function taking a Report* measures nothing when it is null. Phases and counters go to the innermost
 * open container: a phase, else a call, else the report itself.
 */
#pragma once

#include <tlor/error.hpp>
#include <tlor/indicators.hpp>
#include <tlor/params.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace tlor {

/** @brief Size of the instance a call ran on. */
struct InstanceSize {
    /** @brief Number of nodes, depot included. */
    std::int32_t nodes = 0;
    /** @brief Number of collection points. */
    std::int32_t active_points = 0;
    /** @brief Number of days. */
    std::int32_t days = 0;
    /** @brief Number of trucks. */
    std::int32_t trucks = 0;
};

/** @brief What a call record describes. */
struct CallInfo {
    /** @brief Step, such as "route". */
    std::string step;
    /** @brief Method name. */
    std::string method;
    /** @brief Parameters, defaults filled. */
    Params params;
    /** @brief Instance size. */
    InstanceSize size;
};

/** @brief Phases, counters and call records of a run, written as JSON (schema version 1). */
class Report {
public:
    /** @brief Empty report; records the start time. */
    Report();
    /** @brief Frees the report. */
    ~Report();
    Report(const Report&) = delete;
    Report& operator=(const Report&) = delete;
    /** @brief Moves a report. */
    Report(Report&&) noexcept;
    /** @brief Moves a report. */
    Report& operator=(Report&&) noexcept;

    /** @brief Adds a free key and value to the context (author, experiment name...). */
    void set_context(std::string key, std::string value);

    /** @brief Opens a phase in the innermost container and returns its id; phases of the same name are aggregated. */
    std::uint64_t begin_phase(std::string_view name);

    /** @brief Closes the innermost phase; throws Status::invalid_argument when it is not a phase. */
    void end_phase();

    /**
     * @brief Closes the phase of this id if it is still open, closing first whatever is open above it.
     *
     * A call closed this way is recorded with Status::internal_error.
     */
    void end_phase(std::uint64_t id);

    /** @brief Adds delta to a counter of the innermost container. */
    void count(std::string_view name, std::int64_t delta);

    /** @brief Opens a call: a new record at top level, else a phase named "call:<step>/<method>". */
    void begin_call(CallInfo info);

    /**
     * @brief Closes the innermost call; throws Status::invalid_argument when no call is open.
     *
     * Phases still open inside the call are closed first and counted in its "unclosed_phases" counter.
     */
    void end_call(Status status, std::optional<Indicators> indicators);

    /** @brief Whether a call is open. */
    [[nodiscard]] bool in_call() const noexcept;

    /** @brief The report as indented JSON. */
    [[nodiscard]] std::string to_json() const;

    /** @brief Writes the JSON to a file; throws Status::io_error with the path on failure. */
    void write_json(const std::filesystem::path& path) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/** @brief Phase open for the lifetime of the object; does nothing when the report is null. */
class Phase {
public:
    /** @brief Opens the phase. */
    Phase(Report* report, std::string_view name);
    /** @brief Closes the phase. */
    ~Phase();
    Phase(const Phase&) = delete;
    Phase& operator=(const Phase&) = delete;

private:
    Report* report_;
    std::uint64_t id_ = 0;
};

/** @brief Adds delta to a counter; does nothing when the report is null. */
void count(Report* report, std::string_view name, std::int64_t delta = 1);

}
