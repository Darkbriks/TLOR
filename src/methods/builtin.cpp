#include "methods/methods.hpp"

namespace tlor {

void register_builtin_methods(Registry& registry) {
    registry.add_schedule(threshold_info(), schedule_threshold);
}

}
