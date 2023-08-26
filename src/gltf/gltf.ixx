export module gltf;

import common;
import logging;

import std;
import entt;
import rapidjson;

namespace gltf {

    constexpr u32 MAGIC = 0x46546C67;
    constexpr u32 VERSION = 2;

    enum struct ChunkType : u32 {
        JSON = 0x4E4F534A,
        BINARY = 0x004E4942,
    };

    export struct Model {
        struct Header {
            u32 magic{};
            u32 version{};
            u32 length{};
        } header;

        struct Chunk {
            ChunkType type{};
            std::vector<std::byte> data;
        } jsonChunk, binaryChunk;
    };

    template<typename T>
    T parse(std::istream& stream) {
        T value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    }

    template<>
    Model::Header parse(std::istream& stream) {
        Model::Header header;
        header.magic = parse<u32>(stream);
        header.version = parse<u32>(stream);
        header.length = parse<u32>(stream);

        checkEqual(header.magic, MAGIC);
        checkEqual(header.version, VERSION);
        return header;
    }

    template<>
    Model::Chunk parse(std::istream& stream) {
        Model::Chunk chunk;
        auto length = parse<u32>(stream);
        chunk.data.resize(length);
        chunk.type = parse<ChunkType>(stream);
        stream.read(reinterpret_cast<char*>(chunk.data.data()), length);

        checkIn(chunk.type, {ChunkType::JSON, ChunkType::BINARY});
        return chunk;
    }

    template<>
    Model parse(std::istream& stream) {
        Model model;
        model.header = parse<Model::Header>(stream);
        model.jsonChunk = parse<Model::Chunk>(stream);
        model.binaryChunk = parse<Model::Chunk>(stream);
        std::string_view jsonString{reinterpret_cast<char*>(model.jsonChunk.data.data()), model.jsonChunk.data.size()};
        log("{}", jsonString);
        rapidjson::Document document;
        document.Parse(jsonString.data(), jsonString.size());

        checkEqual(model.jsonChunk.type, ChunkType::JSON);
        checkEqual(model.binaryChunk.type, ChunkType::BINARY);
        check(document.IsObject());
        return model;
    }

    export struct ModelLoader final {
        using result_type = std::shared_ptr<Model>;

        result_type operator()(std::istream& stream) const {
            return std::make_shared<Model>(parse<Model>(stream));
        }
    };
}// namespace gltf
