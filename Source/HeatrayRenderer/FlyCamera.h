//
//  FlyCamera.h
//  Heatray
//
//  
//

#pragma once

#define GLM_FORCE_SWIZZLE
#include <glm/glm/mat4x4.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtx/quaternion.hpp>
#include <glm/glm/vec3.hpp>

struct FlyCamera
{
    glm::vec3 position;     ///< Position of the camera in world space.
    glm::quat orientation;  ///< Which direction is the camera currently pointing.

    // Angle is in degrees.
    void pitch(const float angle)
    {
        constexpr glm::vec3 x = glm::vec3(1.0f, 0.0f, 0.0f);
        orientation = glm::angleAxis(angle, x) * orientation;
        orientation = glm::normalize(orientation);
    }

    // Angle is in degrees.
    void yaw(const float angle)
    {
        constexpr glm::vec3 y = glm::vec3(0.0f, 1.0f, 0.0f);
        orientation = glm::angleAxis(angle, y) * orientation;
        orientation = glm::normalize(orientation);
    }

    // Angle is in degrees.
    void roll(const float angle)
    {
        constexpr glm::vec3 z = glm::vec3(0.0f, 0.0f, 1.0f);
        orientation = glm::angleAxis(angle, z) * orientation;
        orientation = glm::normalize(orientation);
    }

    glm::mat4 createViewMatrix() const
    {
        glm::mat4 viewMatrix = glm::mat4_cast(glm::inverse(orientation));

        // Set the position part of the transform matrix.
        glm::vec3 back      = viewMatrix[2].xyz() * -1.0f;
        glm::vec3 up        = viewMatrix[1].xyz();
        glm::vec3 right     = viewMatrix[0].xyz();

        viewMatrix[0][3] = glm::dot(right, position) * -1.0f;
        viewMatrix[1][3] = glm::dot(up, position) * -1.0f;
        viewMatrix[2][3] = glm::dot(back, position) * -1.0f;

        return std::move(viewMatrix);
    }

};
