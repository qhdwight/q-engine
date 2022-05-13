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

struct Key {
    bool previous: 1;
    bool current: 1;
};

struct Input {
    glm::dvec2 cursor, cursorDelta;
    glm::dvec3 move;
    double lean;
    Key menu;
};

struct UI {
    bool visible;
};

struct World {
    entt::registry reg;
    entt::registry::entity_type sharedEnt;

    entt::registry* operator->() noexcept {
        return &reg;
    }
};
