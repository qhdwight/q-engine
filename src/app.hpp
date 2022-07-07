#pragma once

#include "game_pch.hpp"

#include "plugin.hpp"
#include "assets.hpp"
#include "collections/circular_buffer.hpp"

using Context = entt::registry::context;

struct World : entt::registry {
};

constexpr size_t BufferSize = 4096;

struct App {
    World logicWorld;
    World renderWorld;
    circular_buffer<World, BufferSize> cmdWorldHistory;
    Context globalCtx;
    ModelAssets modelAssets;
    std::vector<std::shared_ptr<Plugin>> plugins;

    template<std::derived_from<Plugin> TPlugin, typename ...TParams>
    std::shared_ptr<TPlugin> makePlugin(TParams&& ... params) {
        auto plugin = std::make_shared<TPlugin>(std::forward<TParams>(params)...);
        plugin->build(*this);
        plugins.push_back(plugin);
        return plugin;
    }

    ~App();
};