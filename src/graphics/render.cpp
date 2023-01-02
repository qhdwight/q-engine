#include "render.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "app.hpp"
//#include "shaders.hpp"
#include "shader_math.hpp"

constexpr uint64_t FENCE_TIMEOUT = 100000000;

void VulkanRenderPlugin::build(App& app) {
    app.globalCtx.emplace<VulkanContext>();
    app.globalCtx.emplace<WindowContext>(false, true, false);
}

void VulkanRenderPlugin::execute(App& app) {
    auto pVk = app.globalCtx.find<VulkanContext>();
    if (!pVk) return;

    VulkanContext& vk = *pVk;
    if (!*vk.inst) init(vk);

    GAME_ASSERT(*vk.device);
    GAME_ASSERT(*vk.draw_fence);
    GAME_ASSERT(*vk.render_pass);
    GAME_ASSERT(*vk.swapchain.swapchain);
    GAME_ASSERT(*vk.image_acquire_semaphore);
    GAME_ASSERT(!vk.command_buffers.empty());

    // Acquire next image and signal the semaphore
    auto [acquire_result, current_buffer_index] = vk.swapchain.swapchain.acquireNextImage(FENCE_TIMEOUT, *vk.image_acquire_semaphore, nullptr);

    if (acquire_result == vk::Result::eSuboptimalKHR) {
        recreate_pipeline(vk);
        return;
    }
    if (acquire_result != vk::Result::eSuccess) {
        throw std::runtime_error("Invalid acquire next image KHR result");
    }
    if (vk.framebuffers.size() <= current_buffer_index) {
        throw std::runtime_error("Invalid framebuffer size");
    }
//
//    for (auto [ent, shaderHandle]: app.renderWorld.view<const ShaderHandle>().each()) {
//        if (vk.modelPipelines.contains(shaderHandle.value)) continue;
//
//        // Create blank shader and fill it in
//        createShaderPipeline(vk, vk.modelPipelines[shaderHandle.value]);
//    }
//
    vk.command_buffers.front().begin({});
    vk::ClearValue clear_color = vk::ClearColorValue(std::array<float, 4>{0.2f, 0.2f, 0.2f, 0.2f});
    vk::ClearValue clear_depth = vk::ClearDepthStencilValue(1.0f, 0);
    std::array<vk::ClearValue, 2> clear_values{clear_color, clear_depth};
    vk::RenderPassBeginInfo render_pass_begin_info{
            *vk.render_pass,
            *vk.framebuffers[current_buffer_index],
            vk::Rect2D{{}, vk.window.extent()},
            clear_values
    };
    vk.command_buffers.front().beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
    render_opaque(app);
    render_imgui(app);
    vk.command_buffers.front().endRenderPass();
    vk.command_buffers.front().end();
//
    vk::PipelineStageFlags waitDestStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    // Fences need to be manually reset
    vk.device.resetFences(*vk.draw_fence);
    // Wait for the image to be acquired via the semaphore, signal the drawing fence when submitted
    vk.graphics_queue.submit(vk::SubmitInfo{*vk.image_acquire_semaphore, waitDestStageMask, *vk.command_buffers.front()}, *vk.draw_fence);

    // Wait for the draw fence to be signaled
    while (vk::Result::eTimeout == vk.device.waitForFences(*vk.draw_fence, VK_TRUE, FENCE_TIMEOUT));

    try {
        // Present frame to display
        vk::Result result = vk.present_queue.presentKHR({nullptr, *vk.swapchain.swapchain, current_buffer_index});
        switch (result) {
            case vk::Result::eSuccess:
            case vk::Result::eSuboptimalKHR:
                break;
            default:
                throw std::runtime_error("Bad present KHR result: " + vk::to_string(result));
        }
    } catch (vk::OutOfDateKHRError const&) {
        recreate_pipeline(vk);
    }

    glfwPollEvents();
    bool& keep_open = app.globalCtx.at<WindowContext>().keepOpen;
    keep_open = !glfwWindowShouldClose(vk.window.window_handle.get());
    if (!keep_open) {
        vk.device.waitIdle();
    }
}

void VulkanRenderPlugin::cleanup(App& app) {
    ImGui_ImplVulkan_Shutdown();
//    glslang::FinalizeProcess();
//    for (auto& [_, pipeline]: app.globalCtx.at<VulkanContext>().modelPipelines) {
//        for (auto& item: pipeline.shaders) {
//            spvReflectDestroyShaderModule(&item.reflect);
//        }
//    }
}
