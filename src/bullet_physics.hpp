#pragma once

#include <memory>

#include <btBulletDynamicsCommon.h>

#include "state.hpp"

struct BulletResource {
    std::shared_ptr<btBroadphaseInterface> broadphase;
    std::shared_ptr<btDefaultCollisionConfiguration> collisionConfig;
    std::shared_ptr<btCollisionDispatcher> dispatcher;
    std::shared_ptr<btSequentialImpulseConstraintSolver> solver;
    std::shared_ptr<btDiscreteDynamicsWorld> dynamicsWorld;
};

void bulletPhysics(World& world);
