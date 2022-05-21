#pragma once

#include "state.hpp"

class Plugin {
    virtual void build(SystemContext const& ctx) = 0;

    virtual void cleanup(SystemContext const& ctx) {};

    virtual void execute(SystemContext const& ctx) = 0;
};
