#include <memory>
#include <iostream>
#include <PxConfig.h>
#include <PxPhysicsAPI.h>

template<typename T>
struct px_ptr : public std::shared_ptr<T> {
    explicit px_ptr(T *ptr) : std::shared_ptr<T>(ptr, [](T *ptr) { ptr->release(); }) {}
};

int main() {
    std::cout << "Hello, World!" << std::endl;
    physx::PxDefaultErrorCallback errorCallback;
    physx::PxDefaultAllocator allocCallback;
    px_ptr<physx::PxFoundation> foundation{
            PxCreateFoundation(PX_PHYSICS_VERSION, allocCallback, errorCallback)};
    if (!foundation) {
        return EXIT_FAILURE;
    }
//    PxTolerance
    px_ptr<physx::PxPhysics> physics{
            PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), true)};
    return EXIT_SUCCESS;
}
