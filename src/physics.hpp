#pragma once

#include <edyn/edyn.hpp>

#include "state.hpp"
#include "plugin.hpp"

class PhysicsPlugin : Plugin {
public:
    void build(App& app) override;

    void execute(App& app) override;

    void cleanup(App& app) override;
};
