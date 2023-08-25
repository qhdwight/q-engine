module;

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

export module imgui;

export namespace ImGui {

    void CheckVersion() {
        IMGUI_CHECKVERSION();
    }

    using ::ImGui_ImplGlfw_InitForVulkan;
    using ::ImGui_ImplGlfw_NewFrame;
    using ::ImGui_ImplVulkan_CreateFontsTexture;
    using ::ImGui_ImplVulkan_DestroyFontUploadObjects;
    using ::ImGui_ImplVulkan_Init;
    using ::ImGui_ImplVulkan_InitInfo;
    using ::ImGui_ImplVulkan_NewFrame;
    using ::ImGui_ImplVulkan_RenderDrawData;
    using ::ImGui_ImplVulkan_Shutdown;
    using ::ImGuiIO;
    using ImGui::CreateContext;
    using ImGui::GetDrawData;
    using ImGui::GetIO;
    using ImGui::GetVersion;
    using ImGui::NewFrame;
    using ImGui::Render;
    using ImGui::ShowDemoWindow;
    using ImGui::StyleColorsDark;

}// namespace ImGui
