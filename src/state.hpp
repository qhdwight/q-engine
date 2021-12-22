#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/detail/type_vec3.hpp>

typedef glm::dvec3 position;

typedef glm::dquat rotation;

struct timestamp {
    long long ns;
};

struct world {
    entt::registry reg;
    entt::registry::entity_type worldEnt;
};
