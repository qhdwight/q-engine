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

    std::array<btBroadphaseProxy*, 128> proxies{};
    size_t proxyIndex = 0;
    for (auto [ent, pos, orien, box]: world->view<Position, Orientation, BoxCollider>().each()) {
        btTransform btTf = toBt(pos, orien);
        btVector3 minAabb, maxAabb;
        box.getAabb(btTf, minAabb, maxAabb);
        btBroadphaseProxy* proxy = proxies[proxyIndex++] = bullet.broadphase->createProxy(minAabb, maxAabb, 0, &ent, 0, 0, nullptr);
        bullet.broadphase->setAabb(proxy, minAabb, maxAabb, nullptr);
    }
    bullet.broadphase->calculateOverlappingPairs(nullptr);

    for (size_t index = 0; index < proxyIndex; ++index) {
        bullet.broadphase->destroyProxy(proxies[index], nullptr);
    }
}