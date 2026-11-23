#include <tlor/params.hpp>

#include <tlor/error.hpp>

namespace tlor {

namespace {

const Value& require(const Params& params, std::string_view name) {
    const Value* value = params.find(name);
    if (value == nullptr) {
        throw Error(Status::invalid_param, "missing parameter '" + std::string(name) + "'");
    }
    return *value;
}

[[noreturn]] void wrong_type(std::string_view name, std::string_view expected) {
    throw Error(Status::invalid_param, "parameter '" + std::string(name) + "' is not " + std::string(expected));
}

}

void Params::set(std::string name, Value value) { values_.insert_or_assign(std::move(name), std::move(value)); }

bool Params::contains(std::string_view name) const { return values_.contains(name); }

const Value* Params::find(std::string_view name) const {
    auto it = values_.find(name);
    return it == values_.end() ? nullptr : &it->second;
}

std::int64_t Params::get_int(std::string_view name) const {
    const Value& value = require(*this, name);
    if (const auto* v = std::get_if<std::int64_t>(&value)) {
        return *v;
    }
    wrong_type(name, "an integer");
}

double Params::get_double(std::string_view name) const {
    const Value& value = require(*this, name);
    if (const auto* v = std::get_if<double>(&value)) {
        return *v;
    }
    if (const auto* v = std::get_if<std::int64_t>(&value)) {
        return static_cast<double>(*v);
    }
    wrong_type(name, "a number");
}

bool Params::get_bool(std::string_view name) const {
    const Value& value = require(*this, name);
    if (const auto* v = std::get_if<bool>(&value)) {
        return *v;
    }
    wrong_type(name, "a boolean");
}

const std::string& Params::get_string(std::string_view name) const {
    const Value& value = require(*this, name);
    if (const auto* v = std::get_if<std::string>(&value)) {
        return *v;
    }
    wrong_type(name, "a string");
}

const std::map<std::string, Value, std::less<>>& Params::values() const noexcept { return values_; }

}
