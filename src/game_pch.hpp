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
