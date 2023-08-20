#pragma once

#include "game_pch.hpp"

#include <rapidjson/document.h>

namespace tinygltf {
    class Model;
}

struct ModelLoader {
    using result_type = std::shared_ptr<rapidjson::Document>;

    result_type operator()(std::string_view name);
};
