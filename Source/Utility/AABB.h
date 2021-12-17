//
//  AABB.h
//  Heatray
//
//  Represents a simple axis-aligned box.
//
//

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

    //-------------------------------------------------------------------------
    // Expand the AABB to contain the new point.
    void expand(const glm::vec3& v) {
        min = glm::min(min, v);
        max = glm::max(max, v);
    }

    glm::vec3 center() const {
        glm::vec3 transformedMin = transform * glm::vec4(min, 1.0f);
        glm::vec3 transformedMax = transform * glm::vec4(max, 1.0f);
        return (transformedMin + transformedMax) * 0.5f;
    }

    float radius() const {
        glm::vec3 transformedMin = transform * glm::vec4(min, 1.0f);
        glm::vec3 transformedMax = transform * glm::vec4(max, 1.0f);
        return glm::length(transformedMax - transformedMin);
    }
    
    float bottom() const {
        glm::vec3 transformedMin = transform * glm::vec4(min, 1.0f);
        return transformedMin.y;
    }

    // Corners of the box.
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);
    
    glm::mat4 transform = glm::mat4(1.0f);
};

} // namespace util.
