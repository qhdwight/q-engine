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
    GAME_ASSERT(!vk.commandBuffers.empty());

    try {
        vk::raii::Fence const& fence = vk.waitFences[vk.currentFrame];
        vk::raii::Semaphore const& presentationComplete = vk.presentationCompleteSemaphores[vk.currentFrame];
        vk::raii::Semaphore const& renderComplete = vk.renderCompleteSemaphores[vk.currentFrame];
        vk::raii::CommandBuffer const& cmdBuffer = vk.commandBuffers[vk.currentFrame];
        GAME_ASSERT(*fence);
        GAME_ASSERT(*presentationComplete);
        GAME_ASSERT(*renderComplete);
        GAME_ASSERT(*cmdBuffer);

        vk::Result waitResult = vk.device.waitForFences(*fence, VK_TRUE, FENCE_TIMEOUT);
        GAME_ASSERT(waitResult == vk::Result::eSuccess);

        vk::Result acquireResult;
        std::tie(acquireResult, vk.currentSwapchainImageIndex) = vk.swapchain.swapchain.acquireNextImage(FENCE_TIMEOUT, *presentationComplete, nullptr);
        GAME_ASSERT(acquireResult == vk::Result::eSuccess);

        //
        //    for (auto [ent, shaderHandle]: app.renderWorld.view<const ShaderHandle>().each()) {
        //        if (vk.modelPipelines.contains(shaderHandle.value)) continue;
        //
        //        // Create blank shader and fill it in
        //        createShaderPipeline(vk, vk.modelPipelines[shaderHandle.value]);
        //    }
        //

        vk.device.resetFences(*fence);

        cmdBuffer.reset();
        cmdBuffer.begin({});

        cmdBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eTopOfPipe,
                {},
                {},
                {},
                vk::ImageMemoryBarrier{
                        vk::AccessFlagBits::eColorAttachmentWrite,
                        {},
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eColorAttachmentOptimal,
                        {},
                        {},
                        vk.swapchain.swapchain.getImages()[vk.currentSwapchainImageIndex],
                        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
                });
        cmdBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
                vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
                {},
                {},
                {},
                vk::ImageMemoryBarrier{
                        vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                        {},
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eStencilAttachmentOptimal,
                        {},
                        {},
                        vk.swapchain.swapchain.getImages()[vk.currentSwapchainImageIndex],
                        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eStencil | vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1},
                });

        {
            vk::RenderingAttachmentInfo colorAttachmentInfo{
                    *vk.swapchain.views[vk.currentSwapchainImageIndex],
                    vk::ImageLayout::eAttachmentOptimalKHR,
                    vk::ResolveModeFlagBits::eNone,
                    {},
                    {},
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ClearColorValue{0.2f, 0.2f, 0.2f, 0.2f},
            };
            vk::RenderingAttachmentInfo depthAttachmentInfo{
                    *vk.depthImage->view,
                    vk::ImageLayout::eAttachmentOptimalKHR,
                    vk::ResolveModeFlagBits::eNone,
                    {},
                    {},
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ClearDepthStencilValue(1.0f, 0),
            };
            vk::RenderingInfoKHR renderingInfo{
                    {},
                    vk::Rect2D{{}, vk.swapchain.extent},
                    1,
                    {},
                    colorAttachmentInfo,
                    &depthAttachmentInfo,
            };

            cmdBuffer.beginRenderingKHR(renderingInfo);
            renderOpaque(app);
            renderImgui(app);
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
                            vk::ImageLayout::eColorAttachmentOptimal,
                            vk::ImageLayout::ePresentSrcKHR,
                            {},
                            {},
                            vk.swapchain.swapchain.getImages()[vk.currentSwapchainImageIndex],
                            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
                    });
        }
        cmdBuffer.end();

        vk::PipelineStageFlags waitDestStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk.graphicsQueue.submit(vk::SubmitInfo{*presentationComplete, waitDestStageMask, *cmdBuffer, *renderComplete}, *fence);

        vk::Result presentResult = vk.presentQueue.presentKHR(vk::PresentInfoKHR{*renderComplete, *vk.swapchain.swapchain, vk.currentSwapchainImageIndex});
        GAME_ASSERT(presentResult == vk::Result::eSuccess);

        glfwPollEvents();
        bool& keepOpen = app.globalCtx.at<WindowContext>().keepOpen;
        keepOpen = !glfwWindowShouldClose(vk.window.windowHandle.get());
        if (!keepOpen) {
            vk.device.waitIdle();
        }

        vk.currentFrame = (vk.currentFrame + 1) % vk.commandBuffers.size();

    } catch (vk::OutOfDateKHRError const&) {
        recreatePipeline(vk);
    }
}

void VulkanRenderPlugin::cleanup(App& app) {
    glfwTerminate();
    ImGui_ImplVulkan_Shutdown();
    //    glslang::FinalizeProcess();
    //    for (auto& [_, pipeline]: app.globalCtx.at<VulkanContext>().modelPipelines) {
    //        for (auto& item: pipeline.shaders) {
    //            spvReflectDestroyShaderModule(&item.reflect);
    //        }
    //    }
}
