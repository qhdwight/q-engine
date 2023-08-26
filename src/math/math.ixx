export module math;

import common;

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

export using f64_2 = vec<f64, 2>;
export using f64_3 = vec<f64, 3>;
export using f64_4 = vec<f64, 4>;

export using f32_2 = vec<f32, 2>;
export using f32_3 = vec<f32, 3>;
export using f32_4 = vec<f32, 4>;

export using f32_4x4 = matrix<f32, 4, 4>;

export template<std::integral T>
T saturating_add(T a, T b) {
    T c = a + b;
    return c < a ? std::numeric_limits<T>::max() : c;
}

export template<std::integral T>
T saturating_increment(T a) {
    return saturating_add(a, T{1});
}
