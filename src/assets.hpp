#pragma once

#include "game_pch.hpp"

#include <tiny_gltf.h>

using Model = tinygltf::Model;

struct ModelLoader {
    using result_type = std::shared_ptr<Model>;

    result_type operator()(std::string_view name);
};

using ModelAssets = entt::resource_cache<Model, ModelLoader>;
