#pragma once

#include "state.hpp"

struct Render {
    bool keepOpen;
};

void render(World& world);
