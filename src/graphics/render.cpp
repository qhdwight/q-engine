#include "render.hpp"

#include "app.hpp"
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
    GAME_ASSERT(*vk.swapchain.swapchain);
    GAME_ASSERT(*vk.renderComplete);
    GAME_ASSERT(*vk.presentComplete);
    GAME_ASSERT(!vk.commandBuffers.empty());

    try {
        auto [acquireResult, currentBufferIndex] = vk.swapchain.swapchain.acquireNextImage(FENCE_TIMEOUT, *vk.presentComplete, nullptr);

        //
        //    for (auto [ent, shaderHandle]: app.renderWorld.view<const ShaderHandle>().each()) {
        //        if (vk.modelPipelines.contains(shaderHandle.value)) continue;
        //
        //        // Create blank shader and fill it in
        //        createShaderPipeline(vk, vk.modelPipelines[shaderHandle.value]);
        //    }
        //

        vk::raii::CommandBuffer& cmdBuffer = vk.commandBuffers.front();
        cmdBuffer.begin({});
        {
            vk::RenderingAttachmentInfo colorAttachmentInfo{
                    *vk.swapchain.views[currentBufferIndex],
                    vk::ImageLayout::eUndefined,
                    vk::ResolveModeFlagBits::eNone,
                    {},
                    vk::ImageLayout::eAttachmentOptimalKHR,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ClearColorValue{0.2f, 0.2f, 0.2f, 0.2f},
            };
            //        vk::RenderingAttachmentInfo depthAttachmentInfo{
            //                *vk.swapchain.views[currentBufferIndex],
            //                vk::ImageLayout::eUndefined,
            //                vk::ResolveModeFlagBits::eNone,
            //                {},
            //                vk::ImageLayout::eAttachmentOptimalKHR,
            //                vk::AttachmentLoadOp::eClear,
            //                vk::AttachmentStoreOp::eStore,
            //                vk::ClearDepthStencilValue(1.0f, 0),
            //        };
            vk::RenderingInfoKHR renderingInfo{
                    {},
                    vk::Rect2D{{}, vk.window.extent()},
                    1,
                    {},
                    colorAttachmentInfo,
                    nullptr,
            };

            cmdBuffer.beginRenderingKHR(renderingInfo);
            render_opaque(app);
            render_imgui(app);
            cmdBuffer.endRenderingKHR();

            cmdBuffer.pipelineBarrier(
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits::eBottomOfPipe,
                    {},
                    {},
                    {},
                    vk::ImageMemoryBarrier{
                            vk::AccessFlagBits::eColorAttachmentWrite,
                            {},
                            vk::ImageLayout::eAttachmentOptimalKHR,
                            vk::ImageLayout::ePresentSrcKHR,
                            {},
                            {},
                            vk.swapchain.swapchain.getImages()[currentBufferIndex],
                            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
                    });
        }
        cmdBuffer.end();

        vk::PipelineStageFlags waitDestStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk.graphicsQueue.submit(vk::SubmitInfo{*vk.presentComplete, waitDestStageMask, *cmdBuffer, *vk.renderComplete});

        [[maybe_unused]] vk::Result result = vk.presentQueue.presentKHR(vk::PresentInfoKHR{*vk.renderComplete, *vk.swapchain.swapchain, currentBufferIndex});

        glfwPollEvents();
        bool& keepOpen = app.globalCtx.at<WindowContext>().keepOpen;
        keepOpen = !glfwWindowShouldClose(vk.window.windowHandle.get());
        if (!keepOpen) {
            vk.device.waitIdle();
        }

    } catch (vk::OutOfDateKHRError const&) {
        recreate_pipeline(vk);
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
