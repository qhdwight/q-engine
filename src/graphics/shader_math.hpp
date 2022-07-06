#pragma once

#include "game_pch.hpp"

#include "math.hpp"

// Vulkan clip space has inverted y and half z
static constexpr mat4 ClipMat = {
        vec4{1.0, 0.0, 0.0, 0.0},
        vec4{0.0, -1.0, 0.0, 0.0},
        vec4{0.0, 0.0, 0.5, 0.0},
        vec4{0.0, 0.0, 0.5, 1.0},
};

/** @brief We do all of our calculations in doubles, but current GPUs work best with float */
static constexpr mat4f toShader(mat4 const& m) {
    return {
            std::array<float, 4>{static_cast<float>(m[0][0]), static_cast<float>(m[0][1]), static_cast<float>(m[0][2]), static_cast<float>(m[0][3])},
            std::array<float, 4>{static_cast<float>(m[1][0]), static_cast<float>(m[1][1]), static_cast<float>(m[1][2]), static_cast<float>(m[1][3])},
            std::array<float, 4>{static_cast<float>(m[2][0]), static_cast<float>(m[2][1]), static_cast<float>(m[2][2]), static_cast<float>(m[2][3])},
            std::array<float, 4>{static_cast<float>(m[3][0]), static_cast<float>(m[3][1]), static_cast<float>(m[3][2]), static_cast<float>(m[3][3])},
    };
}

static constexpr vec3f toShader(vec3 const& v) {
    return {static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)};
}

static mat4 calcView(Position const& eye, Look const& look) {
    // Calculations from GLM
    vec3 center = eye + rotate(fromEuler(look), edyn::vector3_y);
    vec3 up = rotate(fromEuler(look), edyn::vector3_z);

    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    // Create a basis with s, u, and f (camera space)
    mat4 view{};
    view[0][0] = +s.x;
    view[1][0] = +s.y;
    view[2][0] = +s.z;
    view[0][1] = +u.x;
    view[1][1] = +u.y;
    view[2][1] = +u.z;
    view[0][2] = -f.x;
    view[1][2] = -f.y;
    view[2][2] = -f.z;
    view[3][0] = -dot(s, eye);
    view[3][1] = -dot(u, eye);
    view[3][2] = +dot(f, eye);
    view[3][3] = 1.0;
    return view;
}

/**
 * @param extent    Screen dimensions in pixels
 * @return          Matrix which makes objects further away appear smaller
 */
static mat4 calcProj(vk::Extent2D const& extent) {
    // Calculations from GLM
    constexpr double rad = edyn::to_radians(45.0);
    double h = std::cos(0.5 * rad) / std::sin(0.5 * rad);
    double w = h * static_cast<double>(extent.height) / static_cast<double>(extent.width);
    constexpr double zNear = 0.1, zFar = 100.0;
    mat4 proj{};
    proj[0][0] = w;
    proj[1][1] = h;
    proj[2][2] = zFar / (zNear - zFar);
    proj[2][3] = -1.0;
    proj[3][2] = -(zFar * zNear) / (zFar - zNear);
    return proj;
}

static mat4 translate(mat4 m, vec3 v) {
    m[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
    return m;
}

static mat4 calcModel(Position const& pos) {
    mat4 model = matrix4x4_identity;
    return translate(model, pos);
}
