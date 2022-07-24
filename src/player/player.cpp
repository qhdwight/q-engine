#include "player.hpp"

#include "math.hpp"
#include "state.hpp"

constexpr scalar Tau = std::numbers::pi * 2.0; // Tau makes more sense than using Pi!
constexpr scalar XLookCap = Tau / 4.0;

void friction(scalar friction, scalar stopSpeed, scalar lateralSpeed, LinearVelocity& linVel, scalar dt) {
    scalar control = std::max(lateralSpeed, stopSpeed);
    scalar drop = control * friction * dt;
    scalar newSpeed = std::max((lateralSpeed - drop) / lateralSpeed, 0.0);
    linVel.x *= newSpeed;
    linVel.y *= newSpeed;
}

void accelerate(scalar accel, vec3 wishDir, scalar wishSpeed, LinearVelocity& linVel, scalar dt) {
    scalar velProj = dot(linVel, wishDir);
    scalar addSpeed = wishSpeed - velProj;
    if (addSpeed <= std::numeric_limits<scalar>::epsilon()) return;

    scalar accelSpeed = std::min(accel * wishSpeed * dt, addSpeed);
    wishDir *= accelSpeed;
    linVel.x += wishDir.x;
    linVel.y += wishDir.z;
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
            GroundedPlayerMove,
            Position, LinearVelocity
    >();
    for (auto [ent, input, look, ts, move, pos, linVel]: groundedView.each()) {
        scalar dt = sec_t(ts.delta).count();
        vec3 initVel = move.linVel;
        vec3 endVel = initVel;
        scalar lateralSpeed = length(to_vector2_xz(initVel));

        edyn::raycast_result result = edyn::raycast(app.logicWorld, pos, pos);
        quat rot = fromEuler(look);
        vec3 forward = edyn::rotate(rot, {input.move.x, input.move.y, 0.0});
        vec3 right = edyn::rotate(rot, {input.move.x, input.move.y, 0.0});
        vec3 wishDir = input.move.z * move.fwdSpeed * forward + input.move.x * move.sideSpeed * right;
        scalar wishSpeed = length(wishDir);
        if (wishSpeed > std::numeric_limits<scalar>::epsilon())
            wishDir / wishSpeed;

        bool isGrounded = result.entity != entt::null;

        wishSpeed = std::min(wishSpeed, move.runSpeed);
        if (isGrounded) {
            if (move.groundTick >= 1) {
                if (lateralSpeed > move.frictionCutoff) {
                    friction(move.friction, move.stopSpeed, lateralSpeed, linVel, dt);
                } else {
                    endVel.x = endVel.z = 0.0;
                }
                endVel.y = 0.0;
            }
            accelerate(move.accel, wishDir, wishSpeed, linVel, dt);
            if (input.jump.current) {
                initVel.z = move.jumpSpeed;
                endVel.z = initVel.z - move.gravity * dt;
            }
            move.groundTick = saturating_increment(move.groundTick);

        } else {
            move.groundTick = 0;
            wishSpeed = std::min(wishSpeed, move.airSpeedCap);
            accelerate(move.airAccel, wishDir, wishSpeed, linVel, dt);
            endVel.z -= move.gravity * dt;
            scalar airSpeed = length(to_vector2_xz(endVel));
            if (airSpeed > move.maxAirSpeed) {
                scalar ratio = move.maxAirSpeed / airSpeed;
                endVel.x *= ratio;
                endVel.y *= ratio;
            }
        }

        move.linVel = endVel;
        linVel = (initVel + endVel) / 2.0;
    }
}
