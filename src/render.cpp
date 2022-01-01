#include "render.hpp"

#include "vulkan_render.hpp"

void render(World& world) {
    tryRenderVulkan(world);
}
