#pragma once

#include "game_pch.hpp"

#include "plugin.hpp"
#include "assets.hpp"

typedef entt::registry::context Context;

using ModelAssets = entt::resource_cache<Model, ModelLoader>;

struct World : entt::registry {
};

struct App {
    World logicWorld;
    World renderWorld;
    Context globalCtx;
    ModelAssets modelAssets;
    std::vector<std::shared_ptr<Plugin>> plugins;

    template<typename TPlugin, typename ...TParams>
    requires std::derived_from<TPlugin, Plugin>
    std::shared_ptr<TPlugin> makePlugin(TParams&& ... params) {
        auto plugin = std::make_shared<TPlugin>(std::forward<TParams>(params)...);
        plugin->build(*this);
        plugins.push_back(plugin);
        return plugin;
    }

    ~App();
};