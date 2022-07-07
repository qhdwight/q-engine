#include "player.hpp"

#include "math.hpp"
#include "state.hpp"

constexpr scalar Tau = std::numbers::pi * 2.0;
constexpr scalar XLookCap = Tau / 4.0;

void PlayerControllerPlugin::execute(App& app) {
    for (auto [ent, input, look]: app.logicWorld.view<const Input, Look>().each()) {
        look += vec3{-input.cursorDelta.y, 0.0, input.cursorDelta.x};
        look.x = std::clamp(look.x, -XLookCap, XLookCap);
        look.z = std::fmod(look.z + Tau / 2.0, Tau) - Tau / 2.0;
        look.y = input.lean * edyn::to_radians(15.0);
    }

    auto flyView = app.logicWorld.view<const FlyPlayerMove, Input, LinearVelocity, const Look, const Timestamp>();
    for (auto [ent, move, input, linVel, look, ts]: flyView.each()) {
        vec3 moveInLook = edyn::rotate(fromEuler(look), {input.move.x, input.move.y, 0.0});
        linVel = moveInLook;
        linVel.z = input.move.z;
        linVel *= move.speed;
        edyn::refresh<LinearVelocity>(app.logicWorld, ent);
    }

    auto groundedView = app.logicWorld.group<const GroundedPlayerMove, Input, Position, LinearVelocity, const Look, const Timestamp>();
    for (auto [ent, move, input, pos, linVel, look, ts]: groundedView.each()) {
        ns_t dt = ts.deltaNs;

//        auto result = edyn::raycast(app.logicWorld, );
        quat rot = fromEuler(look);
        vec3 forward = edyn::rotate(rot, {input.move.x, input.move.y, 0.0});
        vec3 right = edyn::rotate(rot, {input.move.x, input.move.y, 0.0});
        vec3 wishDir = input.move.z * move.fwdSpeed * forward + input.move.x * move.sideSpeed * right;
    }
}
