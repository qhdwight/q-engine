#include "player.hpp"

#include <numbers>

#include <edyn/edyn.hpp>

#include "math.hpp"

constexpr scalar Tau = std::numbers::pi * 2.0;
constexpr scalar XLookCap = Tau / 4.0;

void modify(SystemContext const& ctx) {
    for (auto [ent, input, look]: ctx.world.view<const Input, Look>().each()) {
        look += vec3{-input.cursorDelta.y, 0.0, input.cursorDelta.x};
        look.x = std::clamp(look.x, -XLookCap, XLookCap);
        look.z = std::fmod(look.z + Tau / 2.0, Tau) - Tau / 2.0;
        look.y = input.lean * edyn::to_radians(15.0);
    }
    for (auto [ent, input, linVel, look, ts]: ctx.world.view<const Input, LinearVelocity, const Look, const Timestamp>().each()) {
        linVel = edyn::rotate(fromEuler(look), {input.move.x, input.move.y, 0.0});
        linVel.z = input.move.z;
        linVel *= 10;
        edyn::refresh<LinearVelocity>(ctx.world, ent);
    }
}
