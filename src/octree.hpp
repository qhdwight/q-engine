#pragma once

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <optional>
#include <variant>
#include <bitset>
#include <iostream>

static constexpr size_t BLOCK_SIZE = 1 << 10 << 3; // 8 KiB

struct small_ivec3 {
    int8_t x: 2, y: 2, z: 2;
};

template<typename T>
class octree {
private:
    struct child {
        uint16_t childOff: 15;
        bool isFar: 1;
        std::byte validMask, leafMask;

        uint32_t ctrOff: 24;
        std::byte ctrMask;
    };


    struct contour {
        small_ivec3 pos;

    };

    using children_t = std::array<child, 8>;

    struct alignas(sizeof(children_t)) child_block {
        uint32_t nextOff;
        static constexpr size_t CHILD_ARRAY_COUNT = (BLOCK_SIZE - sizeof(nextOff)) / sizeof(children_t);
        std::array<children_t, CHILD_ARRAY_COUNT> children;
    };

    struct alignas(sizeof(contour)) ctr_block {
        std::array<contour, BLOCK_SIZE / sizeof(contour)> contours;
    };
public:
    explicit octree() {
        std::cout << sizeof(child_block) << std::endl;
//        std::cout << sizeof(glm::vec << std::endl;
    }

    void insert(glm::ivec3 const& point) {
        if (find(point)) {

        }
    }

    bool find(glm::ivec3 const& point) {
//        glm::ivec3 mid = (mPosExt + mNegExt) / 2;

    }
};
