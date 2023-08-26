module;

#include <entt/entt.hpp>

export module entt;

export namespace entt {

    using entt::registry;
    using entt::resource_cache;

    namespace literals {
        using entt::literals::operator""_hs;
    }

}// namespace entt

//export namespace entt::literals {
//
//    using entt::literals::operator ""_hs;
//
//}// namespace entt::literals
