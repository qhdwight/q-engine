#pragma once

#include "game_pch.hpp"

#include "app.hpp"
#include "plugin.hpp"

class PlayerControllerPlugin : public Plugin {
public:
    void execute(App& app) override;
};

