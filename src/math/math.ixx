export module math;

import std;

export template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<std::size_t... Dims>
struct product;

export template<Arithmetic T, std::size_t... Dims>
struct matrix : public std::array<T, (Dims * ...)> {
};

export template<Arithmetic T, std::size_t Dim>
using vec = matrix<T, Dim, 1>;

export using vec2d = vec<double, 2>;
export using vec3d = vec<double, 3>;
export using vec4d = vec<double, 4>;

export using vec2f = vec<float, 2>;
export using vec3f = vec<float, 3>;
export using vec4f = vec<float, 4>;

export using mat4f = matrix<float, 4, 4>;

export template<std::integral T>
T saturating_add(T a, T b) {
    T c = a + b;
    return c < a ? std::numeric_limits<T>::max() : c;
}

export template<std::integral T>
T saturating_increment(T a) {
    return saturating_add(a, T{1});
}
