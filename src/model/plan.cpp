#include <tlor/plan.hpp>

#include <tlor/error.hpp>

#include <algorithm>
#include <string>

namespace tlor {

Plan::Plan(Day start, std::int32_t horizon) : start_(start) {
    if (start < 0 || horizon < 1) {
        throw Error(Status::invalid_argument, "a plan needs start >= 0 and horizon >= 1");
    }
    days_.resize(static_cast<std::size_t>(horizon));
}

Day Plan::start() const noexcept { return start_; }

std::int32_t Plan::horizon() const noexcept { return static_cast<std::int32_t>(days_.size()); }

Day Plan::end() const noexcept { return start_ + horizon(); }

bool Plan::contains(Day day) const noexcept { return day >= start_ && day < end(); }

std::size_t Plan::offset(Day day) const {
    if (!contains(day)) {
        throw Error(Status::invalid_argument, "day " + std::to_string(day) + " is outside the plan");
    }
    return static_cast<std::size_t>(day - start_);
}

void Plan::add(Day day, Node point) {
    auto& points = days_[offset(day)];
    if (point < 0) {
        throw Error(Status::invalid_argument, "point " + std::to_string(point) + " is negative");
    }
    auto it = std::ranges::lower_bound(points, point);
    if (it == points.end() || *it != point) {
        points.insert(it, point);
    }
}

void Plan::remove(Day day, Node point) {
    auto& points = days_[offset(day)];
    auto it = std::ranges::lower_bound(points, point);
    if (it != points.end() && *it == point) {
        points.erase(it);
    }
}

bool Plan::collected(Day day, Node point) const {
    if (!contains(day)) {
        return false;
    }
    return std::ranges::binary_search(days_[offset(day)], point);
}

std::span<const Node> Plan::points(Day day) const { return days_[offset(day)]; }

std::int32_t Plan::collection_count() const noexcept {
    std::size_t count = 0;
    for (const auto& points : days_) {
        count += points.size();
    }
    return static_cast<std::int32_t>(count);
}

}
