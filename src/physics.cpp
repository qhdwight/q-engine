#include "physics.hpp"

#include <edyn/edyn.hpp>

void PhysicsPlugin::build(ExecuteContext& ctx) {
    edyn::init();
    edyn::attach(ctx.world);

    auto floor_def = edyn::rigidbody_def();
    floor_def.kind = edyn::rigidbody_kind::rb_static;
    floor_def.material->restitution = 1;
    floor_def.material->friction = 0.5;
    // Plane at the origin with normal pointing up.
    floor_def.shape = edyn::plane_shape{{0, 1, 0}, 0};
    edyn::make_rigidbody(ctx.world, floor_def);
}

void PhysicsPlugin::execute(ExecuteContext& ctx) {
//    edyn::update(ctx.world);
}
