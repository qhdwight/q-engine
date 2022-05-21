#pragma once

#include <memory>

#include <tiny_gltf.h>

typedef tinygltf::Scene Scene;

struct SceneLoader {
    using result_type = std::shared_ptr<Scene>;

    result_type operator()(std::string const& name);
};
