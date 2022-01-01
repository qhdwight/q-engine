#pragma once

#include "state.hpp"

struct Window {
    bool keepOpen, isFocused;
};

void render(World& world);
