//
//  World.cpp
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#include "World.h"
#include "ShaderGenerator.h"
#include "../utility/util.h"
#include "../utility/rng.h"
#include "../gfx/Shader.h"
#include "../math/Constants.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <functional>

#define GLUT_KEY_ENTER 13
#define GLUT_KEY_SPACE 32

/// Default constructor.
World::World() :
	m_camera_movement_speed(5.5f),
	m_camera_rotation_speed(0.2f),
	m_fbo(RL_NULL_FRAMEBUFFER),
    m_display_fbo(RL_NULL_FRAMEBUFFER),
    m_passes_performed(0),
	m_save_image(false),
    m_total_pass_count(1024),
    m_max_ray_depth(5)
{
    m_keyboard.reset();
}

/// Destructor.
World::~World()
{
}

/// Initialize the world for use. Returns true if
/// initialization was successful.
bool World::initialize(const std::string &config_file_path, RLint &screen_width, RLint &screen_height)
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
    std::thread mesh_thread(&World::loadModel, this, root_config_node->FirstChildElement("Mesh"));
    
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
        
        // Get the lighting information from the mesh and then create the VBOs for rendering.
        getLighting(m_mesh);
        m_mesh.createVBOs();
        
        ShaderGenerator generator;
        if (!generator.generateShaders(m_mesh, m_vertex_shader, m_light_buffer, m_gi_buffer, m_lights))
        {
            return false;
        }
        
        // Create the uniform block buffer which will store the current light subregion to sample. Every ray shader
        // shares this same buffer so we only need to update it once.
        m_light_buffer.bind();
        LightUniformBuffer *block = m_light_buffer.mapBuffer<LightUniformBuffer>();
        block->count = static_cast<int>(m_lights.size());
        for (int ii = 0; ii < block->count; ++ii)
        {
            block->primitives[ii] = m_lights[ii].primitive;
            block->normals[ii]    = m_lights[ii].normal;
        }
        m_light_buffer.unmapBuffer();
        m_light_buffer.unbind();
        
        m_mesh.clearLoadedData();
    }

    return true;
}

/// Deallocate any internal data and
/// prepare for shutdown.
void World::destroy()
{
    if (m_fbo != RL_NULL_FRAMEBUFFER)
    {
        rlDeleteFramebuffers(1, &m_fbo);
        m_fbo = RL_NULL_FRAMEBUFFER;
    }
    if (m_display_fbo != RL_NULL_FRAMEBUFFER)
    {
        rlDeleteFramebuffers(1, &m_display_fbo);
        m_display_fbo = RL_NULL_FRAMEBUFFER;
    }
    
    m_mesh.destroy();
    m_random_values_texture.destroy();
    m_aperture_sample_texture.destroy();
    m_raytracing_frame_program.destroy();
    m_vertex_shader.destroy();
    m_fbo_texture.destroy();
    m_jitter_texture.destroy();
    m_display_fbo_texture.destroy();
    m_display_frame_program.destroy();
    m_light_buffer.destroy();
    m_gi_buffer.destroy();
    
    OpenRLDestroyContext(m_rl_context);
}

/// Update this class.
void World::update(const float dt)
{
    checkKeys(dt);
}

/// Render a frame. Returns a texture value that contains the final rendered image.
const gfx::Texture *World::render()
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
                block->positions[ii] = m_lights[ii].sample_positions[m_passes_performed - 1]; // -1 because m_passes_performed does not start at 0.
            }
            m_light_buffer.unmapBuffer();
            m_light_buffer.unbind();
        }
        
        rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo);
        // Setup the camera parameters to the frameshader.
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);
        m_raytracing_frame_program.bind();
        m_raytracing_frame_program.set3fv("camera_position", m_camera.getPosition().v);
        m_raytracing_frame_program.set3fv("forward", m_camera.getForwardVector().v);
        m_raytracing_frame_program.set3fv("up", m_camera.getUpVector().v);
        m_raytracing_frame_program.set3fv("right", m_camera.getRightVector().v);
        m_raytracing_frame_program.set1f("fov_tan", tanf((math::DEGREE_TO_RADIAN * m_camera.getFOV()) * 0.5f));
        m_raytracing_frame_program.set1f("focal_length", m_camera.getFocalLength());
        m_raytracing_frame_program.set1f("aspect_ratio", m_camera.getAspectRatio());
        m_raytracing_frame_program.setTexture("jitter_texture", m_jitter_texture.getTexture());
        m_raytracing_frame_program.setTexture("aperture_sample_texture", m_aperture_sample_texture.getTexture());
        m_raytracing_frame_program.set1f("pass_divisor", ((float)m_passes_performed) / (float)m_total_pass_count);
        rlRenderFrame();

        // Display the most recent render averaged with all previous renders since the last time the camera
        // moved to the display fbo.
        rlBindFramebuffer(RL_FRAMEBUFFER, m_display_fbo);
        rlClear(RL_COLOR_BUFFER_BIT);
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);
        m_display_frame_program.bind();
        m_display_frame_program.setTexture("input_texture", m_fbo_texture.getTexture());
        m_display_frame_program.set1f("num_passes_divisor", 1.0f / (float)m_passes_performed);
        rlRenderFrame();
        
        ++m_passes_performed;
    }
    
    if (m_save_image)
    {
        // Write the rendered image to an output file.
		std::vector<float> pixels;
        pixels.resize(m_display_fbo_texture.width() * m_display_fbo_texture.height() * 4);
        rlBindTexture(RL_TEXTURE_2D, m_display_fbo_texture.getTexture());
        rlBindBuffer(RL_PIXEL_PACK_BUFFER, RL_NULL_BUFFER); // Make sure no pixel-pack buffer is bound, we want to copy into 'pixels'.
        rlGetTexImage(RL_TEXTURE_2D, 0, RL_RGBA, RL_FLOAT, &pixels[0]);
        util::writeImage("out.tiff", m_display_fbo_texture.width(), m_display_fbo_texture.height(), 4, &pixels[0], 1.0f);
        
        m_save_image = false;
    }
    
    util::checkRLErrors();
    
    return &m_display_fbo_texture;
}

/// Resize the render viewport.
void World::resize(RLint width, RLint height)
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
        m_display_fbo_texture.resize(width, height);

        // Generate a random texture to use for pixel offsets while rendering. This texture has components which are uniformly
        // distrubuted over a circle. This allows for a radial filter during anti-aliasing.
        m_jitter_texture.randomizeRadial(m_fbo_texture.width(), m_fbo_texture.height(), RL_FLOAT, 1.2f, "random");
        resetRenderingState();
    }
}

/// Get a reference to the keyboard keys.
std::bitset<256> &World::getKeys()
{
    return m_keyboard;
}

/// Check the current state of the keyboard.
void World::checkKeys(const float dt)
{
    float aperature_increment = 0.1f;
    float focal_length_increment = 1.0f;
    
    bool reset_rendering_state = false;
    
    if (m_keyboard.test('w'))
    {
        m_camera.move(0.0f, 0.0f, -m_camera_movement_speed * dt);
        reset_rendering_state = true;
    }
    else if (m_keyboard.test('s'))
    {
        m_camera.move(0.0f, 0.0f, m_camera_movement_speed * dt);
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test('a'))
    {
        m_camera.move(-m_camera_movement_speed * dt, 0.0f, 0.0f);
        reset_rendering_state = true;
    }
    else if (m_keyboard.test('d'))
    {
        m_camera.move(m_camera_movement_speed * dt, 0.0f, 0.0f);
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test('r'))
    {
        m_camera.pitch(-m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    else if (m_keyboard.test('f'))
    {
        m_camera.pitch(m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test('e'))
    {
        m_camera.yaw(m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    else if (m_keyboard.test('q'))
    {
        m_camera.yaw(-m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test('z'))
    {
        m_camera.roll(-m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    else if (m_keyboard.test('c'))
    {
        m_camera.roll(m_camera_rotation_speed * dt);
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test(GLUT_KEY_ENTER))
    {
        m_save_image = true;
    }
    else if (m_keyboard.test(GLUT_KEY_SPACE))
    {
        // Write out a configuration file with the current rendering settings.
        writeConfigFile();
    }
    
    if (m_keyboard.test(']'))
    {
        float aperture_width = m_camera.getApertureRadius();
        aperture_width += aperature_increment;
        reset_rendering_state = true;
        
        // Regenerate the aperture sampling texture to reflect the new aperture radius.
        m_aperture_sample_texture.randomizeRadial(m_fbo_texture.width(), m_fbo_texture.height(), RL_FLOAT, aperture_width, "random");
        
        m_camera.setApertureRadius(aperture_width);
    }
    else if (m_keyboard.test('['))
    {
        float aperture_width = m_camera.getApertureRadius();
        aperture_width = std::max(0.0f, aperture_width - aperature_increment);
        reset_rendering_state = true;
        
        // Regenerate the aperture sampling texture to reflect the new aperture radius.
        m_aperture_sample_texture.randomizeRadial(m_fbo_texture.width(), m_fbo_texture.height(), RL_FLOAT, aperture_width, "random");
        
        m_camera.setApertureRadius(aperture_width);
    }
    
    if (m_keyboard.test('p'))
    {
        m_camera.setFocalLength(m_camera.getFocalLength() + focal_length_increment);
        reset_rendering_state = true;
    }
	else if (m_keyboard.test('o'))
    {
        m_camera.setFocalLength(std::max(0.0f, m_camera.getFocalLength() - focal_length_increment));
        reset_rendering_state = true;
    }
    
    if (m_keyboard.test('g')) // perform GI.
    {
        m_gi_buffer.bind();
		GIUniformBuffer *block = m_gi_buffer.mapBuffer<GIUniformBuffer>();
        block->enabled = (block->enabled + 1) & 1;
		m_gi_buffer.unmapBuffer();
        m_gi_buffer.unbind();
        reset_rendering_state = true;
    }
    
    if (reset_rendering_state)
    {
        resetRenderingState();
    }
}

/// Reset the rendering state. This is done if the camera moved or the window got resized
/// and rendering needs to start over from the beginning.
void World::resetRenderingState()
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
void World::loadModel(const tinyxml2::XMLElement *mesh_node)
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
void World::setupCamera(const tinyxml2::XMLNode *camera_node)
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
void World::setupFramebuffer(const tinyxml2::XMLElement *framebuffer_node, RLint &framebuffer_width, RLint &framebuffer_height)
{
    // Heatray renders every pass to the same framebuffer without ever clearing it. Therefore, the framebuffer created below will
    // contain the pixel information for every pass. This FBO must be processed by a shader program which divides each pixel by
    // the current number of passes ran, producing an image that is actually viewable.
    
    // Read the width and height from the config file.
    framebuffer_node->QueryIntAttribute("Width", &framebuffer_width);
    framebuffer_node->QueryIntAttribute("Height", &framebuffer_height);
    
    gfx::Texture::Params texture_params;
    texture_params.min_filter      = RL_LINEAR;
    texture_params.format          = RL_RGBA;
    texture_params.internal_format = RL_RGBA;
    
    // Create the default FBO for rendering into.
    rlGenFramebuffers(1, &m_fbo);
    rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo);
    m_fbo_texture.setParams(texture_params);
    m_fbo_texture.create(framebuffer_width, framebuffer_height, RL_FLOAT, NULL, "Default FBO Texture");
    rlFramebufferTexture2D(RL_FRAMEBUFFER, RL_COLOR_ATTACHMENT0, RL_TEXTURE_2D, m_fbo_texture.getTexture(), 0);
    
    // Create the FBO for displaying to the screen.
    rlGenFramebuffers(1, &m_display_fbo);
    rlBindFramebuffer(RL_FRAMEBUFFER, m_display_fbo);
    m_display_fbo_texture.setParams(texture_params);
    m_display_fbo_texture.create(framebuffer_width, framebuffer_height, RL_FLOAT, NULL, "Display FBO Texture");
    rlFramebufferTexture2D(RL_FRAMEBUFFER, RL_COLOR_ATTACHMENT0, RL_TEXTURE_2D, m_display_fbo_texture.getTexture(), 0);
    
    // Setup the frameshader which will take care of displaying the default buffer is a human-viewable format.
    m_display_frame_program.addShader("Resources/shaders/display.frame", gfx::Shader::FRAME);
    m_display_frame_program.link();
    
    util::checkRLErrors("World::setupFramebuffer()", true);
}

/// Setup general rendering settings independent of any object contained within this class.
void World::setupRenderSettings(const tinyxml2::XMLElement *render_settings_node)
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
void World::getLighting(gfx::Mesh &mesh)
{
    // Go over each mesh piece and look for anything that is a light. For all lights that are found, populate a
    // 'Light' struct with information about that light to use for later rendering.
    // NOTE: For now, only rectangular area lights are supported.
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
            
            // Find the light dimensions. We'll use these dimensions to determine how to sample the light.
            light->dimensions[0] = math::vec3f(HUGE_VALF, HUGE_VALF, HUGE_VALF);
            light->dimensions[1] = math::vec3f(-HUGE_VALF, -HUGE_VALF, -HUGE_VALF);
            for (size_t ii = 0; ii < piece->vertices.size(); ++ii)
            {
                light->dimensions[0] = math::vectorMin(light->dimensions[0], piece->vertices[ii]);
                light->dimensions[1] = math::vectorMax(light->dimensions[1], piece->vertices[ii]);
            }
            
            // Adjust the light dimensions along the normal of the light to avoid any precision issues when trying to determine if a pixel is in shadow.
            light->normal = piece->normals[0];
            light->dimensions[0] += light->normal * 0.001f;
            light->dimensions[1] += light->normal * 0.001f;
            
            // Generate the randomized light positions to sample for each pass. Every pass of the render will use one of these generated
            // light positions.
            light->sample_positions.resize(m_total_pass_count);
            for (int ii = 0; ii < m_total_pass_count; ++ii)
            {
                light->sample_positions[ii] = util::random(light->dimensions[0], light->dimensions[1]);
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
void World::writeConfigFile() const
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



