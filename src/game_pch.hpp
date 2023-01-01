#pragma once

#include <map>
#include <array>
#include <memory>
#include <string>
#include <limits>
#include <chrono>
#include <format>
#include <ranges>
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
#include <edyn/math/vector2_3_util.hpp>

#define VULKAN_HPP_NO_STRUCT_SETTERS
#define VULKAN_HPP_NO_SMART_HANDLE
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR

#include <vulkan/vulkan_raii.hpp>

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

enum struct OS {
    Windows,
    Linux,
    macOS,
};

constexpr auto SCALAR_EPSILON = std::numeric_limits<scalar>::epsilon();

#ifndef NDEBUG
constexpr bool IS_DEBUG = true;
#else
constexpr bool IS_DEBUG = false;
#endif
#if defined(_WIN32) || defined(__CYGWIN__)
constexpr OS OS = OS::Windows;
#elif defined(__linux__)
constexpr OS OS = OS::Linux;
#elif defined(__APPLE__) && defined(__MACH__)
constexpr OS OS = OS::macOS;
#else
#error Unknown environment!
#endif


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
