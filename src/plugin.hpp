#pragma once

#include "state.hpp"

class Plugin {
    virtual void build(ExecuteContext& ctx) = 0;

    virtual void execute(ExecuteContext& ctx) = 0;
};
