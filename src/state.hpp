#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/detail/type_vec3.hpp>

#include <string>
#include <unordered_map>

struct position {
    glm::dvec3 vec;
};
struct orientation {
    glm::dquat quat;
};

struct look {
    glm::dvec3 vec;
};

struct timestamp {
    long long ns, nsDelta;
};

struct Player {

};

struct Cube {

};

struct Input {
    glm::dvec2 cursor, cursorDelta;
    glm::dvec3 move;
    double lean;
};

struct InputConfig {

};

struct World {
    entt::registry reg;
    entt::registry::entity_type sharedEnt;
};
