#pragma once

#include "game_pch.hpp"

struct App;

class Plugin {
public:
    virtual void build(App& app) = 0;

    virtual void cleanup([[maybe_unused]] App& app) {};

    virtual void execute(App& app) = 0;
};
