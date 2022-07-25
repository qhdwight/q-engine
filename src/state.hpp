#pragma once

#include "game_pch.hpp"

#include "assets.hpp"

using ItemId = entt::hashed_string;
using ItemStateId = entt::hashed_string;
using EquipStateId = entt::hashed_string;
using ent_t = entt::registry::entity_type;
using Position = edyn::position;
using Orientation = edyn::orientation;
using LinearVelocity = edyn::linvel;
using possesion_id_t = uint8_t;
using asset_handle_t = entt::id_type;
using equip_idx_t = uint8_t;

struct GameSettings {
    scalar gravity;
};

struct Handle {
    asset_handle_t value;
};

struct ModelHandle : Handle {
};

struct ShaderHandle : Handle {
};

struct TexHandle : Handle {
};

struct Look : vec3 {
};

struct Timestamp {
    clock_point_t point;
    clock_delta_t delta{};
};

enum class Authority {
    Client,
    Server
};

struct LocalContext {
    std::optional<possesion_id_t> possessionId;
    Authority authority{};
};

struct RenderContext {
    std::optional<possesion_id_t> possessionId;
};

// #REFLECT()
struct Player {
    possesion_id_t possessionId;
};

// #REFLECT()
struct MoveStats {
    vec3 wishDir;
};

// #REFLECT()
struct GroundedPlayerMove {
    scalar gravity;
    scalar walkSpeed;
    scalar runSpeed;
    scalar fwdSpeed;
    scalar sideSpeed;
    scalar airSpeedCap;
    scalar airAccel;
    scalar maxAirSpeed;
    scalar accel;
    scalar friction;
    scalar frictionCutoff;
    scalar jumpSpeed;
    scalar stopSpeed;
    uint8_t groundTick;
    vec3 linVel;
};

// #REFLECT()
struct FlyPlayerMove {
    scalar speed;
};

struct Key {
    bool previous: 1;
    bool current: 1;
};

// #REFLECT()
struct Input {
    vec2 cursor, cursorDelta;
    vec3 move;
    scalar lean;
    Key menu;
    Key jump;
};

struct UI {
    bool isVisible;
};

struct ItemStateProps {
    ns_t duration;
    bool isPersistent;
};

struct ItemProps {
    ItemId id;
    scalar moveFactor;
    std::unordered_map<ItemStateId, ItemStateProps> states;
    std::unordered_map<EquipStateId, ItemStateProps> equipStates;
};

struct WeaponProps {
    uint16_t damage;
    double headshotFactor;
    ItemProps itemProps;
};

struct Gun {
    uint16_t ammo;
    uint16_t ammoInReserve;
};

struct Item {
    ItemId id;
    uint16_t amount;
    ItemStateId stateId;
    ns_t stateDur;
    ent_t invEnt;
    equip_idx_t invSlot;
};

struct ItemPickup {
    ItemId id;
};

struct Inventory {
    std::optional<equip_idx_t> equippedSlot;
    std::optional<equip_idx_t> prevEquippedSlot;
    EquipStateId equipStateId;
    ns_t equipStateDur;
    std::array<std::optional<ent_t>, 10> items;
};

// #REFLECT()
struct Material {
    vec4f baseColorFactor;
    vec4f emissiveFactor;
    vec4f diffuseFactor;
    vec4f specularFactor;
    float workflow;
    int baseColorTextureSet;
    int physicalDescriptorTextureSet;
    int normalTextureSet;
    int occlusionTextureSet;
    int emissiveTextureSet;
    float metallicFactor;
    float roughnessFactor;
    float alphaMask;
    float alphaMaskCutoff;
};

struct DiagnosticResource {
    std::array<clock_delta_t, 128> frameTimes{};
    size_t frameTimesIndex{};
    size_t readingCount{};

    void addFrameTime(clock_delta_t delta);

    [[nodiscard]] clock_delta_t getAvgFrameTime() const;
};


#if __has_include("generated/state.generated.hpp")

#include "generated/state.generated.hpp"

#else

#error "Please run tools/meta.go!"

#endif

static void register_reflection() {
    entt::meta<Position>()
            .data<&Position::x>("x"_hs)
            .prop("display_name"_hs, "X"sv)
            .data<&Position::y>("y"_hs)
            .prop("display_name"_hs, "Y"sv)
            .data<&Position::z>("z"_hs)
            .prop("display_name"_hs, "Z"sv);
    register_generated_reflection();
}
