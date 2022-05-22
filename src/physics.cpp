#include "physics.hpp"

#include <iostream>

void PhysicsPlugin::build(App& app) {
    edyn::init();
    edyn::attach(app.logicWorld);

    std::cout << "[Edyn]" << " physics attached" << std::endl;

    auto floor_def = edyn::rigidbody_def();
    floor_def.kind = edyn::rigidbody_kind::rb_static;
//    floor_def.material->restitution = 1;
    floor_def.material->friction = 0.5;
    // Plane at the origin with normal pointing up.
    floor_def.shape = edyn::plane_shape{{0, 0, 1}, 0};
    edyn::make_rigidbody(app.logicWorld, floor_def);
}

void PhysicsPlugin::execute(App& app) {
    edyn::update(app.logicWorld);
}

void PhysicsPlugin::cleanup(App& app) {
    edyn::detach(app.logicWorld);
    edyn::deinit();
}
