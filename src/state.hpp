#pragma once

#include <string>
#include <numeric>
#include <iostream>
#include <optional>
#include <unordered_map>

#include <entt/entt.hpp>
#include <edyn/edyn.hpp>

#include "assets.hpp"

using ns_t = int64_t;
using ItemName = entt::hashed_string;
using ItemStateName = entt::hashed_string;
using EquipStateName = entt::hashed_string;
using ent_t = entt::registry::entity_type;
using Position = edyn::position;
using Orientation = edyn::orientation;
using LinearVelocity = edyn::linvel;
using scalar = edyn::scalar;
using vec3 = edyn::vector3;
using vec2 = edyn::vector2;
using player_id_t = uint8_t;
using asset_handle_t = entt::id_type;
using ModelAssets = entt::resource_cache<Model, ModelLoader>;

struct ModelHandle {
    asset_handle_t value;
};

struct Look : vec3 {
};

struct Timestamp {
    ns_t ns, deltaNs;
};

struct LocalContext {
    player_id_t localId;
};

struct RenderContext {
    std::optional<player_id_t> playerId;
};

struct Player {
    player_id_t id;
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
    bool visible;
};

struct ItemStateProps {
    ns_t duration;
    bool isPersistent;
};

struct ItemProps {
    ItemName name;
    scalar moveFactor;
    std::unordered_map<ItemStateName, ItemStateProps> states;
    std::unordered_map<EquipStateName, ItemStateProps> equipStates;
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
    ItemName name;
    uint16_t amount;
    ItemStateName stateName;
    ns_t stateDur;
    ent_t invEnt;
    uint8_t invSlot;
};

struct ItemPickup {
    ItemName name;
};

struct Inventory {
    std::optional<uint8_t> equippedSlot;
    std::optional<uint8_t> prevEquippedSlot;
    EquipStateName equipStateName;
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

struct World : entt::registry {
};

typedef entt::registry::context Context;

struct App {
    World logicWorld;
    World renderWorld;
    Context globalCtx;
    ModelAssets modelAssets;
};
