#include <tlor/registry.hpp>

#include <tlor/error.hpp>

#include "methods/methods.hpp"

namespace tlor {

namespace {

template <class Map>
auto& find_entry(Map& map, Step step, std::string_view name) {
    auto it = map.find(name);
    if (it == map.end()) {
        throw Error(Status::unknown_method, "no " + std::string(to_string(step)) + " method named '" + std::string(name) + "'");
    }
    return it->second;
}

template <class Map, class Fn>
void add_entry(Map& map, Step step, MethodInfo info, Fn fn) {
    if (info.name.empty()) {
        throw Error(Status::invalid_argument, "a method needs a name");
    }
    if (map.contains(info.name)) {
        throw Error(Status::invalid_argument,
                    "a " + std::string(to_string(step)) + " method named '" + info.name + "' already exists");
    }
    std::string name = info.name;
    map.emplace(std::move(name), typename Map::mapped_type{std::move(info), std::move(fn)});
}

template <class Map>
void remove_entry(Map& map, Step step, std::string_view name) {
    auto& entry = find_entry(map, step, name);
    if (entry.info.builtin) {
        throw Error(Status::invalid_argument, "built-in method '" + std::string(name) + "' cannot be removed");
    }
    map.erase(map.find(name));
}

template <class Map>
std::vector<const MethodInfo*> list(const Map& map) {
    std::vector<const MethodInfo*> out;
    for (const auto& [name, entry] : map) {
        out.push_back(&entry.info);
    }
    return out;
}

std::string_view type_name(ParamType type) {
    switch (type) {
        case ParamType::integer: return "an integer";
        case ParamType::real: return "a number";
        case ParamType::boolean: return "a boolean";
        case ParamType::string: return "a string";
    }
    return "a value";
}

}

std::string_view to_string(Step step) {
    switch (step) {
        case Step::schedule: return "schedule";
        case Step::route: return "route";
        case Step::improve: return "improve";
    }
    return "unknown";
}

Registry& Registry::global() {
    static Registry registry = [] {
        Registry r;
        register_builtin_methods(r);
        return r;
    }();
    return registry;
}

void Registry::add_schedule(MethodInfo info, ScheduleFn fn) { add_entry(schedule_, Step::schedule, std::move(info), std::move(fn)); }

void Registry::add_route(MethodInfo info, RouteFn fn) { add_entry(route_, Step::route, std::move(info), std::move(fn)); }

void Registry::add_improve(MethodInfo info, ImproveFn fn) { add_entry(improve_, Step::improve, std::move(info), std::move(fn)); }

void Registry::remove(Step step, std::string_view name) {
    switch (step) {
        case Step::schedule: remove_entry(schedule_, step, name); break;
        case Step::route: remove_entry(route_, step, name); break;
        case Step::improve: remove_entry(improve_, step, name); break;
    }
}

std::vector<const MethodInfo*> Registry::methods(Step step) const {
    switch (step) {
        case Step::schedule: return list(schedule_);
        case Step::route: return list(route_);
        case Step::improve: return list(improve_);
    }
    return {};
}

const MethodInfo& Registry::info(Step step, std::string_view name) const {
    switch (step) {
        case Step::schedule: return find_entry(schedule_, step, name).info;
        case Step::route: return find_entry(route_, step, name).info;
        case Step::improve: return find_entry(improve_, step, name).info;
    }
    throw Error(Status::unknown_method, "unknown step");
}

const ScheduleFn& Registry::schedule_fn(std::string_view name) const { return find_entry(schedule_, Step::schedule, name).fn; }

const RouteFn& Registry::route_fn(std::string_view name) const { return find_entry(route_, Step::route, name).fn; }

const ImproveFn& Registry::improve_fn(std::string_view name) const { return find_entry(improve_, Step::improve, name).fn; }

Params validate_params(const MethodInfo& method, const Params& given) {
    auto fail = [&](const std::string& param, const std::string& why) {
        throw Error(Status::invalid_param, "parameter '" + param + "' of method '" + method.name + "' " + why);
    };
    Params result;
    for (const auto& [name, value] : given.values()) {
        auto spec = std::ranges::find(method.params, name, &ParamSpec::name);
        if (spec == method.params.end()) {
            fail(name, "does not exist");
        }
        Value checked = value;
        bool ok = false;
        switch (spec->type) {
            case ParamType::integer: ok = std::holds_alternative<std::int64_t>(value); break;
            case ParamType::real:
                if (const auto* i = std::get_if<std::int64_t>(&value)) {
                    checked = static_cast<double>(*i);
                }
                ok = std::holds_alternative<double>(checked);
                break;
            case ParamType::boolean: ok = std::holds_alternative<bool>(value); break;
            case ParamType::string: ok = std::holds_alternative<std::string>(value); break;
        }
        if (!ok) {
            fail(name, "must be " + std::string(type_name(spec->type)));
        }
        if (spec->type == ParamType::integer || spec->type == ParamType::real) {
            const double number = spec->type == ParamType::integer ? static_cast<double>(std::get<std::int64_t>(checked))
                                                                   : std::get<double>(checked);
            if (!(number == number) || (spec->min && number < *spec->min) || (spec->max && number > *spec->max)) {
                fail(name, "is out of range");
            }
        }
        result.set(name, std::move(checked));
    }
    for (const ParamSpec& spec : method.params) {
        if (!result.contains(spec.name)) {
            result.set(spec.name, spec.default_value);
        }
    }
    return result;
}

}
