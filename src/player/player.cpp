#include "player.hpp"

#include "math.hpp"
#include "state.hpp"

constexpr scalar Tau = std::numbers::pi * 2.0;
constexpr scalar XLookCap = Tau / 4.0;

void friction(scalar lateralSpeed, GroundedPlayerMove const& move, LinearVelocity const& linVel, Timestamp const& ts) {
    scalar control = std::max(lateralSpeed, move.stopSpeed);
    scalar drop = control * move.friction * sec_t(ts.delta).count();
//    scalar newSpeed
}

void PlayerControllerPlugin::execute(App& app) {
    for (auto [ent, input, look]: app.logicWorld.view<const Input, Look>().each()) {
        look += vec3{-input.cursorDelta.y, 0.0, input.cursorDelta.x};
        look.x = std::clamp(look.x, -XLookCap, XLookCap);
        look.z = std::fmod(look.z + Tau / 2.0, Tau) - Tau / 2.0;
        look.y = input.lean * edyn::to_radians(15.0);
    }

    auto flyView = app.logicWorld.view<
            const Input, const Look, const Timestamp,
            const FlyPlayerMove,
            LinearVelocity
    >();
    for (auto [ent, input, look, ts, move, linVel]: flyView.each()) {
        vec3 moveInLook = edyn::rotate(fromEuler(look), {input.move.x, input.move.y, 0.0});
        linVel = moveInLook;
        linVel.z = input.move.z;
        linVel *= move.speed;
        edyn::refresh<LinearVelocity>(app.logicWorld, ent);
    }

    auto groundedView = app.logicWorld.group<
            const Input, const Look, const Timestamp,
            const GroundedPlayerMove,
            Position, LinearVelocity
    >();
    for (auto [ent, input, look, ts, move, pos, linVel]: groundedView.each()) {
        vec3 initVel = linVel;
        vec3 endVel = initVel;
        scalar lateralSpeed = length(vec2{initVel.x, initVel.y});

//        auto result = edyn::raycast(app.logicWorld, );
        quat rot = fromEuler(look);
        vec3 forward = edyn::rotate(rot, {input.move.x, input.move.y, 0.0});
        vec3 right = edyn::rotate(rot, {input.move.x, input.move.y, 0.0});
        vec3 wishDir = input.move.z * move.fwdSpeed * forward + input.move.x * move.sideSpeed * right;
        scalar wishSpeed = length(wishDir);
        if (wishSpeed > std::numeric_limits<scalar>::epsilon())
            wishDir / wishSpeed;

        bool isGrounded = false;

        wishSpeed = std::min(wishSpeed, move.runSpeed);
        if (isGrounded) {
            if (move.groundTick >= 1) {
                if (lateralSpeed > move.frictionCutoff) {

                } else {
                    endVel.x = endVel.z = 0.0;
                }
                endVel.y = 0.0;
            }
        } else {

        }
    }
}
