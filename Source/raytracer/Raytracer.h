//
//  Raytracer.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include "../gfx/Camera.h"
#include "../gfx/Program.h"
#include "../gfx/Shader.h"
#include "../gfx/Mesh.h"
#include "../gfx/Texture.h"
#include "../gfx/Buffer.h"
#include "../utility/tinyxml2.h"
#include "Light.h"
#include "Pixels.h"
#include <OpenRL/OpenRL.h>
#include <bitset>

#define MAX_LIGHTS 5

///
/// Class to control raytracing via OpenRL. All rendering setup
/// and execution is performed via the public interface to
/// this class.
///

class Raytracer
{
    public:
    
    	Raytracer();
    	~Raytracer();
    
        ///
    	/// Initialize the raytracer for use.
        ///
        /// @param configFilePath Relative path to the xml configuration file to configure the render.
        ///
        /// @return If true, the raytracer is successfully initialized and ready for rendering.
        ///
        bool Initialize(const std::string &configFilePath);
    
        ///
    	/// Deallocate any internal data and
    	/// prepare for shutdown.
        ///
    	void Destroy();
    
        ///
    	/// Update the renderer. During the update phase the raytracer will test for various user
        /// input and update the raytracing environment accordingly.
        ///
        /// @param dt Change in time since the last call to Update() (in seconds).
        ///
    	void Update(const float dt);
    
        ///
    	/// Render a frame. Each render accumulates the new pass data over the existing render data. Therefore every pass
        /// is the accumulation of EVERY pass that has come before it. To properly display this data, it should be divided
        /// by the number of passes that have been performed so far (to get a proper color average for each pixel).
        ///
        /// @param outputPixels Rendered pixels from EVERY pass, should be averaged for proper viewing.
        ///
    	void Render(Pixels &outputPixels);
    
        ///
    	/// Resize the raytracer output.
        ///
        /// @param width New width of the raytraced output buffer (in pixels).
        /// @param height New height of the raytraced output buffer (in pixels).
        ///
    	void Resize(RLint width, RLint height);
    
        ///
        /// Get a reference to the keyboard keys.
        ///
        std::bitset<256> &GetKeys();

        ///
        /// If true, this key will be reset by the raytracer itself and should not be reset
        /// by the windowing system.
        ///
        bool IsSpecialKey(const char key) const;

        ///
        /// Get the number of passes performed so far.
        ///
        int GetNumPassesPerformed() const;
    
        ///
        /// Get the dimensions of the raytracer's output buffer (in screen pixels).
        ///
        void GetDimensions(RLint &width, RLint &height);

        ///
        /// Get pixel divisor for proper display of the pixels. This divisor will take into
        /// account any exposure changes that the user has requested.
        ///
        float GetPixelDivisor() const;
    
    private:
    
    	// This class is not copyable.
    	Raytracer(const Raytracer &other) = delete;
    	Raytracer & operator=(const Raytracer &other) = delete;
    
        ///
        /// Check the current state of the keyboard.
        ///
        /// @param dt Change in time since the last frame (in seconds).
        ///
        void CheckKeys(const float dt);
    
        //
    	/// Reset the rendering state. This is done if the camera moved or the window got resized
    	/// and rendering needs to start over from the beginning.
        ///
    	void ResetRenderingState();
    
        ///
    	/// Load the model specified in the config file and all materials associated with it. After this is finished,
        /// a mesh will be ready for rendering.
        ///
        /// @param meshNode XML element which has the mesh attributes to load (<Mesh>).
        ///
    	void LoadModel(const tinyxml2::XMLElement *meshNode);
    
        ///
    	/// Setup the camera based on the defaults specified in the config file.
        ///
        /// @param cameraNode XML node which specifies the camera propertices (<Camera>).
        ///
    	void SetupCamera(const tinyxml2::XMLNode *cameraNode);
    
        ///
    	/// Setup the default framebuffer to perform all rendering into.
        ///
        /// @param framebufferNode XML node which specifies the framebuffer size (<Framebuffer>).
        ///
    	void SetupFramebuffer(const tinyxml2::XMLElement *framebuffer_node);
    
        ///
        /// Setup general rendering settings independent of any object contained within this class.
        ///
        /// @param renderSettingsNode XML node which specifies the render settings (<RenderSettings>).
        ///
        void SetupRenderSettings(const tinyxml2::XMLElement *render_settings_node);
    
        ///
        /// Get the light information from the mesh and populate the m_lights member variable.
        ///
        /// @param mesh Mesh to read the light from.
        ///
        void GetLighting(gfx::Mesh &mesh);
    
        ///
    	/// Write a configuration file with the current rendering settings which can be reloaded with a new instance
    	/// of Heatray.
        ///
    	void WriteConfigFile() const;
    
    	std::vector<Light> m_lights; ///< Lighting info. Read from the mesh file. At most MAX_LIGHTS lights are supported.
    
        ///
        /// Structure which represents the uniform block in all shaders for lighting information.
        ///
        struct LightUniformBuffer
        {
            LightUniformBuffer() :
            	count(0),
                primitive{RL_NULL_PRIMITIVE}
            {
            }
            
            math::Vec3f position[MAX_LIGHTS];
            math::Vec3f normal[MAX_LIGHTS];

            int count;
            RLprimitive primitive[MAX_LIGHTS];
        };
    
        /// Structure which represents the uniform block in diffuse shaders for global illumination (GI).
        struct GIUniformBuffer
        {
            RLtexture texture; // Full of random values for samplinging the hemisphere on a diffuse bounce.
            int enabled;       // Enable/disable global illumination.
        };
    
        gfx::Texture m_randomValuesTexture; ///< Texture to use in the RandomUniformBuffer uniform block.
    
    	gfx::Camera m_camera;        ///< Camera which controls the raytracing view.
    	float m_cameraMovementSpeed; ///< Speed at which to move the camera.
    	float m_cameraRotationSpeed; ///< Speed at which to rotate the camera.
    
    	gfx::Texture m_apertureSampleTexture; ///< Texture which has random values within the current radius of the aperture for depth of field.
    
        ///
        /// Uniform locations for the uniforms that control the main frame shader.
        ///
        struct FrameProgramUniforms
        {
            RLint cameraPosition;
            RLint forward;
            RLint up;
            RLint right;
            RLint fovTan;
            RLint focalLength;
            RLint aspectRatio;
            RLint jitterTexture;
            RLint apertureSampleTexture;
            RLint randomTextureMatrix;
        };
    
        FrameProgramUniforms m_frameUniforms;    ///< Frame program uniform locations. Cached so that they're not read every frame.
    	gfx::Program m_raytracingFrameProgram;   ///< RLSL program which generates the primary rays to start raytracing.
    	gfx::Shader m_vertexShader;              ///< Vertex shader to use for all ray shaders.
   
        gfx::Mesh m_mesh; ///< Mesh to use for rendering.
    
        std::bitset<256> m_keyboard; ///< Current state of the keyboard keys.
    
    	RLframebuffer m_fbo;                   ///< FBO to use for raytrace rendering.
		gfx::Texture  m_fboTexture;            ///< Texture to use for the FBO.
        gfx::Texture  m_jitterTexture;         ///< Texture storing jitter values.
        int           m_passesPerformed;       ///< Number of passes rendered so far.
    
        gfx::Buffer   m_lightBuffer;  ///< Current rendering position of the light as a shared buffer between all shaders.
        gfx::Buffer   m_giBuffer;     ///< Buffer filled with random values in a texture (used for path tracing) and an enable flag.
    
    	bool m_saveImage; ///< If true, write the next rendered image to an output file.
    
        int m_totalPassCount; ///< Total number of render passes to perform.
    	int m_maxRayDepth;    ///< Maximum depth that any ray can have in the system.
    
        OpenRLContext m_rlContext; ///< Main context for OpenRL.

        float m_exposureCompensation; ///< Exposure compensation to apply to the rendered pixels.
};
