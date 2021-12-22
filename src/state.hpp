#pragma once

#include <entt/entt.hpp>

struct quaternion {
    double x, y, z, w;
};

struct position {
    double x, y, z;
};

struct timestamp {
    long long ns;
};

struct world {
    entt::registry reg;
    entt::registry::entity_type worldEnt;
};
