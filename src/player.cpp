#include "player.hpp"

void modify(World& world) {
    for (auto[ent, input, look]: world.reg.view<const Input, look>().each()) {
        look.vec += glm::dvec3{-input.cursorDelta.y, input.cursorDelta.x, 0.0};
        look.vec.x = std::clamp(look.vec.x, glm::radians(-90.0), glm::radians(90.0));
    }
    for (auto[ent, input, pos, look]: world.reg.view<const Input, position, const look>().each()) {
        auto mult = 10.0 * static_cast<double>(world.reg.get<timestamp>(world.sharedEnt).nsDelta) / 1e9;
        pos.vec += glm::dquat(look.vec) * input.move * mult;
    }
}
