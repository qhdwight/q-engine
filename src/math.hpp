#pragma once

#include <limits>

#include <glm/gtc/quaternion.hpp>

template<typename T>
requires std::integral<T>
T saturating_add(T a, T b) {
    T c = a + b;
    return c < a ? std::numeric_limits<T>::max() : c;
}

static glm::dquat fromEuler(glm::dvec3 const& vec) {
    glm::dvec3 right{1.0, 0.0, 0.0}, fwd{0.0, 1.0, 0.0}, up{0.0, 0.0, 1.0};
    return glm::angleAxis(-vec.y, fwd) * glm::angleAxis(-vec.z, up) * glm::angleAxis(vec.x, right);
}