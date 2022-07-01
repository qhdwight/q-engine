#pragma once

#include "game_pch.hpp"

#include <tiny_gltf.h>

typedef tinygltf::Model Model;

struct ModelLoader {
    using result_type = std::shared_ptr<Model>;

    result_type operator()(std::string const& name);
};
