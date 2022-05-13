#include "player.hpp"

#include "math.hpp"

void modify(World& world) {
    for (auto [ent, input, look]: world->view<const Input, look>().each()) {
        look.vec += glm::dvec3{-input.cursorDelta.y, 0.0, input.cursorDelta.x};
        look.vec.x = std::clamp(look.vec.x, glm::radians(-90.0), glm::radians(90.0));
        look.vec.z = glm::mod(look.vec.z + glm::radians(180.0), glm::radians(360.0)) - glm::radians(180.0);
        look.vec.y = input.lean * glm::radians(15.0);
    }
    for (auto [ent, input, pos, look]: world->view<const Input, position, const look>().each()) {
        auto mult = 10.0 * static_cast<double>(world->get<timestamp>(world.sharedEnt).nsDelta) / 1e9;
        pos.vec += fromEuler(look.vec) * glm::dvec3{input.move.x, input.move.y, 0.0} * mult;
        pos.vec += glm::dvec3{0.0, 0.0, input.move.z} * mult;
    }
}
