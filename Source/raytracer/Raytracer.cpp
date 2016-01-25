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
        kDecreaseApertureWidth = '[',
        kIncreaseExposure      = 'k',
        kDecreaseExposure      = 'l'
    };
} // namespace Keys

Raytracer::Raytracer() :
    m_cameraMovementSpeed(5.5f),
    m_cameraRotationSpeed(0.2f),
    m_fbo(RL_NULL_FRAMEBUFFER),
    m_passesPerformed(0),
    m_saveImage(false),
    m_totalPassCount(1024),
    m_maxRayDepth(5),
    m_exposureCompensation(0.0f)
{
    m_keyboard.reset();
}

Raytracer::~Raytracer()
{
}

bool Raytracer::Initialize(const std::string &configFilePath)
{
    // Set all sampling textures to use linear filtering.
    gfx::Texture::Params texture_params;
    texture_params.minFilter = RL_LINEAR;
    
    tinyxml2::XMLDocument configFile;
    const tinyxml2::XMLElement *rootConfigNode = nullptr;
    if (configFile.LoadFile(configFilePath.c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cout << "Unable to load configuration file " << configFilePath << std::endl;
        return false;
    }
    
    rootConfigNode = configFile.FirstChildElement("HeatRayConfig");
    
	// Spawn a thread to load the mesh. This just puts the mesh data into memory, still have to create
    // VBOs out of the data later.
    std::thread meshThread(&Raytracer::LoadModel, this, rootConfigNode->FirstChildElement("Mesh"));
    
    // Construct the OpenRL context.
//    OpenRLContextAttribute attributes[] = {kOpenRL_EnableRayPrefixShaders, 1, NULL};
    m_rlContext = OpenRLCreateContext(nullptr, nullptr, nullptr);
    OpenRLSetCurrentContext(m_rlContext);
    
    // Setup configurable objects.
    SetupFramebuffer(rootConfigNode->FirstChildElement("Framebuffer"));
    SetupCamera(rootConfigNode->FirstChildElement("Camera"));
    SetupRenderSettings(rootConfigNode->FirstChildElement("RenderSettings"));

    // Setup the frameshader to generate the primary rays and bind it to the frame primitive.
    {
        bool error = false;
		error  = m_raytracingFrameProgram.AddShader("Resources/shaders/perspective.frame", gfx::Shader::ShaderType::kFrame);
        error |= m_raytracingFrameProgram.Link();
        
        if (!error)
        {
            return false;
        }

        // Read the uniform locations from the frame shader.
        m_frameUniforms.cameraPosition          = m_raytracingFrameProgram.GetUniformLocation("cameraPosition");
        m_frameUniforms.forward                 = m_raytracingFrameProgram.GetUniformLocation("forward");
        m_frameUniforms.up                      = m_raytracingFrameProgram.GetUniformLocation("up");
        m_frameUniforms.right                   = m_raytracingFrameProgram.GetUniformLocation("right");
        m_frameUniforms.fovTan                  = m_raytracingFrameProgram.GetUniformLocation("fovTan");
        m_frameUniforms.focalLength             = m_raytracingFrameProgram.GetUniformLocation("focalLength");
        m_frameUniforms.aspectRatio             = m_raytracingFrameProgram.GetUniformLocation("aspectRatio");
        m_frameUniforms.jitterTexture           = m_raytracingFrameProgram.GetUniformLocation("jitterTexture");
        m_frameUniforms.apertureSampleTexture   = m_raytracingFrameProgram.GetUniformLocation("apertureSampleTexture");
        m_frameUniforms.randomTextureMatrix     = m_raytracingFrameProgram.GetUniformLocation("randomTextureMatrix");
        
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);
		m_raytracingFrameProgram.Bind();
    }
    
    // Setup the vertex shader to use for all ray shaders.
    {
        bool error = m_vertexShader.Load("Resources/shaders/passthrough.vert", gfx::Shader::ShaderType::kVertex);
        if (!error)
        {
            return false;
        }
    }
    
    {
        // Create lighting dummy uniform block in order to setup the bindings for shader generation.
        LightUniformBuffer dummyBlock;
        m_lightBuffer.SetTarget(RL_UNIFORM_BLOCK_BUFFER);
        m_lightBuffer.Load(&dummyBlock, sizeof(LightUniformBuffer), "Light buffer");
        
        // Wait for the mesh to finish loading before we continue.
        meshThread.join();
        
        // Get the lighting information from the mesh and then create the mesh rendering info.
        GetLighting(m_mesh);
        m_mesh.CreateRenderData();
        
        {
            // Read the paths to the shader files from the config file.
            std::string rayShaderPath;
            std::string lightShaderPath;

            const tinyxml2::XMLElement *shaderNode = rootConfigNode->FirstChildElement("Shader");
            rayShaderPath = shaderNode->FindAttribute("Ray")->Value();
            lightShaderPath = shaderNode->FindAttribute("Light")->Value();

            ShaderGenerator::GenerationInfo generatorInfo;
            generatorInfo.mesh            = &m_mesh;
            generatorInfo.vertexShader    = &m_vertexShader;
            generatorInfo.lightBuffer     = &m_lightBuffer;
            generatorInfo.giBuffer        = &m_giBuffer;
            generatorInfo.rayShaderPath   = rayShaderPath;
            generatorInfo.lightShaderPath = lightShaderPath;
            generatorInfo.maxLightCount   = MAX_LIGHTS;
            generatorInfo.lights          = &m_lights;
            
            ShaderGenerator generator;
            if (!generator.GenerateShaders(generatorInfo))
            {
                return false;
            }
        }
        
        // Create the uniform block buffer which will store the current light subregion to sample. Every ray shader
        // shares this same buffer so we only need to update it once.
        m_lightBuffer.Bind();
        LightUniformBuffer *block = m_lightBuffer.MapBuffer<LightUniformBuffer>();
        block->count = static_cast<int>(m_lights.size());
        for (int ii = 0; ii < block->count; ++ii)
        {
            block->primitive[ii] = m_lights[ii].primitive;
        }
        m_lightBuffer.UnmapBuffer();
        m_lightBuffer.Unbind();
        
        m_mesh.ClearLoadedData();
    }

    return true;
}

void Raytracer::Destroy()
{
    if (m_fbo != RL_NULL_FRAMEBUFFER)
    {
        rlDeleteFramebuffers(1, &m_fbo);
        m_fbo = RL_NULL_FRAMEBUFFER;
    }
    
    m_mesh.Destroy();
    m_randomValuesTexture.Destroy();
    m_apertureSampleTexture.Destroy();
    m_raytracingFrameProgram.Destroy();
    m_vertexShader.Destroy();
    m_fboTexture.Destroy();
    m_jitterTexture.Destroy();
    m_lightBuffer.Destroy();
    m_giBuffer.Destroy();
    
    OpenRLDestroyContext(m_rlContext);
}

void Raytracer::Update(const float dt)
{
    CheckKeys(dt);
}

void Raytracer::Render(Pixels &outputPixels)
{
    // Only render if we actually need to, otherwise the finished render will just be displayed by GLUT.
    if (m_passesPerformed <= m_totalPassCount)
    {
		// Update the light position for soft shadowing.
        {
            // Update the lighting uniform block to use the next position for sampling.
            m_lightBuffer.Bind();
            LightUniformBuffer *block = m_lightBuffer.MapBuffer<LightUniformBuffer>();
            for (size_t ii = 0; ii < m_lights.size(); ++ii)
            {
                block->position[ii] = m_lights[ii].samplePositions[m_passesPerformed - 1]; // -1 because m_passesPerformed does not start at 0.
                block->normal[ii]   = m_lights[ii].sampleNormals[m_passesPerformed - 1];
            }
            m_lightBuffer.UnmapBuffer();
            m_lightBuffer.Unbind();
        }

        // Generate a random texture matrix to use for indexing into the textures containing random values.
        // Every frame this matrix will be randomly generated to ensure that random values are sampled
        // in the shaders.
        math::Mat4f randomTextureMatrix = math::Mat4f::Identity();
        {
            math::Mat4f rotation;

            // Rotate about the y axis.
            math::Quatf yRotate(util::Random(-math::PI, math::PI), math::Vec3f(0.0f, 1.0f, 0.0f), true);
            yRotate.ToMatrix(rotation);
            randomTextureMatrix *= rotation;
            
            // Rotate about the x axis.
            math::Quatf xRotate(util::Random(0.0f, math::TWO_PI), math::Vec3f(1.0f, 0.0f, 0.0f), true);
            xRotate.ToMatrix(rotation);
            randomTextureMatrix *= rotation;
            
            // Now randomly scale the matrix.
            const float max_scale = 5.0f;
            math::Mat4f random_scale = math::Mat4f::Identity();
            random_scale(0, 0) = util::Random(0.0f, max_scale);
            random_scale(1, 1) = util::Random(0.0f, max_scale);
            random_scale(2, 2) = util::Random(0.0f, max_scale);

            randomTextureMatrix *= random_scale;
        }
        
        rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo);
        // Setup the camera parameters to the frameshader.
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);
        m_raytracingFrameProgram.Bind();
        m_raytracingFrameProgram.Set3fv(m_frameUniforms.cameraPosition, m_camera.GetPosition().v);
        m_raytracingFrameProgram.Set3fv(m_frameUniforms.forward, m_camera.GetForwardVector().v);
        m_raytracingFrameProgram.Set3fv(m_frameUniforms.up, m_camera.GetUpVector().v);
        m_raytracingFrameProgram.Set3fv(m_frameUniforms.right, m_camera.GetRightVector().v);
        m_raytracingFrameProgram.Set1f(m_frameUniforms.fovTan, tanf((math::DEGREE_TO_RADIAN * m_camera.GetFOV()) * 0.5f));
        m_raytracingFrameProgram.Set1f(m_frameUniforms.focalLength, m_camera.GetFocalLength());
        m_raytracingFrameProgram.Set1f(m_frameUniforms.aspectRatio, m_camera.GetAspectRatio());
        m_raytracingFrameProgram.SetTexture(m_frameUniforms.jitterTexture, m_jitterTexture.GetTexture());
        m_raytracingFrameProgram.SetTexture(m_frameUniforms.apertureSampleTexture, m_apertureSampleTexture.GetTexture());
        m_raytracingFrameProgram.SetMatrix4fv(m_frameUniforms.randomTextureMatrix, randomTextureMatrix.v);
        rlRenderFrame();

        ++m_passesPerformed;
    }
    
    if (m_saveImage)
    {
        // Write the rendered image to an output file.
		std::vector<float> pixels;
        pixels.resize(m_fboTexture.Width() * m_fboTexture.Height() * Pixels::NUM_PIXEL_CHANNELS);
        rlBindTexture(RL_TEXTURE_2D, m_fboTexture.GetTexture());
        rlBindBuffer(RL_PIXEL_PACK_BUFFER, RL_NULL_BUFFER); // Make sure no pixel-pack buffer is bound, we want to copy into 'pixels'.
        rlGetTexImage(RL_TEXTURE_2D, 0, RL_RGB, RL_FLOAT, &pixels[0]);
        util::WriteImage("out.tiff", m_fboTexture.Width(), m_fboTexture.Height(), Pixels::NUM_PIXEL_CHANNELS, &pixels[0], GetPixelDivisor());
        
        m_saveImage = false;
    }

    // Map the rendered texture to a pixelpack buffer so that the calling function can properly display it.
    outputPixels.SetData(m_fboTexture);
    
    CheckRLErrors();
}

void Raytracer::Resize(RLint width, RLint height)
{
    if (height == 0)
    {
        height = 1;
    }
    
    // Set the new viewport.
	rlViewport(0, 0, width, height);
    m_camera.SetAspectRatio((float)((float)width / (float)height));
    
    // Regenerate the render textures to be the proper size.
    if (m_fboTexture.IsValid())
    {
        m_fboTexture.Resize(width, height);

        // Generate a random texture to use for pixel offsets while rendering. This texture has components which are uniformly
        // distrubuted over a circle. This allows for a radial filter during anti-aliasing.
        m_jitterTexture.RandomizeRadial(m_fboTexture.Width(), m_fboTexture.Height(), RL_FLOAT, 1.2f, "random");
        ResetRenderingState();
    }
}

std::bitset<256> &Raytracer::GetKeys()
{
    return m_keyboard;
}

bool Raytracer::IsSpecialKey(const char key) const
{
    return ((key == Keys::kEnableGI)   ||
            (key == Keys::kScreenshot) ||
            (key == Keys::kSaveConfig)
           );
}

int Raytracer::GetNumPassesPerformed() const
{
    return m_passesPerformed;
}

void Raytracer::GetDimensions(RLint &width, RLint &height)
{
    width  = m_fboTexture.Width();
    height = m_fboTexture.Height();
}

float Raytracer::GetPixelDivisor() const
{
    return (1.0f / m_passesPerformed) + m_exposureCompensation;
}

void Raytracer::CheckKeys(const float dt)
{
    static util::Timer currentTime(true);

    // Ignore non-camera movement keypresses for this much time to account for 
    // the key being held accidentally.
    const float timeDelta = 0.5f;

    float aperatureIncrement = 0.1f;
    float focalLengthIncrement = 1.0f;
    float exposureIncrement = 0.0001f;
    
    bool resetRenderer = false;
    
    if (m_keyboard.test(Keys::kCameraForward))
    {
        m_camera.Move(0.0f, 0.0f, -m_cameraMovementSpeed * dt);
        resetRenderer = true;
    }
    else if (m_keyboard.test(Keys::kCameraBackward))
    {
        m_camera.Move(0.0f, 0.0f, m_cameraMovementSpeed * dt);
        resetRenderer = true;
    }
    
    if (m_keyboard.test(Keys::kCameraPanLeft))
    {
        m_camera.Move(-m_cameraMovementSpeed * dt, 0.0f, 0.0f);
        resetRenderer = true;
    }
    else if (m_keyboard.test(Keys::kCameraPanRight))
    {
        m_camera.Move(m_cameraMovementSpeed * dt, 0.0f, 0.0f);
        resetRenderer = true;
    }
    
    if (m_keyboard.test(Keys::kCameraRotateUp))
    {
        m_camera.Pitch(-m_cameraRotationSpeed * dt);
        resetRenderer = true;
    }
    else if (m_keyboard.test(Keys::kCameraRotateDown))
    {
        m_camera.Pitch(m_cameraRotationSpeed * dt);
        resetRenderer = true;
    }
    
    if (m_keyboard.test(Keys::kCameraRotateRight))
    {
        m_camera.Yaw(m_cameraRotationSpeed * dt);
        resetRenderer = true;
    }
    else if (m_keyboard.test(Keys::kCameraRotateLeft))
    {
        m_camera.Yaw(-m_cameraRotationSpeed * dt);
        resetRenderer = true;
    }
    
    if (m_keyboard.test(Keys::kCameraRollLeft))
    {
        m_camera.Roll(-m_cameraRotationSpeed * dt);
        resetRenderer = true;
    }
    else if (m_keyboard.test(Keys::kCameraRollRight))
    {
        m_camera.Roll(m_cameraRotationSpeed * dt);
        resetRenderer = true;
    }
    
    if (m_keyboard.test(Keys::kScreenshot) && currentTime.GetElapsedTime() > timeDelta)
    {
        m_saveImage = true;
        currentTime.Restart();

        // Reset this key manually as it won't be reset by the windowing system.
        m_keyboard.reset(Keys::kScreenshot);
    }
    else if (m_keyboard.test(Keys::kSaveConfig) && currentTime.GetElapsedTime() > timeDelta)
    {
        // Write out a configuration file with the current rendering settings.
        WriteConfigFile();
        currentTime.Restart();

        // Reset this key manually as it won't be reset by the windowing system.
        m_keyboard.reset(Keys::kSaveConfig);
    }
    
    if (m_keyboard.test(Keys::kIncreaseApertureWidth))
    {
        float apertureWidth = m_camera.GetApertureRadius();
        apertureWidth += aperatureIncrement;
        resetRenderer = true;
        
        // Regenerate the aperture sampling texture to reflect the new aperture radius.
        m_apertureSampleTexture.RandomizeRadial(m_fboTexture.Width(), m_fboTexture.Height(), RL_FLOAT, apertureWidth, "random");
        
        m_camera.SetApertureRadius(apertureWidth);
    }
    else if (m_keyboard.test(Keys::kDecreaseApertureWidth))
    {
        float apertureWidth = m_camera.GetApertureRadius();
        apertureWidth = std::max(0.0f, apertureWidth - aperatureIncrement);
        resetRenderer = true;
        
        // Regenerate the aperture sampling texture to reflect the new aperture radius.
        m_apertureSampleTexture.RandomizeRadial(m_fboTexture.Width(), m_fboTexture.Height(), RL_FLOAT, apertureWidth, "random");
        
        m_camera.SetApertureRadius(apertureWidth);
    }
    
    if (m_keyboard.test(Keys::kIncreaseFocalLength))
    {
        m_camera.SetFocalLength(m_camera.GetFocalLength() + focalLengthIncrement);
        resetRenderer = true;
    }
	else if (m_keyboard.test(Keys::kDecreaseFocalLength))
    {
        m_camera.SetFocalLength(std::max(0.0f, m_camera.GetFocalLength() - focalLengthIncrement));
        resetRenderer = true;
    }
    
    if (m_keyboard.test(Keys::kEnableGI) && currentTime.GetElapsedTime() > timeDelta) // perform GI.
    {
        m_giBuffer.Bind();
		GIUniformBuffer *block = m_giBuffer.MapBuffer<GIUniformBuffer>();
        block->enabled = (block->enabled + 1) & 1;
		m_giBuffer.UnmapBuffer();
        m_giBuffer.Unbind();
        resetRenderer = true;
        currentTime.Restart();

        // Reset this key manually as it won't be reset by the windowing system.
        m_keyboard.reset(Keys::kEnableGI);
    }

    if (m_keyboard.test(Keys::kIncreaseExposure))
    {
        m_exposureCompensation += exposureIncrement;
    }
    else if (m_keyboard.test(Keys::kDecreaseExposure))
    {
        m_exposureCompensation -= exposureIncrement;
        m_exposureCompensation = std::max(0.0f, m_exposureCompensation);
    }
    
    if (resetRenderer)
    {
        ResetRenderingState();
    }
}

void Raytracer::ResetRenderingState()
{
    if (m_fbo != RL_NULL_FRAMEBUFFER)
    {
        rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo);
        rlClear(RL_COLOR_BUFFER_BIT);
    }
    
    m_passesPerformed = 1;
}

void Raytracer::LoadModel(const tinyxml2::XMLElement *meshNode)
{
    std::string modelPath = meshNode->Attribute("Model");
    float modelScaling = 1.0f;
    
    meshNode->QueryFloatAttribute("Scale", &modelScaling);
    
    if (!m_mesh.Load(modelPath, false, modelScaling, false))
    {
        exit(0);
    }
}

void Raytracer::SetupCamera(const tinyxml2::XMLNode *cameraNode)
{
    const tinyxml2::XMLElement *element = nullptr;
    math::Vec3f tmpVector;
    float       tmpValue = 0.0f;
    
    // Setup the camera's position.
    {
        element = cameraNode->FirstChildElement("Position");
        
		element->QueryFloatAttribute("X", &tmpVector[0]);
        element->QueryFloatAttribute("Y", &tmpVector[1]);
        element->QueryFloatAttribute("Z", &tmpVector[2]);

        m_camera.SetPosition(tmpVector);
    }
    
    // Setup the camera's lens attributes.
    {
        element = cameraNode->FirstChildElement("Lens");
        
        element->QueryFloatAttribute("FocalLength", &tmpValue);
        m_camera.SetFocalLength(tmpValue);
        
		element->QueryFloatAttribute("ApertureRadius", &tmpValue);
        m_camera.SetApertureRadius(tmpValue);
        
        // Generate the aperture sampling texture to reflect the aperture radius.
        m_apertureSampleTexture.RandomizeRadial(m_fboTexture.Width(), m_fboTexture.Height(), RL_FLOAT, m_camera.GetApertureRadius(), "random");
    }
    
    // Setup the camera's orientation.
    {
        element = cameraNode->FirstChildElement("Orientation");
        
        element->QueryFloatAttribute("X", &tmpVector[0]);
        element->QueryFloatAttribute("Y", &tmpVector[1]);
        element->QueryFloatAttribute("Z", &tmpVector[2]);
        element->QueryFloatAttribute("Angle", &tmpValue);
        
        math::Quatf orientation(tmpValue, tmpVector);
        m_camera.SetOrientation(orientation);
    }
    
    // Setup the camera's movement speed parameters.
    {
        element = cameraNode->FirstChildElement("Speed");
        
        element->QueryFloatAttribute("Movement", &m_cameraMovementSpeed);
        element->QueryFloatAttribute("Rotation", &m_cameraRotationSpeed);
    }
}

void Raytracer::SetupFramebuffer(const tinyxml2::XMLElement *framebufferNode)
{
    // Heatray renders every pass to the same framebuffer without ever clearing it. Therefore, the framebuffer created below will
    // contain the pixel information for every pass. This FBO must be processed by a shader program which divides each pixel by
    // the current number of passes ran, producing an image that is actually viewable.
    
    // Read the width and height from the config file.
    RLint framebufferWidth  = 0;
    RLint framebufferHeight = 0;
    framebufferNode->QueryIntAttribute("Width", &framebufferWidth);
    framebufferNode->QueryIntAttribute("Height", &framebufferHeight);
    
    gfx::Texture::Params textureParams;
    textureParams.minFilter      = RL_LINEAR;
    textureParams.format         = RL_RGB;
    textureParams.internalFormat = RL_RGB;
    
    // Create the default FBO for rendering into.
    rlGenFramebuffers(1, &m_fbo);
    rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo);
    m_fboTexture.SetParams(textureParams);
    m_fboTexture.Create(framebufferWidth, framebufferHeight, RL_FLOAT, nullptr, "Default FBO Texture");
    rlFramebufferTexture2D(RL_FRAMEBUFFER, RL_COLOR_ATTACHMENT0, RL_TEXTURE_2D, m_fboTexture.GetTexture(), 0);
    
    CheckRLErrors();
}

void Raytracer::SetupRenderSettings(const tinyxml2::XMLElement *renderSettingsNode)
{
    // Create the random values uniform block data. These values will be used in all diffuse shaders to bounce rays
    // and achieve indirect illumination.
    m_randomValuesTexture.Randomize(m_fboTexture.Width(), m_fboTexture.Height(), 3, RL_FLOAT, 0.0f, 1.0f, "Random 0-1 texture");
    
    GIUniformBuffer gi_uniformBuffer;
    gi_uniformBuffer.texture = m_randomValuesTexture.GetTexture();
    renderSettingsNode->QueryIntAttribute("DefaultGIOn", &(gi_uniformBuffer.enabled));
    m_giBuffer.SetTarget(RL_UNIFORM_BLOCK_BUFFER);
    m_giBuffer.Load(&gi_uniformBuffer, sizeof(GIUniformBuffer), "Random buffer");
    
    // Setup the number of passes to perform per the config file.
    renderSettingsNode->QueryIntAttribute("RaysPerPixel", &m_totalPassCount);
    
    // Set the maximum number of bounces any ray in the system can have before being terminated.
    renderSettingsNode->QueryIntAttribute("MaxRayDepth", &m_maxRayDepth);
    rlFrameParameter1i(RL_FRAME_RAY_DEPTH_LIMIT, m_maxRayDepth);
    
    CheckRLErrors();
}

void Raytracer::GetLighting(gfx::Mesh &mesh)
{
    // Go over each mesh piece and look for anything that is a light. For all lights that are found, populate a
    // 'Light' struct with information about that light to use for later rendering.
    gfx::Mesh::MeshList &meshes = mesh.GetMeshList();
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

            int num_lightTriangles = static_cast<int>(piece->vertices.size()) / 3;

            // Generate the randomized light positions to sample for each pass. Every pass of the render will use one of these generated
            // light positions.
            light->samplePositions.resize(m_totalPassCount);
            light->sampleNormals.resize(m_totalPassCount);
            std::vector<float> randomBarycentrics;
            util::GenerateRandomNumbers(0.0f, 1.0f, m_totalPassCount * 3, randomBarycentrics); // * 3 to account of 3 barycentrics per sample point.
            int barycentricIndex = 0;

            for (int ii = 0; ii < m_totalPassCount; ++ii)
            {
                // Find a random triangle to use for this sample.
                int triangleIndex = util::Random(0, num_lightTriangles - 1);

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
                int vertexIndex = triangleIndex * 3;
                math::Vec3f samplePoint = (piece->vertices[vertexIndex + 0] * gamma) +
                                          (piece->vertices[vertexIndex + 1] * beta) +
                                          (piece->vertices[vertexIndex + 2] * alpha);

                // Similarly set the normal for this sample point.
                math::Vec3f sampleNormal = (piece->normals[vertexIndex + 0] * gamma) +
                                           (piece->normals[vertexIndex + 1] * beta) +
                                           (piece->normals[vertexIndex + 2] * alpha);
                sampleNormal.Normalize();

                light->samplePositions[ii] = samplePoint;
                light->sampleNormals[ii]   = sampleNormal;
            }
        }
    }
    
    if (m_lights.empty())
    {
        std::cout << "No material contains the name \"Light\" in loaded mesh, exiting..." << std::endl;
        exit(0);
    }
}

void Raytracer::WriteConfigFile() const
{
	tinyxml2::XMLDocument config;
	tinyxml2::XMLElement *rootElement = config.NewElement("HeatRayConfig");
    config.InsertFirstChild(rootElement);
    
    // Write the framebuffer info.
	{
        tinyxml2::XMLElement *node = config.NewElement("Framebuffer");
        node->SetAttribute("Width", m_fboTexture.Width());
        node->SetAttribute("Height", m_fboTexture.Height());
        
        rootElement->InsertEndChild(node);
    }
    
    // Write the mesh info.
    {
        tinyxml2::XMLElement *node = config.NewElement("Mesh");
        node->SetAttribute("Model", m_mesh.GetName().c_str());
        node->SetAttribute("Scale", m_mesh.GetScale());
        
        rootElement->InsertEndChild(node);
    }
    
    // Write the camera info.
    {
        tinyxml2::XMLElement *cameraNode = config.NewElement("Camera");
    
        tinyxml2::XMLElement *position = config.NewElement("Position");
        math::Vec3f cameraPosition = m_camera.GetPosition();
        position->SetAttribute("X", cameraPosition[0]);
        position->SetAttribute("Y", cameraPosition[1]);
        position->SetAttribute("Z", cameraPosition[2]);
        cameraNode->InsertEndChild(position);
        
        tinyxml2::XMLElement *lens = config.NewElement("Lens");
        lens->SetAttribute("FocalLength", m_camera.GetFocalLength());
        lens->SetAttribute("ApertureRadius", m_camera.GetApertureRadius());
        cameraNode->InsertEndChild(lens);
        
        tinyxml2::XMLElement *orientation = config.NewElement("Orientation");
        math::Quatf cameraOrientation = m_camera.GetOrientation();
        orientation->SetAttribute("X", cameraOrientation.GetAxis()[0]);
        orientation->SetAttribute("Y", cameraOrientation.GetAxis()[1]);
        orientation->SetAttribute("Z", cameraOrientation.GetAxis()[2]);
        orientation->SetAttribute("Angle", cameraOrientation.GetAngle());
        cameraNode->InsertEndChild(orientation);
        
        tinyxml2::XMLElement *speed = config.NewElement("Speed");
        speed->SetAttribute("Movement", m_cameraMovementSpeed);
        speed->SetAttribute("Rotatino", m_cameraRotationSpeed);
        cameraNode->InsertEndChild(speed);
        
        rootElement->InsertEndChild(cameraNode);
    }
    
    // Write render settings.
    {
        tinyxml2::XMLElement *renderSettingsNode = config.NewElement("RenderSettings");
        
        // Read the current status of the GI from the uniform block data.
        int giOn = 0;
        m_giBuffer.Bind();
		GIUniformBuffer *block = m_giBuffer.MapBuffer<GIUniformBuffer>();
        giOn = block->enabled;
		m_giBuffer.UnmapBuffer();
        m_giBuffer.Unbind();

        renderSettingsNode->SetAttribute("RaysPerPixel", m_totalPassCount);
        renderSettingsNode->SetAttribute("DefaultGIOn", giOn);
        renderSettingsNode->SetAttribute("MaxRayDepth", m_maxRayDepth);
        
        rootElement->InsertEndChild(renderSettingsNode);
    }

    // Write out the document.
    config.SaveFile("scene.xml");
}



