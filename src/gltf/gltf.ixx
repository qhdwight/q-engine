export module gltf;

import logging;

import std;
import entt;
import rapidjson;

namespace gltf {

    constexpr std::uint32_t MAGIC = 0x46546C67;
    constexpr std::uint32_t VERSION = 2;

    enum struct ChunkType : std::uint32_t {
        JSON = 0x4E4F534A,
        BINARY = 0x004E4942,
    };

    export struct Model {
        struct Header {
            std::uint32_t magic{};
            std::uint32_t version{};
            std::uint32_t length{};
        } header;

        struct JsonChunk {

        };

        struct Chunk {
            ChunkType type{};
            std::vector <std::byte> data;
        } jsonChunk, binaryChunk;
    };

    template<typename T>
    T parse(std::istream &stream) {
        T value;
        stream.read(reinterpret_cast<char *>(&value), sizeof(value));
        return value;
    }

    template<>
    Model::Header parse(std::istream &stream) {
        Model::Header header;
        header.magic = parse<std::uint32_t>(stream);
        header.version = parse<std::uint32_t>(stream);
        header.length = parse<std::uint32_t>(stream);

        check_equal(header.magic, MAGIC);
        check_equal(header.version, VERSION);
        return header;
    }

    template<>
    Model::Chunk parse(std::istream &stream) {
        Model::Chunk chunk;
        auto length = parse<std::uint32_t>(stream);
        chunk.data.resize(length);
        chunk.type = parse<ChunkType>(stream);
        stream.read(reinterpret_cast<char *>(chunk.data.data()), length);

        check_in(chunk.type, {ChunkType::JSON, ChunkType::BINARY});
        return chunk;
    }

    template<>
    Model parse(std::istream &stream) {
        Model model;
        model.header = parse<Model::Header>(stream);
        model.jsonChunk = parse<Model::Chunk>(stream);
        model.binaryChunk = parse<Model::Chunk>(stream);
        std::string_view jsonString{reinterpret_cast<char *>(model.jsonChunk.data.data()), model.jsonChunk.data.size()};
        log("{}", jsonString);
        rapidjson::Document document;
        document.Parse(jsonString.data(), jsonString.size());

        check_equal(model.jsonChunk.type, ChunkType::JSON);
        check_equal(model.binaryChunk.type, ChunkType::BINARY);
        check(document.IsObject());
        return model;
    }

    export struct ModelLoader final {
        using result_type = std::shared_ptr<Model>;

        result_type operator()(std::istream &stream) const {
            return std::make_shared<Model>(parse<Model>(stream));
        }
    };
}
