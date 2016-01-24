//
//  Camera.h
//  Heatray
//
//  Created by Cody White on 5/12/14.
//
//

#pragma once

#include "../utility/util.h"
#include "../math/Vector.h"
#include "../math/Quaternion.h"
#include "../math/Matrix.h"

namespace gfx
{
    
///
/// Camera class which uses a quaternion to represent rotation.
///

class Camera
{
    public:
    
        Camera();
        ~Camera();
    
        ///
        /// Set the position of the camera.
        ///
        /// @param position Position to set the camera to (in world space).
        ///
        void SetPosition(const math::Vec3f &position);
    
        ///
    	/// Explicitly set the orientation of the camera.
        ///
        /// @param orientation Orientation to force the camera to.
        ///
    	void SetOrientation(const math::Quatf &orientation);
    
        ///
        /// Get the current position of the camera in world space.
        ///
        const math::Vec3f GetPosition() const;
    
        ///
    	/// Get the current orientation of the camera (as a quaternion).
        ///
        math::Quatf GetOrientation() const;
    
        ///
        /// Move the camera along it local axis.
        ///
        /// @param xDistance Distance to move the camera in the x direction.
        /// @param yDistance Distance to move the camera in the y direction.
        /// @param zDistance Distance to move the camera in the z direction.
        ///
        void Move(const float x_distance, const float y_distance, const float z_distance);
    
        ///
        /// Rotate about the x axis.
        ///
        /// @param angle Angle of rotation about the x axis (radians).
        ///
        void Pitch(const float angle);
    
        ///
        /// Rotate about the y axis.
        ///
        /// @param angle Angle of rotation about the y axis (radians).
        ///
        void Yaw(const float angle);
    
        ///
        /// Rotate about the z axis.
        ///
        /// @param angle Angle of rotation abou the z axis (radians).
        ///
        void Roll(const float angle);
    
        ///
        /// Rotate about an arbitrary world-space axis (Can contain mutliple rotations).
        ///
        /// @param q Quaternion to apply to the current orientation that rotates about the fixed-world axis.
        ///
        void RotateWorld(const math::Quatf &q);
    
        ///
        /// Rotate about an arbitrary object-space axis (Can contain mutliple rotations).
        ///
        /// @param q Quaternion to apply to the current rotation that rotates about the orientation's local axis.
        ///
        void RotateLocal(const math::Quatf &q);
    
        ///
        /// Get the normalized forward vector from the current view.
        ///
        const math::Vec3f GetForwardVector() const;
    
        ///
        /// Get the normalized up vector from the current view.
        ///
        const math::Vec3f GetUpVector() const;
    
        ///
        /// Get the normalized right vector from the current view.
        ///
        const math::Vec3f GetRightVector() const;
    
        ///
        /// Get the current model view matrix. This matrix is in colum-major
        /// format.
        ///
        /// @param matrix Matrix to populate with the camera's modelview matrix.
        ///
        void GetModelViewMatrix(math::Mat4f &matrix) const;
    
        ///
        /// Set the field of view (y) of the camera.
        ///
        /// @param fov Field of view in the y direction to set the camera (degrees).
        ///
        void SetFOV(const float fov);
    
        ///
    	/// Set the aspect ratio of the camera.
        ///
        /// @param ratio Aspect ratio, typically screen width / screen height.
        ///
    	void SetAspectRatio(const float ratio);
    
        ///
        /// Set the focal length of the camera.
        ///
        void SetFocalLength(const float length);
    
        ///
        /// Set the radius of the aperture of the camera.
        ///
        void SetApertureRadius(const float radius);
    
        ///
    	/// Get the field of view of the camera in degrees.
        ///
    	float GetFOV() const;
    
        ///
    	/// Get the aspect ratio of the camera.
        ///
    	float GetAspectRatio() const;
    
        ///
    	/// Get the focal length of the camera.
        ///
    	float GetFocalLength() const;
    
        ///
    	/// Get the radius of the aperture of the camera.
        ///
    	float GetApertureRadius() const;
    
    private:
    
        ///
        /// Update the internally stored view matrix.
        ///
        void UpdateViewMatrix();
    
        ///
        /// Update position of the camera.
        ///
        void UpdateMatrixPosition();
        
        // Member variables.
        math::Vec3f	m_position;	 			///< Current position of the camera.
        math::Quatf m_orientation;	 		///< Current orientation of the camera.
        math::Mat4f m_viewMatrix;	  		///< Stored modelview matrix for the current orientation of the camera.
        
        float m_fieldOfView;    ///< Field of view of the camera (degrees).
    	float m_aspectRatio;    ///< Aspect ratio of the camera.
    	float m_focalLength;    ///< Focal length of the camera lense.
    	float m_apertureRadius; ///< Radius of the aperture.
};
    
} // namespace gfx

