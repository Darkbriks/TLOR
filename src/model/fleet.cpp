#include <tlor/fleet.hpp>

#include <tlor/error.hpp>

#include <cmath>
#include <string>

namespace tlor {

Fleet::Fleet(std::vector<Truck> trucks, std::int32_t day_count, std::vector<std::uint8_t> available)
    : trucks_(std::move(trucks)), day_count_(day_count), available_(std::move(available)) {
    if (trucks_.empty()) {
        throw Error(Status::invalid_argument, "a fleet needs at least one truck");
    }
    if (day_count_ < 1) {
        throw Error(Status::invalid_argument, "a fleet needs at least one day");
    }
    for (const Truck& t : trucks_) {
        if (!std::isfinite(t.capacity_kg) || t.capacity_kg <= 0 || !std::isfinite(t.max_work_s) || t.max_work_s <= 0) {
            throw Error(Status::invalid_argument, "truck capacity and maximum work time must be positive");
        }
    }
    const std::size_t expected = static_cast<std::size_t>(day_count_) * trucks_.size();
    if (available_.empty()) {
        available_.assign(expected, 1);
    } else if (available_.size() != expected) {
        throw Error(Status::invalid_argument,
                    "availability has " + std::to_string(available_.size()) + " values, expected day_count * truck_count");
    }
}

std::int32_t Fleet::truck_count() const noexcept { return static_cast<std::int32_t>(trucks_.size()); }

std::int32_t Fleet::day_count() const noexcept { return day_count_; }

const Truck& Fleet::truck(TruckId truck) const {
    if (truck < 0 || truck >= truck_count()) {
        throw Error(Status::invalid_argument, "truck " + std::to_string(truck) + " is out of range");
    }
    return trucks_[static_cast<std::size_t>(truck)];
}

bool Fleet::available(Day day, TruckId truck) const noexcept {
    if (day < 0 || day >= day_count_ || truck < 0 || truck >= truck_count()) {
        return false;
    }
    return available_[static_cast<std::size_t>(day) * trucks_.size() + static_cast<std::size_t>(truck)] != 0;
}

std::int32_t Fleet::available_count(Day day) const noexcept {
    std::int32_t count = 0;
    for (TruckId t = 0; t < truck_count(); ++t) {
        count += available(day, t) ? 1 : 0;
    }
    return count;
}

std::vector<TruckId> Fleet::available_trucks(Day day) const {
    std::vector<TruckId> trucks;
    for (TruckId t = 0; t < truck_count(); ++t) {
        if (available(day, t)) {
            trucks.push_back(t);
        }
    }
    return trucks;
}

std::span<const std::uint8_t> Fleet::available_data() const noexcept { return available_; }

}
