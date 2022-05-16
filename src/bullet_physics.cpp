#include <iostream>

#include "bullet_physics.hpp"

void init(BulletResource& bullet) {
    bullet.collisionConfig = std::make_shared<btDefaultCollisionConfiguration>();
    // Use the default collision dispatcher. For parallel processing you can use a different dispatcher (see Extras/BulletMultiThreaded)
    bullet.dispatcher = std::make_shared<btCollisionDispatcher>(bullet.collisionConfig.get());
    // btDbvtBroadphase is a good general purpose broad-phase. You can also try out btAxis3Sweep.
    bullet.broadphase = std::make_shared<btDbvtBroadphase>();
    // The default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
    bullet.solver = std::make_shared<btSequentialImpulseConstraintSolver>();
    bullet.dynamicsWorld = std::make_shared<btDiscreteDynamicsWorld>(bullet.dispatcher.get(), bullet.broadphase.get(),
                                                                     bullet.solver.get(), bullet.collisionConfig.get());
    bullet.dynamicsWorld->setGravity(btVector3{0, -9.8, 0});

    std::cout << "[Bullet] " << btGetVersion() << " initialized" << std::endl;
}

void bulletPhysics(World& world) {
    auto pBullet = world->try_get<BulletResource>(world.sharedEnt);
    if (!pBullet) return;

    BulletResource& bullet = *pBullet;
    if (!bullet.dynamicsWorld) {
        init(bullet);
    }


}
