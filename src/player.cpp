#include "player.hpp"

#include "math.hpp"

void modify(ExecuteContext& ctx) {
    for (auto [ent, input, look]: ctx.world.view<const Input, Look>().each()) {
        look += vec3{-input.cursorDelta.y, 0.0, input.cursorDelta.x};
        look.x = std::clamp(look.x, edyn::to_radians(-90.0), edyn::to_radians(90.0));
        look.z = std::fmod(look.z + edyn::to_radians(180.0), edyn::to_radians(360.0)) - edyn::to_radians(180.0);
        look.y = input.lean * edyn::to_radians(15.0);
    }
    for (auto [ent, input, pos, look, ts]: ctx.world.view<const Input, Position, const Look, Timestamp>().each()) {
        auto mult = 10.0 * static_cast<double>(ts.deltaNs) / 1e9;
        pos += edyn::rotate(fromEuler(look), vec3{input.move.x, input.move.y, 0.0}) * mult;
        pos += vec3{0.0, 0.0, input.move.z} * mult;
    }
}
