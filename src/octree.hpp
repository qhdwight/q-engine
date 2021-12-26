#pragma once

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <optional>

template<typename T>
class octree {
private:
    using children_t = std::array<std::shared_ptr<octree<T>>, 8>;
    struct node {
        glm::ivec3 mPoint;
        T val;
        std::weak_ptr<node> parent;
        children_t children;
    };
    glm::ivec3 mPosExt, mNegExt;
    children_t children;
public:
    void insert(glm::ivec3 const& point) {
        if (find(point)) {

        }
        int32_t pos = -1;

    }

    bool find(glm::ivec3 const& point) {
        glm::ivec3 mid = (mPosExt + mNegExt) / 2;

    }
};
