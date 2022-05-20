#pragma once

#include <string>
#include <numeric>
#include <iostream>
#include <optional>
#include <unordered_map>

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/detail/type_vec3.hpp>

typedef int64_t ns_t;
typedef std::string ItemName;
typedef std::string ItemStateName;
typedef std::string EquipStateName;
typedef entt::registry::entity_type ent_t;

struct Position {
    glm::dvec3 vec;

    glm::dvec3* operator->() noexcept { return &vec; }
};

struct LinearVelocity {
    glm::dvec3 vec;

    glm::dvec3* operator->() noexcept { return &vec; }
};

struct Orientation {
    glm::dquat quat;

    glm::dquat* operator->() noexcept { return &quat; }
};

struct Look {
    glm::dvec3 vec;

    glm::dvec3* operator->() noexcept { return &vec; }
};

struct Timestamp {
    ns_t ns, deltaNs;
};

struct Player {

};

struct Cube {

};

struct Key {
    bool previous: 1;
    bool current: 1;
};

struct Input {
    glm::dvec2 cursor, cursorDelta;
    glm::dvec3 move;
    double lean;
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
    double moveFactor;
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
    std::array<long long, 128> frameTimesNs;
    size_t frameTimesIndex;
    size_t readingCount;

    void addFrameTime(ns_t deltaNs);

    [[nodiscard]] double getAvgFrameTime() const;
};

struct World {
    entt::registry reg;
    entt::registry::entity_type sharedEnt;

    entt::registry* operator->() noexcept { return &reg; }
};
