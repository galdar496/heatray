#pragma once

#include "glm/glm/glm.hpp"

#include <limits>

namespace util {

struct AABB {
    AABB() {
        min = glm::vec3(std::numeric_limits<float>::max());
        max = glm::vec3(std::numeric_limits<float>::min());
    }
    ~AABB() = default;
    AABB(const AABB& other) = default;
    AABB& operator=(const AABB& other) = default;

    void expand(const glm::vec3& v) {
        min = glm::min(min, v);
        max = glm::max(max, v);
    }

    glm::vec3 center() const {
        return (min + max) * 0.5f;
    }

    float radius() const {
        return glm::length(max - min);
    }

    glm::vec3 min;
    glm::vec3 max;

};

} // namespace util.
