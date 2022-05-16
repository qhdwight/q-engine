#include <iostream>

#include "bullet_physics.hpp"

void init(BulletResource& bullet) {
    bullet.broadphase = std::make_shared<btDbvtBroadphase>();
    bullet.solver = std::make_shared<btSequentialImpulseConstraintSolver>();
    std::cout << "[Bullet] " << btGetVersion() << " initialized" << std::endl;
}

btVector3 toBt(LinearVelocity& p) { return {p->x, p->y, p->z}; }

btVector3 toBt(Position& p) { return {p->x, p->y, p->z}; }

btQuaternion toBt(Orientation& o) { return {o->x, o->y, o->z, o->w}; }

btTransform toBt(Position& p, Orientation& o) { return btTransform{toBt(o), toBt(p)}; }

void bulletPhysics(World& world) {
    auto pBullet = world->try_get<BulletResource>(world.sharedEnt);
    if (!pBullet) return;

    BulletResource& bullet = *pBullet;
    if (!bullet.broadphase || !bullet.solver) {
        init(bullet);
    }

    for (auto [ent, pos, orien, linVel]: world->view<Position, Orientation, LinearVelocity>().each()) {
        btTransform btTf = toBt(pos, orien);
        btTransform btTfPred;
        btTransformUtil::integrateTransform(btTf, toBt(linVel), {}, 1.0f, btTfPred);
    }
    for (auto [ent, pos, orien, box, proxy]: world->view<Position, Orientation, btBoxShape, btBroadphaseProxy>().each()) {
        btTransform btTf = toBt(pos, orien);
        bullet.broadphase->createProxy()
        proxy.m_uniqueId = static_cast<int>(ent);
        box.getAabb(btTf, proxy.m_aabbMin, proxy.m_aabbMax);
        bullet.broadphase->setAabb(&proxy, proxy.m_aabbMin, proxy.m_aabbMax, nullptr);
    }
    bullet.broadphase->calculateOverlappingPairs(nullptr);
}
