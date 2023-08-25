export module imgui;

import std;

export import <backends/imgui_impl_glfw.h>;
export import <backends/imgui_impl_vulkan.h>;

export namespace ImGui {

    void checkVersion() {
        IMGUI_CHECKVERSION();
    }

    constexpr std::string_view VERSION = IMGUI_VERSION;

}// namespace ImGui
