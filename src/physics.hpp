#pragma once

#include "state.hpp"
#include "plugin.hpp"

class PhysicsPlugin : Plugin {
public:
    void build(ExecuteContext& ctx) override;

    void execute(ExecuteContext& ctx) override;
};
