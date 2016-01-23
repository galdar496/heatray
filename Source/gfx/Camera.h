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
    
/// Camera class which uses quaternions to represent rotations.
class Camera
{
    public:
    
        /// Default constructor.
        Camera();
    
        /// Destructor.
        ~Camera();
    
        /// Set the position of the camera.
        void setPosition(const math::Vec3f &position // IN: Position to set the camera to (in world space).
                        );
    
    	/// Explicitly set the orientation of the camera.
    	void setOrientation(const math::Quatf &orientation // IN: Orientation to force the camera to.
                           );
    
        /// Get the current position of the camera in world space.
        const math::Vec3f getPosition() const;
    
    	/// Get the current orientation of the camera (as a quaternion).
        math::Quatf getOrientation() const;
    
        /// Move the camera along it local axis.
        void move(const float x_distance,  // IN: Distance to move the camera in the x direction.
                  const float y_distance,  // IN: Distance to move the camera in the y direction.
                  const float z_distance   // IN: Distance to move the camera in the z direction.
                 );
    
        /// Move the camera along its right vector (x-axis).
        void strafe(const float x_distance // IN: Distance to move the camera in the x direction.
                   );
    
        /// Rotate about the x axis.
        void pitch(const float angle // IN: Angle of rotation about the x axis (radians).
                  );
    
        /// Rotate about the y axis.
        void yaw(const float angle // IN: Angle of rotation about the y axis (radians).
                );
    
        /// Rotate about the z axis. value is in radians.
        void roll(const float angle // IN: Angle of rotation about the z axis (radians).
                 );
    
        /// Rotate about an arbitrary world-space axis (Can contain mutliple rotations).
        void rotateWorld(const math::Quatf &q // IN: Quaternion to apply to the current orientation that rotates about the fixed-world axis.
                        );
    
        /// Rotate about an arbitrary object-space axis (Can contain mutliple rotations).
        void rotateLocal(const math::Quatf &q // IN: Quaternion to apply to the current rotation that rotates about the orientation's local axis.
                        );
    
        /// Get the normalized forward vector from the current view.
        const math::Vec3f getForwardVector() const;
    
        /// Get the normalized up vector from the current view.
        const math::Vec3f getUpVector() const;
    
        /// Get the normalized right vector from the current view.
        const math::Vec3f getRightVector() const;
    
        /// Get the current model view matrix.
        math::Mat4f getModelViewMatrix() const;
    
        /// Set the field of view (y) of the camera.
        void setFOV(const float fov // IN: Field of view in the y direction to set the camera (degrees).
                   );
    
    	/// Set the aspect ratio of the camera.
    	void setAspectRatio(const float ratio // IN: Aspect ratio.
                           );
    
        /// Set the focal length of the camera.
        void setFocalLength(const float length // IN: Focal length to use for the camera.
        				   );
        
        /// Set the radius of the aperture of the camera.
        void setApertureRadius(const float radius // IN: Radius to use for the camera.
        					  );
    
    	/// Get the field of view of the camera in degrees.
    	float getFOV() const;
    
    	/// Get the aspect ratio of the camera.
    	float getAspectRatio() const;
    
    	/// Get the focal length of the camera.
    	float getFocalLength() const;
    
    	/// Get the radius of the aperture of the camera.
    	float getApertureRadius() const;
    
    private:
    
        /// Update the internally stored view matrix.
        void updateViewMatrix();
    
        /// Update position of the camera.
        void updateMatrixPosition();
        
        // Member variables.
        math::Vec3f	m_position;	 			// Current position of the camera.
        math::Quatf m_orientation;	 		// Current orientation of the camera.
        math::Mat4f m_view_matrix;	  		// Stored modelview matrix for the current orientation of the camera.
        
        float m_field_of_view;   // Field of view of the camera (degrees).
    	float m_aspect_ratio;    // Aspect ratio of the camera.
    	float m_focal_length;    // Focal length of the camera lense.
    	float m_aperture_radius; // Radius of the aperture.
};
    
} // namespace gfx

