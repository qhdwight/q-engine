#pragma once

#include <map>
#include <array>
#include <memory>
#include <string>
#include <limits>
#include <chrono>
#include <numeric>
#include <numbers>
#include <fstream>
#include <iomanip>
#include <utility>
#include <concepts>
#include <iostream>
#include <optional>
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

#define GAME_ASSERT(x) assert(x)

#include <entt/entt.hpp>
#include <edyn/edyn.hpp>

using scalar = edyn::scalar;
using vec3 = edyn::vector3;
using vec2 = edyn::vector2;
using quat = edyn::quaternion;

using ns_t = std::chrono::nanoseconds;
using ms_t = std::chrono::duration<double, std::chrono::milliseconds::period>;
using sec_t = std::chrono::duration<double, std::chrono::seconds::period>;
using steady_clock_t = std::chrono::steady_clock;
using clock_point_t = std::chrono::time_point<steady_clock_t>;
using clock_delta_t = clock_point_t::duration;

constexpr auto SCALAR_EPSILON = std::numeric_limits<scalar>::epsilon();

// #REFLECT()
struct vec2f {
    float x, y;
};

// #REFLECT()
struct vec3f {
    float x, y, z;
};

// #REFLECT()
struct vec4f {
    float x, y, z, w;
};

using namespace entt::literals;
using namespace std::literals;
