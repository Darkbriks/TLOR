#include <tlor/state.hpp>

namespace tlor {

State State::initial(std::int32_t node_count) {
    const auto n = static_cast<std::size_t>(node_count < 0 ? 0 : node_count);
    return State{std::vector<double>(n, 0.0), std::vector<Day>(n, -1)};
}

}
