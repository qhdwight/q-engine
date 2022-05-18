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
}