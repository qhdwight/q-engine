#pragma once

struct vec4 {
    double x, y, z, w;

    constexpr scalar& operator[](size_t i) noexcept {
        return (&x)[i];
    }

    constexpr scalar const& operator[](size_t i) const noexcept {
        return (&x)[i];
    }
};

constexpr vec4 operator+(const vec4& v, const vec4& w) noexcept {
    return {v.x + w.x, v.y + w.y, v.z + w.z, v.w + w.w};
}

constexpr vec4 operator*(const vec4& v, scalar s) noexcept {
    return {v.x * s, v.y * s, v.z * s, v.w * s};
}

struct mat4 {
    std::array<vec4, 4> row;

    constexpr vec4& operator[](size_t i) noexcept {
        return row[i];
    }

    constexpr vec4 const& operator[](size_t i) const noexcept {
        return row[i];
    }

    constexpr vec4 column(size_t i) const noexcept {
        return {row[0][i], row[1][i], row[2][i], row[3][i]};
    }
};

constexpr mat4 transpose(mat4 const& m) noexcept {
    return {m.column(0), m.column(1), m.column(2), m.column(3)};
}

inline constexpr mat4 matrix4x4_identity{
        vec4{1.0, 0.0, 0.0, 0.0},
        vec4{0.0, 1.0, 0.0, 0.0},
        vec4{0.0, 0.0, 1.0, 0.0},
        vec4{0.0, 0.0, 0.0, 1.0}
};
