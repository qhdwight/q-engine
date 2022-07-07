#pragma once

#include "game_pch.hpp"

#include "app.hpp"
#include "plugin.hpp"

struct InputSettings {
    float sensitivity;
};

class InputPlugin : public Plugin {
public:
    void execute(App& app) override;
};
