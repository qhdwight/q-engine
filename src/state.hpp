#pragma once

#include <iostream>
#include <string>
#include <numeric>
#include <unordered_map>

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/detail/type_vec3.hpp>

struct position {
    glm::dvec3 vec;
};
struct orientation {
    glm::dquat quat;
};

struct look {
    glm::dvec3 vec;
};

struct timestamp {
    long long ns, deltaNs;
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

struct DiagnosticResource {
    std::array<long long, 128> frameTimesNs;
    size_t frameTimesIndex;
    size_t readingCount;

    void addFrameTime(long long deltaNs) {
        frameTimesNs[frameTimesIndex++] = deltaNs;
        frameTimesIndex %= frameTimesNs.size();
        readingCount = std::min(readingCount + 1, frameTimesNs.size());
    }

    [[nodiscard]] double getAvgFrameTime() const {
        double avgFrameTime;
        for (size_t i = 0; i < readingCount; ++i) {
            avgFrameTime += static_cast<double>(frameTimesNs[i]) / static_cast<double>(readingCount);
        }
        return avgFrameTime;
    }
};

struct World {
    entt::registry reg;
    entt::registry::entity_type sharedEnt;

    entt::registry* operator->() noexcept {
        return &reg;
    }
};
