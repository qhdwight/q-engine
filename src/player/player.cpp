#include "player.hpp"

#include "math.hpp"
#include "state.hpp"

constexpr scalar Tau = std::numbers::pi * 2.0;
constexpr scalar XLookCap = Tau / 4.0;

void modify(App& app) {
    for (auto [ent, input, look]: app.logicWorld.view<const Input, Look>().each()) {
        look += vec3{-input.cursorDelta.y, 0.0, input.cursorDelta.x};
        look.x = std::clamp(look.x, -XLookCap, XLookCap);
        look.z = std::fmod(look.z + Tau / 2.0, Tau) - Tau / 2.0;
        look.y = input.lean * edyn::to_radians(15.0);
    }
    for (auto [ent, input, linVel, look, ts]: app.logicWorld.view<const Input, LinearVelocity, const Look, const Timestamp>().each()) {
        linVel = edyn::rotate(fromEuler(look), {input.move.x, input.move.y, 0.0});
        linVel.z = input.move.z;
        linVel *= 10;
        edyn::refresh<LinearVelocity>(app.logicWorld, ent);
    }
}
