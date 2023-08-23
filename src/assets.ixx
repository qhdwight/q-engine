module;

#include <pch.hpp>

export module assets;

import std;

export struct ModelLoader {
    using result_type = std::shared_ptr<rapidjson::Document>;

    result_type operator()(std::string_view name) {
        std::filesystem::path path = std::filesystem::current_path() / "assets" / name;
        auto document = std::make_shared<rapidjson::Document>();
        std::ifstream stream{path};
        rapidjson::IStreamWrapper wrapper{stream};
        document->ParseStream(wrapper);
        return document;
    }
};