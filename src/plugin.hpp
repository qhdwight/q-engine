#pragma once

#include "game_pch.hpp"

struct App;

class Plugin {
public:
    virtual void build(App& app);

    virtual void cleanup(App& app);

    virtual void execute(App& app);

    virtual ~Plugin() = default;
};
