export module app;

import collections.circular_buffer;

import pch;

constexpr size_t BufferSize = 4096;

export struct App;

export class Plugin {
public:
    virtual void build(App& app);

    virtual void cleanup(App& app);

    virtual void execute(App& app);

    virtual ~Plugin() = default;
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
    std::shared_ptr<TPlugin> makePlugin(TParams&&... params) {
        auto plugin = std::make_shared<TPlugin>(std::forward<TParams>(params)...);
        plugin->build(*this);
        plugins.push_back(plugin);
        return plugin;
    }
};

void Plugin::build(App& app) {}

void Plugin::execute(App& app) {}

void Plugin::cleanup(App& app) {}
