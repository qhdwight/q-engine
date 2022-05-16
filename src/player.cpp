#include "player.hpp"

#include "math.hpp"

void modify(World& world) {
    for (auto [ent, input, look]: world->view<const Input, Look>().each()) {
        look.vec += glm::dvec3{-input.cursorDelta.y, 0.0, input.cursorDelta.x};
        look->x = std::clamp(look->x, glm::radians(-90.0), glm::radians(90.0));
        look->z = glm::mod(look->z + glm::radians(180.0), glm::radians(360.0)) - glm::radians(180.0);
        look->y = input.lean * glm::radians(15.0);
    }
    for (auto [ent, input, pos, look]: world->view<const Input, Position, const Look>().each()) {
        auto mult = 10.0 * static_cast<double>(world->get<Timestamp>(world.sharedEnt).deltaNs) / 1e9;
        pos.vec += fromEuler(look.vec) * glm::dvec3{input.move.x, input.move.y, 0.0} * mult;
        pos.vec += glm::dvec3{0.0, 0.0, input.move.z} * mult;
    }
}
