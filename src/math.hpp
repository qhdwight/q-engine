#pragma once

#include <limits>

#include <edyn/math/math.hpp>
#include <edyn/math/vector3.hpp>
#include <edyn/math/quaternion.hpp>

template<typename T>
requires std::integral<T>
T saturating_add(T a, T b) {
    T c = a + b;
    return c < a ? std::numeric_limits<T>::max() : c;
}

static edyn::quaternion fromEuler(vec3 const& vec) {
    vec3 right{1.0, 0.0, 0.0}, fwd{0.0, 1.0, 0.0}, up{0.0, 0.0, 1.0};
    return edyn::quaternion_axis_angle(fwd, -vec.y)
           * edyn::quaternion_axis_angle(up, -vec.z)
           * edyn::quaternion_axis_angle(right, vec.x);
}