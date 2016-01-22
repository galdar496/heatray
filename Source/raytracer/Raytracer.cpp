//
//  Raytracer.cpp
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#include "Raytracer.h"
#include "ShaderGenerator.h"
#include "../utility/util.h"
#include "../utility/rng.h"
#include "../utility/Timer.h"
#include "../gfx/Shader.h"
#include "../math/Constants.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <functional>

#define GLUT_KEY_ENTER 13
#define GLUT_KEY_SPACE 32

// Keys supported by Heatray
namespace Keys
{
    enum
    {
        kCameraForward      = 'w',
        kCameraBackward     = 's',
        kCameraPanLeft      = 'a',
        kCameraPanRight     = 'd',
        kCameraRotateUp     = 'r',
        kCameraRotateDown   = 'f',
        kCameraRotateLeft   = 'q',
        kCameraRotateRight  = 'e',
        kCameraRollLeft     = 'z',
        kCameraRollRight    = 'c',

        kEnableGI   = 'g',
        kScreenshot = GLUT_KEY_ENTER,
        kSaveConfig = GLUT_KEY_SPACE,

        kIncreaseFocalLength   = 'p',
        kDecreaseFocalLength   = 'o',
        kIncreaseApertureWidth = ']',
        kDecreaseApertureWidth = '['
    };
} // namespace Keys

/// Default constructor.
Raytracer::Raytracer() :
	m_camera_movement_speed(5.5f),
	m_camera_rotation_speed(0.2f),
	m_fbo(RL_NULL_FRAMEBUFFER),
    m_passes_performed(0),
	m_save_image(false),
    m_total_pass_count(1024),
    m_max_ray_depth(5)
{
    m_keyboard.reset();
}

/// Destructor.
Raytracer::~Raytracer()
{
}

/// Initialize the raytracer for use. Returns true if
/// initialization was successful.
bool Raytracer::initialize(const std::string &config_file_path, RLint &screen_width, RLint &screen_height)
{
    // Set all sampling textures to use linear filtering.
    gfx::Texture::Params texture_params;
    texture_params.min_filter = RL_LINEAR;
    
    tinyxml2::XMLDocument config_file;
    const tinyxml2::XMLElement *root_config_node;
    if (config_file.LoadFile(config_file_path.c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cout << "Unable to load configuration file " << config_file_path << std::endl;
        return false;
    }
    
    root_config_node = config_file.FirstChildElement("HeatRayConfig");
    
	// Spawn a thread to load the mesh. This just puts the mesh data into memory, still have to create
    // VBOs out of the data later.
    std::thread mesh_thread(&Raytracer::loadModel, this, root_config_node->FirstChildElement("Mesh"));
    
    // Construct the OpenRL context.
//    OpenRLContextAttribute attributes[] = {kOpenRL_EnableRayPrefixShaders, 1, NULL};
    m_rl_context = OpenRLCreateContext(NULL, NULL, NULL);
    OpenRLSetCurrentContext(m_rl_context);
    
    // Setup configurable objects.
    setupFramebuffer(root_config_node->FirstChildElement("Framebuffer"), screen_width, screen_height);
    setupCamera(root_config_node->FirstChildElement("Camera"));
    setupRenderSettings(root_config_node->FirstChildElement("RenderSettings"));

    // Setup the frameshader to generate the primary rays and bind it to the frame primitive.
    {
        bool error = false;
		error  = m_raytracing_frame_program.addShader("Resources/shaders/perspective.frame", gfx::Shader::FRAME);
        error |= m_raytracing_frame_program.link();
        
        if (!error)
        {
            return false;
        }
        
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);
		m_raytracing_frame_program.bind();
    }
    
    // Setup the vertex shader to use for all ray shaders.
    {
        bool error = m_vertex_shader.load("Resources/shaders/passthrough.vert", gfx::Shader::VERTEX);
        if (!error)
        {
            return false;
        }
    }
    
    {
        // Create lighting dummy uniform block in order to setup the bindings for shader generation.
        LightUniformBuffer dummy_block;
        m_light_buffer.setTarget(RL_UNIFORM_BLOCK_BUFFER);
        m_light_buffer.load(&dummy_block, sizeof(LightUniformBuffer), "Light buffer");
        
        // Wait for the mesh to finish loading before we continue.
        mesh_thread.join();
        
        // Get the lighting information from the mesh and then create the mesh rendering info.
        getLighting(m_mesh);
        m_mesh.createRenderData();
        
        {
            // Read the paths to the shader files from the config file.
            std::string ray_shader_path;
            std::string light_shader_path;

            const tinyxml2::XMLElement *shader_node = root_config_node->FirstChildElement("Shader");
            ray_shader_path = shader_node->FindAttribute("Ray")->Value();
            light_shader_path = shader_node->FindAttribute("Light")->Value();

            ShaderGenerator generator;
            if (!generator.generateShaders(m_mesh, m_vertex_shader, m_light_buffer, m_gi_buffer, ray_shader_path, light_shader_path, MAX_LIGHTS, m_lights))
            {
                return false;
            }
        }
        
        // Create the uniform block buffer which will store the current light subregion to sample. Every ray shader
        // shares this same buffer so we only need to update it once.
        m_light_buffer.bind();
        LightUniformBuffer *block = m_light_buffer.mapBuffer<LightUniformBuffer>();
        block->count = static_cast<int>(m_lights.size());
        for (int ii = 0; ii < block->count; ++ii)
        {
            block->primitive[ii] = m_lights[ii].primitive;
        }
        m_light_buffer.unmapBuffer();
        m_light_buffer.unbind();
        
        m_mesh.clearLoadedData();
    }

    return true;
}

/// Deallocate any internal data and
/// prepare for shutdown.
void Raytracer::destroy()
{
    if (m_fbo != RL_NULL_FRAMEBUFFER)
    {
        rlDeleteFramebuffers(1, &m_fbo);
        m_fbo = RL_NULL_FRAMEBUFFER;
    }
    
    m_mesh.destroy();
    m_random_values_texture.destroy();
    m_aperture_sample_texture.destroy();
    m_raytracing_frame_program.destroy();
    m_vertex_shader.destroy();
    m_fbo_texture.destroy();
    m_jitter_texture.destroy();
    m_light_buffer.destroy();
    m_gi_buffer.destroy();
    
    OpenRLDestroyContext(m_rl_context);
}

/// Update this class.
void Raytracer::update(const float dt)
{
    checkKeys(dt);
}

/// Render a frame.
void Raytracer::render(Pixels &outputPixels)
{
    // Only render if we actually need to, otherwise the finished render will just be displayed by GLUT.
    if (m_passes_performed <= m_total_pass_count)
    {
		// Update the light position for soft shadowing.
        {
            // Update the lighting uniform block to use the next position for sampling.
            m_light_buffer.bind();
            LightUniformBuffer *block = m_light_buffer.mapBuffer<LightUniformBuffer>();
            for (size_t ii = 0; ii < m_lights.size(); ++ii)
            {
                block->position[ii] = m_lights[ii].sample_positions[m_passes_performed - 1]; // -1 because m_passes_performed does not start at 0.
                block->normal[ii]   = m_lights[ii].sample_normals[m_passes_performed - 1];
            }
            m_light_buffer.unmapBuffer();
            m_light_buffer.unbind();
        }

        // Generate a random texture matrix to use for indexing into the textures containing random values.
        // Every frame this matrix will be randomly generated to ensure that random values are sampled
        // in the shaders.
        math::Mat4f random_texture_matrix = math::Mat4f::identity();
        {
            math::Mat4f rotation;

            // Rotate about the y axis.
            math::quatf y_rotate(util::Random(-math::PI, math::PI), math::vec3f(0.0f, 1.0f, 0.0f), true);
            y_rotate.toMatrix(rotation);
            random_texture_matrix *= rotation;
            
            // Rotate about the x axis.
            math::quatf x_rotate(util::Random(0.0f, math::TWO_PI), math::vec3f(1.0f, 0.0f, 0.0f), true);
            x_rotate.toMatrix(rotation);
            random_texture_matrix *= rotation;
            
            // Now randomly scale the matrix.
            const float max_scale = 5.0f;
            math::Mat4f random_scale = math::Mat4f::identity();
            random_scale(0, 0) = util::Random(0.0f, max_scale);
            random_scale(1, 1) = util::Random(0.0f, max_scale);
            random_scale(2, 2) = util::Random(0.0f, max_scale);

            random_texture_matrix *= random_scale;
        }
        
        rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo);
        // Setup the camera parameters to the frameshader.
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);
        m_raytracing_frame_program.bind();
        m_raytracing_frame_program.set3fv("cameraPosition", m_camera.getPosition().v);
        m_raytracing_frame_program.set3fv("forward", m_camera.getForwardVector().v);
        m_raytracing_frame_program.set3fv("up", m_camera.getUpVector().v);
        m_raytracing_frame_program.set3fv("right", m_camera.getRightVector().v);
        m_raytracing_frame_program.set1f("fovTan", tanf((math::DEGREE_TO_RADIAN * m_camera.getFOV()) * 0.5f));
        m_raytracing_frame_program.set1f("focalLength", m_camera.getFocalLength());
        m_raytracing_frame_program.set1f("aspectRatio", m_camera.getAspectRatio());
        m_raytracing_frame_program.setTexture("jitterTexture", m_jitter_texture.getTexture());
        m_raytracing_frame_program.setTexture("apertureSampleTexture", m_aperture_sample_texture.getTexture());
        m_raytracing_frame_program.setMatrix4fv("randomTextureMatrix", random_texture_matrix.v);
        rlRenderFrame();

        ++m_passes_performed;
    }
    
    if (m_save_image)
    {
        // Write the rendered image to an output file.
		std::vector<float> pixels;
        pixels.resize(m_fbo_texture.width() * m_fbo_texture.height() * Pixels::NUM_PIXEL_CHANNELS);
        rlBindTexture(RL_TEXTURE_2D, m_fbo_texture.getTexture());
        rlBindBuffer(RL_PIXEL_PACK_BUFFER, RL_NULL_BUFFER); // Make sure no pixel-pack buffer is bound, we want to copy into 'pixels'.
        rlGetTexImage(RL_TEXTURE_2D, 0, RL_RGB, RL_FLOAT, &pixels[0]);
        util::WriteImage("out.tiff", m_fbo_texture.width(), m_fbo_texture.height(), Pixels::NUM_PIXEL_CHANNELS, &pixels[0], static_cast<float>(m_passes_performed));
        
        m_save_image = false;
    }

    // Map the rendered texture to a pixelpack buffer so that the calling function can properly display it.
    outputPixels.setData(m_fbo_texture);
    
    util::CheckRLErrors();
}

/// Resize the render viewport.
void Raytracer::resize(RLint width, RLint height)
{
    if (height == 0)
    {
        height = 1;
    }
    
    // Set the new viewport.
	rlViewport(0, 0, width, height);
    m_camera.setAspectRatio((float)((float)width / (float)height));
    
    // Regenerate the render textures to be the proper size.
    if (m_fbo_texture.isValid())
    {
        m_fbo_texture.resize(width, height);

        // Generate a random texture to use for pixel offsets while rendering. This texture has components which are uniformly
        // distrubuted over a circle. This allows for a radial filter during anti-aliasing.
        m_jitter_texture.randomizeRadial(m_fbo_texture.width(), m_fbo_texture.height(), RL_FLOAT, 1.2f, "random");
        resetRenderingState();
    }
}

/// Get a reference to the keyboard keys.
std::bitset<256> &Raytracer::getKeys()
{
    return m_keyboard;
}

/// If true, this key will be reset by the raytracer itself and should not be reset
/// by the windowing system.
bool Raytracer::isSpecialKey(const char key) const
{
    return ((key == Keys::kEnableGI)   ||
            (key == Keys::kScreenshot) ||
            (key == Keys::kSaveConfig)
           );
}

/// Get the number of passes performed so far.
int Raytracer::getNumPassesPerformed() const
{
    return m_passes_performed;
}

/// Check the current state of the keyboard.
void Raytracer::checkKeys(const float dt)
{
    static util::Timer currentTime(true);

    // Ignore non-camera movement keypresses for this much time to account for 
    // the key being held accidentally.
    const float time_delta = 0.5f;

    float aperature_increment = 0.1f;
    float focal_length_increment = 1.0f;
    
    bool reset_rendering_state = false;
    
    if (m_keyboard.test(Keys::kCameraForward))
    {
        m_camera.move(0.0f, 0.0f, -m_camera_movement_speed * dt);
        reset_rendering_state = true;
    }
    else if (m_keyboard.test(Keys::kCameraBackward))
    {
        m_camera.move(0.0f, 0.0f, m_camera_movement_speed * dt);
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test(Keys::kCameraPanLeft))
    {
        m_camera.move(-m_camera_movement_speed * dt, 0.0f, 0.0f);
        reset_rendering_state = true;
    }
    else if (m_keyboard.test(Keys::kCameraPanRight))
    {
        m_camera.move(m_camera_movement_speed * dt, 0.0f, 0.0f);
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test(Keys::kCameraRotateUp))
    {
        m_camera.pitch(-m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    else if (m_keyboard.test(Keys::kCameraRotateDown))
    {
        m_camera.pitch(m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test(Keys::kCameraRotateRight))
    {
        m_camera.yaw(m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    else if (m_keyboard.test(Keys::kCameraRotateLeft))
    {
        m_camera.yaw(-m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test(Keys::kCameraRollLeft))
    {
        m_camera.roll(-m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    else if (m_keyboard.test(Keys::kCameraRollRight))
    {
        m_camera.roll(m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test(Keys::kScreenshot) && currentTime.GetElapsedTime() > time_delta)
    {
        m_save_image = true;
        currentTime.Restart();

        // Reset this key manually as it won't be reset by the windowing system.
        m_keyboard.reset(Keys::kScreenshot);
    }
    else if (m_keyboard.test(Keys::kSaveConfig) && currentTime.GetElapsedTime() > time_delta)
    {
        // Write out a configuration file with the current rendering settings.
        writeConfigFile();
        currentTime.Restart();

        // Reset this key manually as it won't be reset by the windowing system.
        m_keyboard.reset(Keys::kSaveConfig);
    }
    
    if (m_keyboard.test(Keys::kIncreaseApertureWidth))
    {
        float aperture_width = m_camera.getApertureRadius();
        aperture_width += aperature_increment;
        reset_rendering_state = true;
        
        // Regenerate the aperture sampling texture to reflect the new aperture radius.
        m_aperture_sample_texture.randomizeRadial(m_fbo_texture.width(), m_fbo_texture.height(), RL_FLOAT, aperture_width, "random");
        
        m_camera.setApertureRadius(aperture_width);
    }
    else if (m_keyboard.test(Keys::kDecreaseApertureWidth))
    {
        float aperture_width = m_camera.getApertureRadius();
        aperture_width = std::max(0.0f, aperture_width - aperature_increment);
        reset_rendering_state = true;
        
        // Regenerate the aperture sampling texture to reflect the new aperture radius.
        m_aperture_sample_texture.randomizeRadial(m_fbo_texture.width(), m_fbo_texture.height(), RL_FLOAT, aperture_width, "random");
        
        m_camera.setApertureRadius(aperture_width);
    }
    
    if (m_keyboard.test(Keys::kIncreaseFocalLength))
    {
        m_camera.setFocalLength(m_camera.getFocalLength() + focal_length_increment);
        reset_rendering_state = true;
    }
	else if (m_keyboard.test(Keys::kDecreaseFocalLength))
    {
        m_camera.setFocalLength(std::max(0.0f, m_camera.getFocalLength() - focal_length_increment));
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test(Keys::kEnableGI) && currentTime.GetElapsedTime() > time_delta) // perform GI.
    {
        m_gi_buffer.bind();
		GIUniformBuffer *block = m_gi_buffer.mapBuffer<GIUniformBuffer>();
        block->enabled = (block->enabled + 1) & 1;
		m_gi_buffer.unmapBuffer();
        m_gi_buffer.unbind();
        reset_rendering_state = true;
        currentTime.Restart();

        // Reset this key manually as it won't be reset by the windowing system.
        m_keyboard.reset(Keys::kEnableGI);
    }
    
    if (reset_rendering_state)
    {
        resetRenderingState();
    }
}

/// Reset the rendering state. This is done if the camera moved or the window got resized
/// and rendering needs to start over from the beginning.
void Raytracer::resetRenderingState()
{
    if (m_fbo != RL_NULL_FRAMEBUFFER)
    {
        rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo);
        rlClear(RL_COLOR_BUFFER_BIT);
    }
    
    m_passes_performed = 1;
}

/// Load the model specified in the config file and all materials associated with it. After this is finished,
/// a mesh will be ready for rendering.
void Raytracer::loadModel(const tinyxml2::XMLElement *mesh_node)
{
    std::string model_path = mesh_node->Attribute("Model");
    float model_scaling = 1.0f;
    
    mesh_node->QueryFloatAttribute("Scale", &model_scaling);
    
    if (!m_mesh.load(model_path, false, model_scaling, false))
    {
        exit(0);
    }
}

/// Setup the camera based on the defaults specified in the config file.
void Raytracer::setupCamera(const tinyxml2::XMLNode *camera_node)
{
    const tinyxml2::XMLElement *element = NULL;
    math::vec3f tmp_vector;
    float       tmp_value = 0.0f;
    
    // Setup the camera's position.
    {
        element = camera_node->FirstChildElement("Position");
        
		element->QueryFloatAttribute("X", &tmp_vector[0]);
        element->QueryFloatAttribute("Y", &tmp_vector[1]);
        element->QueryFloatAttribute("Z", &tmp_vector[2]);

        m_camera.setPosition(tmp_vector);
    }
    
    // Setup the camera's lens attributes.
    {
        element = camera_node->FirstChildElement("Lens");
        
        element->QueryFloatAttribute("FocalLength", &tmp_value);
        m_camera.setFocalLength(tmp_value);
        
		element->QueryFloatAttribute("ApertureRadius", &tmp_value);
        m_camera.setApertureRadius(tmp_value);
        
        // Generate the aperture sampling texture to reflect the aperture radius.
        m_aperture_sample_texture.randomizeRadial(m_fbo_texture.width(), m_fbo_texture.height(), RL_FLOAT, m_camera.getApertureRadius(), "random");
    }
    
    // Setup the camera's orientation.
    {
        element = camera_node->FirstChildElement("Orientation");
        
        element->QueryFloatAttribute("X", &tmp_vector[0]);
        element->QueryFloatAttribute("Y", &tmp_vector[1]);
        element->QueryFloatAttribute("Z", &tmp_vector[2]);
        element->QueryFloatAttribute("Angle", &tmp_value);
        
        math::quatf orientation(tmp_value, tmp_vector);
        m_camera.setOrientation(orientation);
    }
    
    // Setup the camera's movement speed parameters.
    {
        element = camera_node->FirstChildElement("Speed");
        
        element->QueryFloatAttribute("Movement", &m_camera_movement_speed);
        element->QueryFloatAttribute("Rotation", &m_camera_rotation_speed);
    }
}

/// Setup the default framebuffer to perform all rendering into.
void Raytracer::setupFramebuffer(const tinyxml2::XMLElement *framebuffer_node, RLint &framebuffer_width, RLint &framebuffer_height)
{
    // Heatray renders every pass to the same framebuffer without ever clearing it. Therefore, the framebuffer created below will
    // contain the pixel information for every pass. This FBO must be processed by a shader program which divides each pixel by
    // the current number of passes ran, producing an image that is actually viewable.
    
    // Read the width and height from the config file.
    framebuffer_node->QueryIntAttribute("Width", &framebuffer_width);
    framebuffer_node->QueryIntAttribute("Height", &framebuffer_height);
    
    gfx::Texture::Params texture_params;
    texture_params.min_filter      = RL_LINEAR;
    texture_params.format          = RL_RGB;
    texture_params.internal_format = RL_RGB;
    
    // Create the default FBO for rendering into.
    rlGenFramebuffers(1, &m_fbo);
    rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo);
    m_fbo_texture.setParams(texture_params);
    m_fbo_texture.create(framebuffer_width, framebuffer_height, RL_FLOAT, NULL, "Default FBO Texture");
    rlFramebufferTexture2D(RL_FRAMEBUFFER, RL_COLOR_ATTACHMENT0, RL_TEXTURE_2D, m_fbo_texture.getTexture(), 0);
    
    util::CheckRLErrors("Raytracer::setupFramebuffer()", true);
}

/// Setup general rendering settings independent of any object contained within this class.
void Raytracer::setupRenderSettings(const tinyxml2::XMLElement *render_settings_node)
{
    // Create the random values uniform block data. These values will be used in all diffuse shaders to bounce rays
    // and achieve indirect illumination.
    m_random_values_texture.randomize(m_fbo_texture.width(), m_fbo_texture.height(), 4, RL_FLOAT, 0.0f, 1.0f, "Random 0-1 texture");
    GIUniformBuffer gi_uniform_buffer;
    gi_uniform_buffer.texture = m_random_values_texture.getTexture();
    render_settings_node->QueryIntAttribute("DefaultGIOn", &(gi_uniform_buffer.enabled));
    m_gi_buffer.setTarget(RL_UNIFORM_BLOCK_BUFFER);
    m_gi_buffer.load(&gi_uniform_buffer, sizeof(GIUniformBuffer), "Random buffer");
    
    // Setup the number of passes to perform per the config file.
    render_settings_node->QueryIntAttribute("RaysPerPixel", &m_total_pass_count);
    
    // Set the maximum number of bounces any ray in the system can have before being terminated.
    render_settings_node->QueryIntAttribute("MaxRayDepth", &m_max_ray_depth);
    rlFrameParameter1i(RL_FRAME_RAY_DEPTH_LIMIT, m_max_ray_depth);
}

/// Get the light information from the mesh and populate the m_light member variable.
void Raytracer::getLighting(gfx::Mesh &mesh)
{
    // Go over each mesh piece and look for anything that is a light. For all lights that are found, populate a
    // 'Light' struct with information about that light to use for later rendering.
    gfx::Mesh::MeshList &meshes = mesh.getMeshList();
    for (gfx::Mesh::MeshList::const_iterator iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        const gfx::Mesh::MeshPiece *piece = &(iter->second);
        if (piece->material.name.find("Light") != std::string::npos)
        {
            if (m_lights.size() >= MAX_LIGHTS)
            {
            	std::cout << "Too many lights in mesh file, only 5 are currently supported, exiting..." << std::endl;
                exit(0);
            }
            
            m_lights.push_back(Light());
            Light *light = &m_lights[m_lights.size() - 1];

            int num_light_triangles = static_cast<int>(piece->vertices.size()) / 3;

            // Generate the randomized light positions to sample for each pass. Every pass of the render will use one of these generated
            // light positions.
            light->sample_positions.resize(m_total_pass_count);
            light->sample_normals.resize(m_total_pass_count);
            std::vector<float> randomBarycentrics;
            util::GenerateRandomNumbers(0.0f, 1.0f, m_total_pass_count * 3, randomBarycentrics); // * 3 to account of 3 barycentrics per sample point.
            int barycentricIndex = 0;

            for (int ii = 0; ii < m_total_pass_count; ++ii)
            {
                // Find a random triangle to use for this sample.
                int triangle_index = util::Random(0, num_light_triangles - 1);

                // Generate 3 random barycentric coordinates.
                float gamma = randomBarycentrics[barycentricIndex++];
                float beta  = randomBarycentrics[barycentricIndex++];
                float alpha;
                if (gamma + beta > 1.0f)
                {
                    // Subtract off some random value from beta.
                    beta = std::max(0.0f, beta - randomBarycentrics[barycentricIndex++]);
                }

                alpha = 1.0f - (gamma + beta);

                // Use the barycentrics to generate a random position within the triangle.
                int vertex_index = triangle_index * 3;
                math::vec3f sample_point = (piece->vertices[vertex_index + 0] * gamma) +
                                           (piece->vertices[vertex_index + 1] * beta) +
                                           (piece->vertices[vertex_index + 2] * alpha);

                // Similarly set the normal for this sample point.
                math::vec3f sample_normal = (piece->normals[vertex_index + 0] * gamma) +
                                            (piece->normals[vertex_index + 1] * beta) +
                                            (piece->normals[vertex_index + 2] * alpha);
                sample_normal = math::normalize(sample_normal);

                light->sample_positions[ii] = sample_point;
                light->sample_normals[ii]   = sample_normal;
            }
        }
    }
    
    if (m_lights.empty())
    {
        std::cout << "No material contains the name \"Light\" in loaded mesh, exiting..." << std::endl;
        exit(0);
    }
}

/// Write a configuration file with the current rendering settings which can be reloaded with a new instance
/// of Heatray.
void Raytracer::writeConfigFile() const
{
	tinyxml2::XMLDocument config;
	tinyxml2::XMLElement *root_element = config.NewElement("HeatRayConfig");
    config.InsertFirstChild(root_element);
    
    // Write the framebuffer info.
	{
        tinyxml2::XMLElement *node = config.NewElement("Framebuffer");
        node->SetAttribute("Width", m_fbo_texture.width());
        node->SetAttribute("Height", m_fbo_texture.height());
        
        root_element->InsertEndChild(node);
    }
    
    // Write the mesh info.
    {
        tinyxml2::XMLElement *node = config.NewElement("Mesh");
        node->SetAttribute("Model", m_mesh.getName().c_str());
        node->SetAttribute("Scale", m_mesh.getScale());
        
        root_element->InsertEndChild(node);
    }
    
    // Write the camera info.
    {
        tinyxml2::XMLElement *camera_node = config.NewElement("Camera");
    
        tinyxml2::XMLElement *position = config.NewElement("Position");
        math::vec3f camera_position = m_camera.getPosition();
        position->SetAttribute("X", camera_position[0]);
        position->SetAttribute("Y", camera_position[1]);
        position->SetAttribute("Z", camera_position[2]);
        camera_node->InsertEndChild(position);
        
        tinyxml2::XMLElement *lens = config.NewElement("Lens");
        lens->SetAttribute("FocalLength", m_camera.getFocalLength());
        lens->SetAttribute("ApertureRadius", m_camera.getApertureRadius());
        camera_node->InsertEndChild(lens);
        
        tinyxml2::XMLElement *orientation = config.NewElement("Orientation");
        math::quatf camera_orientation = m_camera.getOrientation();
        orientation->SetAttribute("X", camera_orientation.getAxis()[0]);
        orientation->SetAttribute("Y", camera_orientation.getAxis()[1]);
        orientation->SetAttribute("Z", camera_orientation.getAxis()[2]);
        orientation->SetAttribute("Angle", camera_orientation.getAngle());
        camera_node->InsertEndChild(orientation);
        
        tinyxml2::XMLElement *speed = config.NewElement("Speed");
        speed->SetAttribute("Movement", m_camera_movement_speed);
        speed->SetAttribute("Rotatino", m_camera_rotation_speed);
        camera_node->InsertEndChild(speed);
        
        root_element->InsertEndChild(camera_node);
    }
    
    // Write render settings.
    {
        tinyxml2::XMLElement *render_settings_node = config.NewElement("RenderSettings");
        
        // Read the current status of the GI from the uniform block data.
        int gi_on = 0;
        m_gi_buffer.bind();
		GIUniformBuffer *block = m_gi_buffer.mapBuffer<GIUniformBuffer>();
        gi_on = block->enabled;
		m_gi_buffer.unmapBuffer();
        m_gi_buffer.unbind();

        render_settings_node->SetAttribute("RaysPerPixel", m_total_pass_count);
        render_settings_node->SetAttribute("DefaultGIOn", gi_on);
        render_settings_node->SetAttribute("MaxRayDepth", m_max_ray_depth);
        
        root_element->InsertEndChild(render_settings_node);
    }

    // Write out the document.
    config.SaveFile("scene.xml");
}



