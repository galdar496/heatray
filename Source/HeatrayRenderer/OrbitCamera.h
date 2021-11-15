//
//  OrbitCamera.h
//  Heatray
//
//  
//

#pragma once

#define GLM_FORCE_SWIZZLE
#include <glm/glm/mat4x4.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtx/polar_coordinates.hpp>
#include <glm/glm/gtx/quaternion.hpp>
#include <glm/glm/vec3.hpp>

#include <algorithm>
#include <assert.h>

struct OrbitCamera
{
    float distance = 19.0f;             /// Distance to the camera from the look-at position in world-space.
    float phi      = 0.0f;              ///< In radians [0 - 2π]
    float theta    = 0.0f;              ///< In radians [-π/2 - π/2]
    glm::vec3 target = glm::vec3(0.0f); ///< Position to look at in world-space.

    float max_distance = 100.0f; ///< Maximum distance the camera can go.

    glm::mat4x4 createViewMatrix() const
    {
        constexpr glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
        constexpr glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::quat rotTheta = glm::angleAxis(theta, right);
        glm::quat rotPhi = glm::angleAxis(phi, up);
        glm::quat orientation = rotTheta * rotPhi;

        const glm::mat4 translation = glm::translate(glm::mat4(1), target + glm::vec3(0, 0, distance));
        const glm::mat4 viewMatrix = glm::mat4_cast(glm::inverse(orientation)) * translation;

        return std::move(viewMatrix);
    }
};
