#pragma once

#include <edyn/edyn.hpp>

#include "state.hpp"
#include "plugin.hpp"

class PhysicsPlugin : Plugin {
public:
    void build(ExecuteContext& ctx) override;

    void execute(ExecuteContext const& ctx) override;
};
