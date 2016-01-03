//
//  Camera.cpp
//  Heatray
//
//  Created by Cody White on 5/12/14.
//
//

#include "Camera.h"

namespace gfx
{

/// Default constructor.
Camera::Camera() :
    m_field_of_view(45.0f),
    m_aspect_ratio(1.0f),
    m_focal_length(50.0f),
    m_aperture_radius(0.0f)
{
    m_orientation.identity();
    updateViewMatrix();
}

/// Destructor.
Camera::~Camera()
{
}

/// Set the position of the camera.
void Camera::setPosition(const math::vec3f &position)
{
	m_position = position;
    updateMatrixPosition();
}
    
/// Explicitly set the orientation of the camera.
void Camera::setOrientation(const math::quatf &orientation)
{
    m_orientation = orientation;
    updateViewMatrix();
}

/// Get the current position of the camera in world space.
const math::vec3f Camera::getPosition() const
{
    return m_position;
}

/// Get the current orientation of the camera (as a quaternion).
math::quatf Camera::getOrientation() const
{
	return m_orientation;
}

/// Move the camera along it local axis.
void Camera::move(const float x_distance, const float y_distance, const float z_distance)
{
    m_position += getForwardVector () * -z_distance;
    m_position += getUpVector () * y_distance;
    m_position += getRightVector () * x_distance;
    updateMatrixPosition ();
}

/// Move the camera along its right vector (x-axis).
void Camera::strafe(const float x_distance)
{
	move(x_distance, 0.0f, 0.0f);
}

/// Rotate about the x axis.
void Camera::pitch(const float angle)
{
	math::quatf tmp(angle, math::vec3f(1.0f, 0.0f, 0.0f), true);
    rotateWorld(tmp);
}

/// Rotate about the y axis.
void Camera::yaw(const float angle)
{
    math::quatf tmp(angle, math::vec3f(0.0f, 1.0f, 0.0f), true);
    rotateWorld(tmp);
}

/// Rotate about the z axis. value is in radians.
void Camera::roll(const float angle)
{
    math::quatf tmp(angle, math::vec3f(0.0f, 0.0f, 1.0f), true);
    rotateWorld(tmp);
}

/// Rotate about an arbitrary world-space axis (Can contain mutliple rotations).
void Camera::rotateWorld(const math::quatf &q)
{
    m_orientation = q * m_orientation;
    m_orientation.normalize();
    updateViewMatrix();
}

/// Rotate about an arbitrary object-space axis (Can contain mutliple rotations).
void Camera::rotateLocal(const math::quatf &q)
{
    m_orientation = m_orientation * q;
    m_orientation.normalize();
    updateViewMatrix();
}

/// Get the normalized forward vector from the current view.
const math::vec3f Camera::getForwardVector() const
{
    return math::vec3f(-m_view_matrix(2, 0), -m_view_matrix(2, 1), -m_view_matrix(2, 2));
}

/// Get the normalized up vector from the current view.
const math::vec3f Camera::getUpVector() const
{
    return math::vec3f(m_view_matrix(1, 0), m_view_matrix(1, 1), m_view_matrix(1, 2));
}

/// Get the normalized right vector from the current view.
const math::vec3f Camera::getRightVector() const
{
    return math::vec3f(m_view_matrix(0, 0), m_view_matrix(0, 1), m_view_matrix(0, 2));
}

/// Get the current model view matrix.
math::Mat4f Camera::getModelViewMatrix() const
{
	return m_view_matrix;
}

/// Set the field of view (y) of the camera.
/// Changes will not take affect until setPerspective() is called.
void Camera::setFOV(const float fov)
{
	m_field_of_view = fov;
}
    
/// Set the aspect ratio of the camera.
void Camera::setAspectRatio(const float ratio)
{
	m_aspect_ratio = ratio;
}
    
/// Set the focal length of the camera.
void Camera::setFocalLength(const float length)
{
	m_focal_length = length;
}
    
/// Set the radius of the aperture of the camera.
void Camera::setApertureRadius(const float radius)
{
	m_aperture_radius = radius;
}
    
/// Get the field of view of the camera in degrees.
float Camera::getFOV() const
{
	return m_field_of_view;
}

/// Get the aspect ratio of the camera.
float Camera::getAspectRatio() const
{
	return m_aspect_ratio;
}
    
/// Get the focal length of the camera.
float Camera::getFocalLength() const
{
	return m_focal_length;
}

/// Get the radius of the aperture of the camera.
float Camera::getApertureRadius() const
{
	return m_aperture_radius;
}

/// Update the internally stored view matrix.
void Camera::updateViewMatrix()
{
    math::quatf tmp = m_orientation.inverse();
    tmp.toMatrix(m_view_matrix);
    updateMatrixPosition();
}

/// Update position of the camera.
void Camera::updateMatrixPosition()
{
    // Set the translation part of the matrix.
    math::vec3f back  = getForwardVector() * -1.0f;
    math::vec3f up    = getUpVector();
    math::vec3f right = getRightVector();
    
    m_view_matrix(0, 3) = -math::dot(right, m_position);
    m_view_matrix(1, 3) = -math::dot(up, m_position);
    m_view_matrix(2, 3) = -math::dot(back, m_position);
}
    
} // namespace gfx
