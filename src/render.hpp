#pragma once

#include "state.hpp"

struct WindowResource {
    bool keepOpen, isFocused;
};

void render(World& world);
