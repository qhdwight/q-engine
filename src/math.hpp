#pragma once

#include <limits>
#include <concepts>

#include <edyn/math/vector3.hpp>
#include <edyn/math/quaternion.hpp>

template<typename T>
requires std::integral<T>
T saturating_add(T a, T b) {
    T c = a + b;
    return c < a ? std::numeric_limits<T>::max() : c;
}

static edyn::quaternion fromEuler(vec3 const& vec) {
    vec3 right = edyn::vector3_x, fwd = edyn::vector3_y, up = edyn::vector3_z;
    return edyn::quaternion_axis_angle(fwd, -vec.y)
           * edyn::quaternion_axis_angle(up, -vec.z)
           * edyn::quaternion_axis_angle(right, vec.x);
}
