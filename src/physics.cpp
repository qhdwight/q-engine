#include "physics.hpp"

#include <iostream>

#include <edyn/edyn.hpp>

void PhysicsPlugin::build(App& app) {
    edyn::init();
    std::cout << "[Edyn]" << " 1.1.0 initialized" << std::endl;
    edyn::attach(app.logicWorld);
    std::cout << "[Edyn]" << " Attached" << std::endl;

    auto floorDef = edyn::rigidbody_def();
    floorDef.kind = edyn::rigidbody_kind::rb_static;
//    floorDef.material->restitution = 1;
    floorDef.material->friction = 0.5;
    // Plane at the origin with normal pointing up.
    floorDef.shape = edyn::plane_shape{{0, 0, 1}, 0};
    edyn::make_rigidbody(app.logicWorld, floorDef);
}

void PhysicsPlugin::execute(App& app) {
    edyn::update(app.logicWorld);
}

void PhysicsPlugin::cleanup(App& app) {
    edyn::detach(app.logicWorld);
    edyn::deinit();
}
