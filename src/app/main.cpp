#include <tlor/check.hpp>
#include <tlor/error.hpp>
#include <tlor/evaluate.hpp>
#include <tlor/fleet.hpp>
#include <tlor/indicators.hpp>
#include <tlor/io.hpp>
#include <tlor/params.hpp>
#include <tlor/pipeline.hpp>
#include <tlor/report.hpp>
#include <tlor/state.hpp>

#include "io/csv.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view usage =
    "usage: TLOR --tables DIR --maps DIR --out DIR --trucks N --truck-capacity-kg X --truck-max-work-s X\n"
    "            [--service-time FILE] [--first D] [--last D] [--horizon H] [--step K]\n"
    "            [--schedule NAME] [--route NAME] [--improve NAME]\n"
    "            [--param schedule|route|improve|evaluation.NAME=VALUE]... [--weight NAME=VALUE]...\n"
    "            [--weekends-off] [--report FILE] [--check-each-step]\n"
    "D is a day index or a YYYY-MM-DD date.\n";

class UsageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Options {
    std::filesystem::path tables;
    std::filesystem::path maps;
    std::filesystem::path out;
    std::filesystem::path service_time;
    std::filesystem::path report;
    std::int32_t trucks = 0;
    double capacity_kg = 0;
    double max_work_s = 0;
    std::optional<std::string> first;
    std::optional<std::string> last;
    std::int32_t horizon = 7;
    std::int32_t step = 1;
    tlor::MethodChoice schedule{"threshold", {}};
    tlor::MethodChoice route{"nearest_neighbour", {}};
    std::optional<tlor::MethodChoice> improve;
    tlor::Params improve_params;
    tlor::Params evaluation;
    tlor::Params weights;
    bool weekends_off = false;
    bool check_each_step = false;
};

std::int32_t integer_of(std::string_view option, std::string_view text) {
    const auto value = tlor::io::parse_integer(text);
    if (!value || *value < std::numeric_limits<std::int32_t>::min() ||
        *value > std::numeric_limits<std::int32_t>::max()) {
        throw UsageError(std::string(option) + " expects an integer, not '" + std::string(text) + "'");
    }
    return static_cast<std::int32_t>(*value);
}

double number_of(std::string_view option, std::string_view text) {
    const auto value = tlor::io::parse_double(text);
    if (!value) {
        throw UsageError(std::string(option) + " expects a number, not '" + std::string(text) + "'");
    }
    return *value;
}

tlor::Value value_of(std::string_view text) {
    if (const auto value = tlor::io::parse_integer(text)) {
        return *value;
    }
    if (const auto value = tlor::io::parse_double(text)) {
        return *value;
    }
    if (text == "true" || text == "false") {
        return text == "true";
    }
    return std::string(text);
}

std::pair<std::string, tlor::Value> assignment(std::string_view option, std::string_view text) {
    const auto equal = text.find('=');
    if (equal == std::string_view::npos || equal == 0) {
        throw UsageError(std::string(option) + " expects NAME=VALUE, not '" + std::string(text) + "'");
    }
    return {std::string(text.substr(0, equal)), value_of(text.substr(equal + 1))};
}

void add_param(Options& o, std::string_view text) {
    const auto dot = text.find('.');
    const auto target = text.substr(0, dot == std::string_view::npos ? 0 : dot);
    tlor::Params* params = target == "schedule"     ? &o.schedule.params
                           : target == "route"      ? &o.route.params
                           : target == "improve"    ? &o.improve_params
                           : target == "evaluation" ? &o.evaluation
                                                    : nullptr;
    if (params == nullptr) {
        throw UsageError("--param expects schedule|route|improve|evaluation.NAME=VALUE, not '" + std::string(text) +
                         "'");
    }
    auto [name, value] = assignment("--param", text.substr(dot + 1));
    params->set(std::move(name), std::move(value));
}

Options parse(std::span<char*> args) {
    Options o;
    std::set<std::string, std::less<>> seen;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view option = args[i];
        const auto value = [&]() -> std::string_view {
            if (i + 1 >= args.size()) {
                throw UsageError(std::string(option) + " needs a value");
            }
            return args[++i];
        };
        if (option != "--param" && option != "--weight" && !seen.insert(std::string(option)).second) {
            throw UsageError(std::string(option) + " is given twice");
        }
        if (option == "--tables") {
            o.tables = std::filesystem::path(value());
        } else if (option == "--maps") {
            o.maps = std::filesystem::path(value());
        } else if (option == "--out") {
            o.out = std::filesystem::path(value());
        } else if (option == "--service-time") {
            o.service_time = std::filesystem::path(value());
        } else if (option == "--report") {
            o.report = std::filesystem::path(value());
        } else if (option == "--trucks") {
            o.trucks = integer_of(option, value());
            if (o.trucks < 1) {
                throw UsageError("--trucks expects at least 1");
            }
        } else if (option == "--truck-capacity-kg") {
            o.capacity_kg = number_of(option, value());
        } else if (option == "--truck-max-work-s") {
            o.max_work_s = number_of(option, value());
        } else if (option == "--first") {
            o.first = std::string(value());
        } else if (option == "--last") {
            o.last = std::string(value());
        } else if (option == "--horizon") {
            o.horizon = integer_of(option, value());
        } else if (option == "--step") {
            o.step = integer_of(option, value());
        } else if (option == "--schedule") {
            o.schedule.name = value();
        } else if (option == "--route") {
            o.route.name = value();
        } else if (option == "--improve") {
            o.improve = tlor::MethodChoice{std::string(value()), {}};
        } else if (option == "--param") {
            add_param(o, value());
        } else if (option == "--weight") {
            auto [name, weight] = assignment(option, value());
            o.weights.set(std::move(name), std::move(weight));
        } else if (option == "--weekends-off") {
            o.weekends_off = true;
        } else if (option == "--check-each-step") {
            o.check_each_step = true;
        } else {
            throw UsageError("unknown option '" + std::string(option) + "'");
        }
    }
    for (const auto* required : {"--tables", "--maps", "--out", "--trucks", "--truck-capacity-kg", "--truck-max-work-s"}) {
        if (!seen.contains(required)) {
            throw UsageError(std::string(required) + " is required");
        }
    }
    if (o.improve) {
        o.improve->params = o.improve_params;
    } else if (!o.improve_params.values().empty()) {
        throw UsageError("--param improve.NAME=VALUE needs --improve");
    }
    return o;
}

tlor::Day day_of(std::string_view option, const std::string& text, std::chrono::sys_days first_date) {
    if (tlor::io::parse_integer(text)) {
        return integer_of(option, text);
    }
    if (text.find('-') != std::string::npos) {
        return static_cast<tlor::Day>((tlor::parse_date(text) - first_date).count());
    }
    throw UsageError(std::string(option) + " expects a day index or a YYYY-MM-DD date, not '" + text + "'");
}

tlor::Fleet fleet_of(const Options& o, const tlor::Instance& instance, std::chrono::sys_days first_date) {
    const auto trucks = static_cast<std::size_t>(o.trucks);
    std::vector<std::uint8_t> available(static_cast<std::size_t>(instance.day_count()) * trucks, 1);
    if (o.weekends_off) {
        for (tlor::Day day = 0; day < instance.day_count(); ++day) {
            const std::chrono::weekday weekday{first_date + std::chrono::days{day}};
            if (weekday == std::chrono::Saturday || weekday == std::chrono::Sunday) {
                std::fill_n(available.begin() + static_cast<std::ptrdiff_t>(static_cast<std::size_t>(day) * trucks),
                            trucks, std::uint8_t{0});
            }
        }
    }
    return {std::vector<tlor::Truck>(trucks, tlor::Truck{o.capacity_kg, o.max_work_s}), instance.day_count(),
            std::move(available)};
}

std::string number(double value) {
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof buffer, value);
    return {buffer, result.ptr};
}

std::string command_line(std::span<char*> args) {
    std::string text = "TLOR";
    for (const char* arg : args) {
        text += ' ';
        text += tlor::io::shown(std::filesystem::path(arg));
    }
    return text;
}

int run(std::span<char*> args) {
    const Options o = parse(args);
    auto loaded = tlor::load_instance({o.tables, o.maps, o.service_time, true});
    const tlor::Instance& instance = loaded.instance;
    const tlor::Fleet fleet = fleet_of(o, instance, loaded.first_date);
    tlor::RollingOptions options;
    options.first_day = o.first ? day_of("--first", *o.first, loaded.first_date) : 0;
    options.last_day = o.last ? day_of("--last", *o.last, loaded.first_date) : instance.day_count() - 1;
    options.horizon = o.horizon;
    options.step = o.step;
    options.schedule = o.schedule;
    options.route = o.route;
    options.improve = o.improve;
    options.weights = o.weights;
    options.evaluation = o.evaluation;
    options.check_each_step = o.check_each_step;

    std::optional<tlor::Report> report;
    if (!o.report.empty()) {
        report.emplace();
        report->set_context("command_line", command_line(args));
        report->set_context("first_date", tlor::format_date(loaded.first_date));
    }
    const tlor::State initial = tlor::State::initial(instance.node_count());
    const auto result = tlor::run_rolling(instance, fleet, initial, options, nullptr, report ? &*report : nullptr);
    tlor::write_solution(o.out, instance, initial, result.solution, instance.demand(), loaded.first_date);
    if (report) {
        report->write_json(o.report);
    }

    for (std::string_view name : tlor::indicator_names) {
        std::cout << name << ' ' << number(result.evaluation.total.at(name)) << '\n';
    }
    std::map<std::string, int, std::less<>> violations;
    for (const auto& violation : result.violations) {
        ++violations[std::string(tlor::to_string(violation.kind))];
    }
    std::cout << "violations " << result.violations.size() << '\n';
    for (const auto& [kind, count] : violations) {
        std::cout << "violations." << kind << ' ' << count << '\n';
    }
    return 0;
}

}

int main(int argc, char** argv) {
    const std::span<char*> args(argv + 1, static_cast<std::size_t>(argc > 0 ? argc - 1 : 0));
    try {
        if (args.size() == 1 && (std::string_view(args[0]) == "--help" || std::string_view(args[0]) == "-h")) {
            std::cout << usage;
            return 0;
        }
        return run(args);
    } catch (const UsageError& e) {
        std::cerr << "TLOR: " << e.what() << '\n' << usage;
        return 2;
    } catch (const tlor::Error& e) {
        std::cerr << "TLOR: " << tlor::to_string(e.status()) << ": " << e.what() << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "TLOR: " << e.what() << '\n';
        return 1;
    }
}
