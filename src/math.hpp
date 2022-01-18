#pragma once

#include <glm/gtc/quaternion.hpp>

static glm::dquat fromEuler(glm::dvec3 const& vec) {
    glm::dvec3 right{1.0, 0.0, 0.0}, fwd{0.0, 1.0, 0.0}, up{0.0, 0.0, 1.0};
    return glm::angleAxis(-vec.y, fwd) * glm::angleAxis(-vec.z, up) * glm::angleAxis(vec.x, right);
}