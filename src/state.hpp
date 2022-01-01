#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/detail/type_vec3.hpp>

#include <string>
#include <unordered_map>

typedef glm::dvec3 position;
typedef glm::dquat orientation;

struct timestamp {
    long long ns;
};

struct Player {

};

struct Cube {

};

struct Input {
    glm::dvec2 mouse;
    std::unordered_map<std::string, bool> buttons;
};

struct InputConfig {

};

struct World {
    entt::registry reg;
    entt::registry::entity_type sharedEnt;
};
