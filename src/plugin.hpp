#pragma once

#include "state.hpp"

class Plugin {
    virtual void build(App& app) = 0;

    virtual void cleanup([[maybe_unused]] App& app) {};

    virtual void execute(App& app) = 0;
};
