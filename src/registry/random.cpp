#include <tlor/random.hpp>

#include <tlor/error.hpp>

namespace tlor {

Random::Random(std::uint64_t seed) : engine_(seed) {}

std::uint64_t Random::next() { return engine_(); }

std::uint64_t Random::below(std::uint64_t n) {
    if (n == 0) {
        throw Error(Status::invalid_argument, "Random::below needs a positive bound");
    }
    const std::uint64_t threshold = (0 - n) % n;
    while (true) {
        const std::uint64_t x = next();
        if (x >= threshold) {
            return x % n;
        }
    }
}

}
