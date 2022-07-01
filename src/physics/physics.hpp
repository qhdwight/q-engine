#pragma once

#include "plugin.hpp"

class PhysicsPlugin : public Plugin {
public:
    void build(App& app) override;

    void execute(App& app) override;

    void cleanup(App& app) override;
};
