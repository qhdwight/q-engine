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

using vec2f = std::array<float, 2>;
using vec3f = std::array<float, 3>;
using vec4f = std::array<float, 4>;

using namespace entt::literals;
