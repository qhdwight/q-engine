#pragma once

#include "game_pch.hpp"

#include "assets.hpp"

using ns_t = int64_t;
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
    ns_t ns, deltaNs;
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

struct Player {
    possesion_id_t possessionId;
};

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
};

struct FlyPlayerMove {
    scalar speed;
};

struct Key {
    bool previous: 1;
    bool current: 1;
};

struct Input {
    vec2 cursor, cursorDelta;
    vec3 move;
    scalar lean;
    Key menu;
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

struct DiagnosticResource {
    std::array<ns_t, 128> frameTimesNs;
    size_t frameTimesIndex;
    size_t readingCount;

    void addFrameTime(ns_t deltaNs);

    [[nodiscard]] double getAvgFrameTime() const;
};
