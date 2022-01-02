#include "player.hpp"

#include <glm/gtx/string_cast.hpp>

#include <iostream>

void modify(World& world) {
    for (auto[ent, input, look]: world.reg.view<const Input, look>().each()) {
        look.vec += glm::dvec3{input.cursorDelta.y, 0.0, -input.cursorDelta.x};
        look.vec.x = std::clamp(look.vec.x, glm::radians(-90.0), glm::radians(90.0));
        look.vec.z = glm::mod(look.vec.z + glm::radians(180.0), glm::radians(360.0)) - glm::radians(180.0);
//        std::cout << glm::to_string(look.vec) << std::endl;
    }
    for (auto[ent, input, pos, look]: world.reg.view<const Input, position, const look>().each()) {
        auto mult = 10.0 * static_cast<double>(world.reg.get<timestamp>(world.sharedEnt).nsDelta) / 1e9;
        pos.vec += glm::dquat(look.vec) * glm::dvec3{input.move.x, input.move.y, 0.0} * mult;
        pos.vec += glm::dvec3{0.0, 0.0, input.move.z} * mult;
    }
}
