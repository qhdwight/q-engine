export module imgui;

import std;

export import <backends/imgui_impl_glfw.h>;
export import <backends/imgui_impl_vulkan.h>;

export namespace ImGui {

    void CheckVersion() {
        IMGUI_CHECKVERSION();
    }

}// namespace ImGui
