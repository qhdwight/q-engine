#pragma once

#include "state.hpp"

#include "vulkan_render.hpp"

using GraphicsResource = VulkanResource;

struct WindowResource {
    bool isReady, keepOpen, isFocused;
};

void render(ExecuteContext& ctx);
