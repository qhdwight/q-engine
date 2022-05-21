#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include "assets.hpp"

SceneLoader::result_type SceneLoader::operator()(std::string const& name) {
    tinygltf::TinyGLTF loader;
    auto model = std::make_shared<tinygltf::Model>();
    std::string err;
    std::string warn;
    loader.LoadBinaryFromFile(model.get(), &err, &warn, name);
    return nullptr;
}
