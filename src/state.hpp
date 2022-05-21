#pragma once

#include <string>
#include <numeric>
#include <iostream>
#include <optional>
#include <unordered_map>

#include <entt/entt.hpp>
#include <edyn/math/vector2.hpp>
#include <edyn/math/matrix3x3.hpp>
#include <edyn/comp/position.hpp>
#include <edyn/comp/orientation.hpp>

typedef int64_t ns_t;
typedef std::string ItemName;
typedef std::string ItemStateName;
typedef std::string EquipStateName;
typedef entt::registry::entity_type ent_t;
typedef edyn::position Position;
typedef edyn::orientation Orientation;
typedef edyn::scalar scalar;
typedef edyn::vector3 vec3;
typedef edyn::vector2 vec2;
typedef uint8_t player_id_t;

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

struct Cube {

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

typedef entt::registry::context Resources;

struct App {
    World logicWorld;
    World renderWorld;
    Resources resources;
};

struct ExecuteContext {
    World& world;
    Resources& resources;
};
