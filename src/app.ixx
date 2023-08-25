export module app;

import collections.circular_buffer;

import std;
import entt;

export struct App;

export class Plugin {
public:
    virtual ~Plugin() = default;

    virtual void build(App& app) = 0;

    virtual void execute(App& app) = 0;

    virtual void cleanup(App& app) = 0;
};

export struct App {
    App() = default;

    ~App() {
        for (std::shared_ptr<Plugin>& plugin: plugins) {
            plugin->cleanup(*this);
        }
    }

    entt::registry logicWorld;
    entt::registry renderWorld;
    //    circular_buffer<entt::registry, BufferSize> cmdWorldHistory{};
    entt::registry::context globalContext{{}};
    std::vector<std::shared_ptr<Plugin>> plugins;

    template<std::derived_from<Plugin> TPlugin, typename... TParams>
    std::shared_ptr<TPlugin> makePlugin(TParams&& ... params) {
        auto plugin = std::make_shared<TPlugin>(std::forward<TParams>(params)...);
        plugin->build(*this);
        plugins.push_back(plugin);
        return plugin;
    }
};
