//
//  World.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include <OpenRL/OpenRL.h>
#include "../gfx/Camera.h"
#include "../gfx/Program.h"
#include "../gfx/Shader.h"
#include "../gfx/Mesh.h"
#include "../gfx/Texture.h"
#include "../gfx/Buffer.h"
#include "../utility/tinyxml2.h"
#include "Light.h"
#include "Pixels.h"
#include <bitset>

#define MAX_LIGHTS 5

/// Class to control raytracing via OpenRL. All rendering setup
/// and execution is performed via the public interface to
/// this class.
class World
{
    public:
    
    	/// Default constructor.
    	World();
    
    	/// Destructor.
    	~World();
    
    	/// Initialize the world for use. Returns true if
    	/// initialization was successful.
    	bool initialize(const std::string &config_file_path, //  IN: Path to the xml configuration file to configure the render.
                        RLint             &screen_width,     // OUT: Initial width to set the screen to.
                        RLint             &screen_height     // OUT: Initial height to set the screen to.
                       );
    
    	/// Deallocate any internal data and
    	/// prepare for shutdown.
    	void destroy();
    
    	/// Update this class.
    	void update(const float dt // IN: Change in time since the last call to update() (in seconds).
                   );
    
    	/// Render a frame.
    	void render(Pixels &outputPixels // OUT: Rendered pixels from EVERY pass, should be averaged for proper viewing.
                   );
    
    	/// Resize the render viewport.
    	void resize(RLint width, // IN: Width of the screen in pixels.
                    RLint height // IN: Height of the screen in pixels.
                   );
    
        /// Get a reference to the keyboard keys.
        std::bitset<256> &getKeys();

        /// If true, this key will be reset by the raytracer itself and should not be reset
        /// by the windowing system.
        bool isSpecialKey(const char key) const;

        /// Get the number of passes performed so far.
        int getNumPassesPerformed() const;
    
    private:
    
    	// This class is not copyable.
    	World(const World &other) = delete;
    	World & operator=(const World &other) = delete;
    
        /// Check the current state of the keyboard.
        void checkKeys(const float dt // IN: Change in time since the last frame (in seconds).
                      );
    
    	/// Reset the rendering state. This is done if the camera moved or the window got resized
    	/// and rendering needs to start over from the beginning.
    	void resetRenderingState();
    
    	/// Load the model specified in the config file and all materials associated with it. After this is finished,
        /// a mesh will be ready for rendering.
    	void loadModel(const tinyxml2::XMLElement *mesh_node // IN: XML element which has the mesh attributes to load (<Mesh>).
                      );
    
    	/// Setup the camera based on the defaults specified in the config file.
    	void setupCamera(const tinyxml2::XMLNode *camera_node // IN: XML node which specifies the camera properties (<Camera>).
                        );
    
    	/// Setup the default framebuffer to perform all rendering into.
    	void setupFramebuffer(const tinyxml2::XMLElement *framebuffer_node,  //  IN: XML node which specifies the framebuffer size (<Framebuffer>).
    						  RLint                      &framebuffer_width, // OUT: Width that the framebuffer is configured to use. Read from the config file.
    						  RLint                      &framebuffer_height // OUT: Height that the framebuffer is configured to use. Read from the config file.
    						);
    
        /// Setup general rendering settings independent of any object contained within this class.
        void setupRenderSettings(const tinyxml2::XMLElement *render_settings_node // IN: XML node which specifies the render settings (<RenderSettings>).
                                );
    
        /// Get the light information from the mesh and populate the m_light member variable.
        void getLighting(gfx::Mesh &mesh  //  IN: Mesh to read the light from.
                        );
    
    	/// Write a configuration file with the current rendering settings which can be reloaded with a new instance
    	/// of Heatray.
    	void writeConfigFile() const;
    
    	std::vector<Light> m_lights; // Lighting info. Read from the mesh file. At most 5 lights are supported.
    
        /// Structure which represents the uniform block in all shaders for lighting information.
        struct LightUniformBuffer
        {
            LightUniformBuffer() :
            	count(0),
                primitives{RL_NULL_PRIMITIVE}
            {
            }
            
            int count;
            RLprimitive primitives[MAX_LIGHTS];
            math::vec3f positions[MAX_LIGHTS];
            math::vec3f normals[MAX_LIGHTS];
        };
    
        /// Structure which represents the uniform block in diffuse shaders for global illumination (GI).
        struct GIUniformBuffer
        {
            RLtexture texture; // Full of random values for samplinging the hemisphere on a diffuse bounce.
            int enabled;       // Enable/disable global illumination.
        };
    
        gfx::Texture m_random_values_texture; // Texture to use in the RandomUniformBuffer uniform block.
    
    	gfx::Camera m_camera;          // Camera which controls the raytracing view.
    	float m_camera_movement_speed; // Speed at which to move the camera.
    	float m_camera_rotation_speed; // Speed at which to rotate the camera.
    
    	gfx::Texture m_aperture_sample_texture; // Texture which has random values within the current radius of the aperture for depth of field.
    
    	gfx::Program m_raytracing_frame_program; // RLSL program which generates the primary rays to start raytracing.
    	gfx::Shader m_vertex_shader;             // Vertex shader to use for all ray shader.
   
        gfx::Mesh m_mesh; // Mesh to use for rendering.
    
        std::bitset<256> m_keyboard; // Current state of the keyboard keys.
    
    	RLframebuffer m_fbo;                    // FBO to use for raytrace rendering.
		gfx::Texture  m_fbo_texture;            // Texture to use for the FBO.
        gfx::Texture  m_jitter_texture;         // Texture storing jitter values.
        int           m_passes_performed;       // Number of passes rendered so far.
    
        gfx::Buffer   m_light_buffer;  // Current rendering position of the light as a shared buffer between all shaders.
        gfx::Buffer   m_gi_buffer;     // Buffer filled with random values in a texture (used for path tracing) and an enable flag.
    
    	bool m_save_image; // If true, write the next rendered image to an output file.
    
        int m_total_pass_count; // Total number of render passes to perform.
    	int m_max_ray_depth;    // Maximum depth that any ray can have in the system.
    
        OpenRLContext m_rl_context; // Main context for OpenRL.
};
