#include "assets.hpp"

#include <rapidjson/istreamwrapper.h>

ModelLoader::result_type ModelLoader::operator()(std::string_view name) {
    std::filesystem::path path = std::filesystem::current_path() / "assets" / name;
    auto document = std::make_shared<rapidjson::Document>();
    std::ifstream stream{path};
    rapidjson::IStreamWrapper wrapper{stream};
    document->ParseStream(wrapper);
    return document;
}
