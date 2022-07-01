#include "app.hpp"
#include "plugin.hpp"

App::~App() {
    for (std::shared_ptr<Plugin>& plugin: plugins) {
        plugin->cleanup(*this);
    }
}
