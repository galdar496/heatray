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

Camera::Camera() :
    m_fieldOfView(45.0f),
    m_aspectRatio(1.0f),
    m_focalDistance(50.0f),
    m_apertureRadius(0.0f)
{
    m_orientation.Identity();
    UpdateViewMatrix();
}

Camera::~Camera()
{
}

void Camera::SetPosition(const math::Vec3f &position)
{
	m_position = position;
    UpdateMatrixPosition();
}
    
void Camera::SetOrientation(const math::Quatf &orientation)
{
    m_orientation = orientation;
    UpdateViewMatrix();
}

const math::Vec3f Camera::GetPosition() const
{
    return m_position;
}

math::Quatf Camera::GetOrientation() const
{
	return m_orientation;
}

void Camera::Move(const float x_distance, const float y_distance, const float z_distance)
{
    m_position += GetForwardVector() * -z_distance;
    m_position += GetUpVector() * y_distance;
    m_position += GetRightVector() * x_distance;
    UpdateMatrixPosition();
}

void Camera::Pitch(const float angle)
{
    static math::Vec3f x(1.0f, 0.0f, 0.0f);
    
	math::Quatf tmp(angle, x, true);
    RotateWorld(tmp);
}

void Camera::Yaw(const float angle)
{
    static math::Vec3f y(0.0f, 1.0f, 0.0f);
    
    math::Quatf tmp(angle, y, true);
    RotateWorld(tmp);
}

void Camera::Roll(const float angle)
{
    static math::Vec3f z(0.0f, 0.0f, 1.0f);
    
    math::Quatf tmp(angle, z, true);
    RotateWorld(tmp);
}

void Camera::RotateWorld(const math::Quatf &q)
{
    m_orientation = q * m_orientation;
    m_orientation.Normalize();
    UpdateViewMatrix();
}

void Camera::RotateLocal(const math::Quatf &q)
{
    m_orientation = m_orientation * q;
    m_orientation.Normalize();
    UpdateViewMatrix();
}

const math::Vec3f Camera::GetForwardVector() const
{
    return math::Vec3f(-m_viewMatrix(2, 0), -m_viewMatrix(2, 1), -m_viewMatrix(2, 2));
}

const math::Vec3f Camera::GetUpVector() const
{
    return math::Vec3f(m_viewMatrix(1, 0), m_viewMatrix(1, 1), m_viewMatrix(1, 2));
}

const math::Vec3f Camera::GetRightVector() const
{
    return math::Vec3f(m_viewMatrix(0, 0), m_viewMatrix(0, 1), m_viewMatrix(0, 2));
}

void Camera::GetModelViewMatrix(math::Mat4f &matrix) const
{
    matrix = m_viewMatrix;
}

void Camera::SetFOV(const float fov)
{
	m_fieldOfView = fov;
}
    
void Camera::SetAspectRatio(const float ratio)
{
	m_aspectRatio = ratio;
}
    
void Camera::SetFocalDistance(const float distance)
{
	m_focalDistance = distance;
}
    
void Camera::SetApertureRadius(const float radius)
{
	m_apertureRadius = radius;
}
    
float Camera::GetFOV() const
{
	return m_fieldOfView;
}

float Camera::GetAspectRatio() const
{
	return m_aspectRatio;
}
    
float Camera::GetFocalDistance() const
{
	return m_focalDistance;
}

float Camera::GetApertureRadius() const
{
	return m_apertureRadius;
}

void Camera::UpdateViewMatrix()
{
    math::Quatf tmp = m_orientation.Inverse();
    tmp.ToMatrix(m_viewMatrix);
    UpdateMatrixPosition();
}

void Camera::UpdateMatrixPosition()
{
    // Set the translation part of the matrix.
    math::Vec3f back  = GetForwardVector() * -1.0f;
    math::Vec3f up    = GetUpVector();
    math::Vec3f right = GetRightVector();
    
    m_viewMatrix(0, 3) = -right.Dot(m_position);
    m_viewMatrix(1, 3) = -up.Dot(m_position);
    m_viewMatrix(2, 3) = -back.Dot(m_position);
}
    
} // namespace gfx
