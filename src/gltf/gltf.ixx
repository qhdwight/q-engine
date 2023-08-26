export module gltf;

import common;
import logging;

import std;
import entt;
import rapidjson;

namespace gltf {

    template<typename T>
    concept Unitable = std::is_integral_v<T> || std::is_floating_point_v<T>;

    template<typename T>
    concept Enumable = std::is_enum_v<T>;

    constexpr u32 MAGIC = 0x46546C67;
    constexpr u32 VERSION = 2;

    enum struct ChunkType : u32 {
        JSON = 0x4E4F534A,
        BINARY = 0x004E4942,
    };

    enum struct ComponentType : u16 {
        I8 = 5120,
        U8 = 5121,
        I16 = 5122,
        U16 = 5123,
        I32 = 5124,
        U32 = 5125,
        F32 = 5126,
    };

    export struct ModelAsset {
        struct Header {
            u32 version{};
            u32 length{};
        } header;

        struct Chunk {
            ChunkType type{};
            std::vector<std::byte> data;
        } jsonChunk, binaryChunk;

        struct Accesor {

        };

        struct BufferView {

        };

        struct Mesh {
            std::string name;
        };
    };

    template<typename T>
    T parse(std::istream& stream) {
        static_assert(Unitable<T>); // TODO: why can this not be concept
        T value{};
        stream.read(reinterpret_cast<char*>(&value), sizeof(value));
        if constexpr (std::endian::native == std::endian::little) {
            return value;
        }
        return std::byteswap(value);
    }

    template<Enumable T>
    T parse(std::istream& stream) {
        return static_cast<T>(parse<std::underlying_type_t<T>>(stream));
    }

    template<>
    ModelAsset::Header parse<ModelAsset::Header>(std::istream& stream) {
        ModelAsset::Header header;
        auto magic = parse<u32>(stream);
        header.version = parse<u32>(stream);
        header.length = parse<u32>(stream);

        checkEqual(magic, MAGIC);
        checkEqual(header.version, VERSION);
        return header;
    }

    template<>
    ModelAsset::Chunk parse<ModelAsset::Chunk>(std::istream& stream) {
        ModelAsset::Chunk chunk;
        auto length = parse<u32>(stream);
        chunk.data.resize(length);
        chunk.type = parse<ChunkType>(stream);
        stream.read(reinterpret_cast<char*>(chunk.data.data()), length);

        checkIn(chunk.type, {ChunkType::JSON, ChunkType::BINARY});
        return chunk;
    }

    template<>
    ModelAsset parse<ModelAsset>(std::istream& stream) {
        ModelAsset model;
        model.header = parse<ModelAsset::Header>(stream);
        model.jsonChunk = parse<ModelAsset::Chunk>(stream);
        model.binaryChunk = parse<ModelAsset::Chunk>(stream);
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
        using result_type = std::shared_ptr<ModelAsset>;

        result_type operator()(std::istream& stream) const {
            check(stream.good());
            return std::make_shared<ModelAsset>(parse<ModelAsset>(stream));
        }
    };

}// namespace gltf
