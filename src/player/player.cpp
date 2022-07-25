#include "player.hpp"

#include "math.hpp"
#include "state.hpp"

constexpr scalar Tau = std::numbers::pi * 2.0; // Tau makes more sense than using Pi!
constexpr scalar XLookCap = Tau / 4.0;

void friction(scalar friction, scalar stopSpeed, scalar lateralSpeed, vec3& linVel, scalar dt) {
    scalar control = std::max(lateralSpeed, stopSpeed);
    scalar drop = control * friction * dt;
    scalar newSpeed = std::max((lateralSpeed - drop) / lateralSpeed, 0.0);
    linVel.x *= newSpeed;
    linVel.y *= newSpeed;
}

void accelerate(scalar accel, vec3 wishDir, scalar wishSpeed, vec3& linVel, scalar dt) {
    scalar velProj = dot(linVel, wishDir);
    scalar addSpeed = wishSpeed - velProj;
    if (addSpeed <= SCALAR_EPSILON) return;

    scalar accelSpeed = std::min(accel * wishSpeed * dt, addSpeed);
    wishDir *= accelSpeed;
    linVel.x += wishDir.x;
    linVel.y += wishDir.y;
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
        scalar lateralSpeed = length(to_vector2_xy(initVel));

        edyn::raycast_result result = edyn::raycast(
                app.logicWorld,
                pos,
                pos - vec3{0.0, 0.0, 1.0625},
                [ent](entt::entity hitEnt) { return hitEnt == ent; } // prevent self collisions
        );
        vec3 wishDir = edyn::rotate(fromEuler(look), {input.move.x * move.sideSpeed, input.move.y * move.fwdSpeed, 0.0});
        scalar wishSpeed = length(wishDir);
        if (wishSpeed > SCALAR_EPSILON)
            wishDir /= wishSpeed;

        bool isGrounded = result.entity != entt::null;

        wishSpeed = std::min(wishSpeed, move.runSpeed);

        if (auto* stats = app.logicWorld.try_get<MoveStats>(ent)) {
            stats->wishDir = wishDir;
            stats->wishSpeed = wishSpeed;
            stats->lateralSpeed = lateralSpeed;
        }

        if (isGrounded) {
            if (move.groundTick >= 1) {
                if (lateralSpeed > move.frictionCutoff) {
                    friction(move.friction, move.stopSpeed, lateralSpeed, endVel, dt);
                } else {
                    endVel.x = endVel.y = 0.0;
                }
                endVel.z = -0.0625;
            }
            accelerate(move.accel, wishDir, wishSpeed, endVel, dt);
            if (input.jump.current) {
                initVel.z = move.jumpSpeed;
                endVel.z = initVel.z - move.gravity * dt;
            }
            move.groundTick = saturating_increment(move.groundTick);

        } else {
            move.groundTick = 0;
            wishSpeed = std::min(wishSpeed, move.airSpeedCap);
            accelerate(move.airAccel, wishDir, wishSpeed, endVel, dt);
            endVel.z -= move.gravity * dt;
            scalar airSpeed = length(to_vector2_xy(endVel));
            if (airSpeed > move.maxAirSpeed) {
                scalar ratio = move.maxAirSpeed / airSpeed;
                endVel.x *= ratio;
                endVel.y *= ratio;
            }
        }

        move.linVel = endVel;
        linVel = (initVel + endVel) * 0.5;
        edyn::refresh<LinearVelocity>(app.logicWorld, ent);
    }
}
