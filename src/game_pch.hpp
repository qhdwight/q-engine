#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numbers>
#include <numeric>
#include <optional>
#include <print>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#define GAME_ASSERT(x) assert(x)

template<typename... Args>
void log(std::format_string<Args...> const& format, Args&&... args) {
    std::println(std::cout, format, std::forward<Args>(args)...);
}

#include <edyn/edyn.hpp>
#include <edyn/math/vector2_3_util.hpp>
#include <entt/entt.hpp>

#define VULKAN_HPP_NO_STRUCT_SETTERS
#define VULKAN_HPP_NO_SMART_HANDLE
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR
#include <vulkan/vulkan_raii.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

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
