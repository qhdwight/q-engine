#pragma once

#include <memory>
#include <variant>

#define BT_USE_DOUBLE_PRECISION
#include <btBulletDynamicsCommon.h>

#include "state.hpp"

using BoxCollider = btBoxShape;

struct BulletResource {
    std::shared_ptr<btBroadphaseInterface> broadphase;
    std::shared_ptr<btSequentialImpulseConstraintSolver> solver;
};

void bulletPhysics(World& world);
