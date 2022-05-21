#pragma once

#include <edyn/edyn.hpp>

#include "state.hpp"
#include "plugin.hpp"

class PhysicsPlugin : Plugin {
public:
    void build(SystemContext const& ctx) override;

    void execute(SystemContext const& ctx) override;

    void cleanup(SystemContext const& ctx) override;
};
