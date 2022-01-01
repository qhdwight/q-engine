#include "render.hpp"

#include "vulkan_render.hpp"

void render(world& world) {
    tryRenderVulkan(world);
}
