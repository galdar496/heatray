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
    
    config::ConfigVariables configVariables;
    if (!configVariables.ParseConfigFile(configFilePath))
    {
        return false;
    }
    
	// Spawn a thread to load the mesh. This just puts the mesh data into memory, still have to create
    // VBOs out of the data later.
    std::thread meshThread(&Raytracer::LoadModel, this, configVariables);
    
    // Construct the OpenRL context.
//    OpenRLContextAttribute attributes[] = {kOpenRL_EnableRayPrefixShaders, 1, NULL};
    m_rlContext = OpenRLCreateContext(nullptr, nullptr, nullptr);
    OpenRLSetCurrentContext(m_rlContext);
    
    // Setup configurable objects.
    SetupFramebuffer(configVariables);
    SetupCamera(configVariables);
    SetupRenderSettings(configVariables);

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
        
        // Read the paths to the shader files from the config file.
        std::string rayShaderPath;
        std::string lightShaderPath;
        configVariables.GetVariableValue(config::ConfigVariables::kRayShaderPath, rayShaderPath);
        configVariables.GetVariableValue(config::ConfigVariables::kLightShaderPath, lightShaderPath);
        
        // Wait for the mesh to finish loading before we continue.
        meshThread.join();
        
        // Get the lighting information from the mesh and then create the mesh rendering info.
        GetLighting(m_mesh);
        m_mesh.CreateRenderData();
        
        {
            ShaderGenerator::GenerationInfo generatorInfo;
            generatorInfo.mesh            = &m_mesh;
            generatorInfo.vertexShader    = &m_vertexShader;
            generatorInfo.lightBuffer     = &m_lightBuffer;
            generatorInfo.giBuffer        = &m_giBuffer;
            generatorInfo.rayShaderPath   = rayShaderPath;
            generatorInfo.lightShaderPath = lightShaderPath;
            generatorInfo.maxLightCount   = g_MaxNumLights;
            generatorInfo.lights          = &m_lights;
            
            // Create the shading programs to use and properly bind then along with all mesh inputs into the programs.
            // This includes the proper uniform values (read from the mesh material files), lighting information,
            // and mesh buffer inputs. The mesh MUST have its render data created before calling this function otherwise
            // the call will fail.
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
        
        // Setup the camera parameters to the frameshader.
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
    
    //CheckRLErrors();
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
    float divisor = 1.0f / m_passesPerformed;

    // Apply exposure compensation. m_exposureCompensation stores the power but no the final value.
    float exposure = powf(2.0f, m_exposureCompensation);

    return divisor * exposure;
}

void Raytracer::CheckKeys(const float dt)
{
    static util::Timer currentTime(true);

    // Ignore non-camera movement keypresses for this much time to account for 
    // the key being held accidentally.
    const float timeDelta = 0.5f;

    float aperatureIncrement = 0.1f;
    float focalLengthIncrement = 1.0f;
    
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

    float exposureStep = 0.1f;
    if (m_keyboard.test(Keys::kIncreaseExposure))
    {
        m_exposureCompensation += exposureStep;
    }
    else if (m_keyboard.test(Keys::kDecreaseExposure))
    {
        m_exposureCompensation -= exposureStep;
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
        rlClear(RL_COLOR_BUFFER_BIT);
    }
    
    m_passesPerformed = 1;
}

void Raytracer::LoadModel(const config::ConfigVariables &configVariables)
{
    std::string modelPath;
    float scaling = 1.0f;
    
    configVariables.GetVariableValue(config::ConfigVariables::kModelPath, modelPath);
    configVariables.GetVariableValue(config::ConfigVariables::kScale, scaling);
    
    if (!m_mesh.Load(modelPath, false, scaling, false))
    {
        exit(0);
    }
}

void Raytracer::SetupCamera(const config::ConfigVariables &configVariables)
{
    math::Vec3f tmpVector;
    float       tmpValue = 0.0f;
    
    // Setup the camera's position.
    {
        configVariables.GetVariableValue(config::ConfigVariables::kPositionX, tmpVector[0]);
        configVariables.GetVariableValue(config::ConfigVariables::kPositionY, tmpVector[1]);
        configVariables.GetVariableValue(config::ConfigVariables::kPositionZ, tmpVector[2]);

        m_camera.SetPosition(tmpVector);
    }
    
    // Setup the camera's lens attributes.
    {
        configVariables.GetVariableValue(config::ConfigVariables::kLensFocalLength, tmpValue);
        m_camera.SetFocalLength(tmpValue);
        
        configVariables.GetVariableValue(config::ConfigVariables::kApertureRadius, tmpValue);
        m_camera.SetApertureRadius(tmpValue);
        
        // Generate the aperture sampling texture to reflect the aperture radius.
        m_apertureSampleTexture.RandomizeRadial(m_fboTexture.Width(), m_fboTexture.Height(), RL_FLOAT, m_camera.GetApertureRadius(), "random");
    }
    
    // Setup the camera's orientation.
    {
        configVariables.GetVariableValue(config::ConfigVariables::kOrientationX, tmpVector[0]);
        configVariables.GetVariableValue(config::ConfigVariables::kOrientationY, tmpVector[1]);
        configVariables.GetVariableValue(config::ConfigVariables::kOrientationZ, tmpVector[2]);
        configVariables.GetVariableValue(config::ConfigVariables::kOrientationAngle, tmpValue);
        
        math::Quatf orientation(tmpValue, tmpVector);
        m_camera.SetOrientation(orientation);
    }
    
    // Setup the camera's movement speed parameters.
    {
        configVariables.GetVariableValue(config::ConfigVariables::kMovementSpeed, m_cameraMovementSpeed);
        configVariables.GetVariableValue(config::ConfigVariables::kRotationSpeed, m_cameraRotationSpeed);
    }
}

void Raytracer::SetupFramebuffer(const config::ConfigVariables &configVariables)
{
    // Heatray renders every pass to the same framebuffer without ever clearing it. Therefore, the framebuffer created below will
    // contain the pixel information for every pass. This FBO must be processed by a shader program which divides each pixel by
    // the current number of passes ran, producing an image that is actually viewable.
    
    // Read the width and height from the config file.
    RLint framebufferWidth  = 0;
    RLint framebufferHeight = 0;
    configVariables.GetVariableValue(config::ConfigVariables::kFramebufferWidth, framebufferWidth);
    configVariables.GetVariableValue(config::ConfigVariables::kFramebufferHeight, framebufferHeight);
    
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
    
    // Set this framebuffer as the default one for all raytraced rendering.
    rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo);
    
    CheckRLErrors();
}

void Raytracer::SetupRenderSettings(const config::ConfigVariables &configVariables)
{
    // Create the random values uniform block data. These values will be used in all diffuse shaders to bounce rays
    // and achieve indirect illumination.
    m_randomValuesTexture.Randomize(m_fboTexture.Width(), m_fboTexture.Height(), 3, RL_FLOAT, 0.0f, 1.0f, "Random 0-1 texture");
    
    GIUniformBuffer giUniformBuffer;
    giUniformBuffer.texture = m_randomValuesTexture.GetTexture();
    configVariables.GetVariableValue(config::ConfigVariables::kGIOn, giUniformBuffer.enabled);
    m_giBuffer.SetTarget(RL_UNIFORM_BLOCK_BUFFER);
    m_giBuffer.Load(&giUniformBuffer, sizeof(GIUniformBuffer), "Random buffer");
    
    // Setup the number of passes to perform per the config file.
    configVariables.GetVariableValue(config::ConfigVariables::kRaysPerPixel, m_totalPassCount);
    
    // Set the maximum number of bounces any ray in the system can have before being terminated.
    configVariables.GetVariableValue(config::ConfigVariables::kMaxRayDepth, m_maxRayDepth);
    rlFrameParameter1i(RL_FRAME_RAY_DEPTH_LIMIT, m_maxRayDepth);
    
    configVariables.GetVariableValue(config::ConfigVariables::kExposureCompensation, m_exposureCompensation);
    
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
            if (m_lights.size() >= g_MaxNumLights)
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
    config::ConfigVariables configVars;
    
    // Set the values of the config variables to be written.
    
    // Mesh
    {
        configVars.SetVariableValue(config::ConfigVariables::kModelPath, m_mesh.GetName());
        configVars.SetVariableValue(config::ConfigVariables::kScale, m_mesh.GetScale());
    }
    
    // Camera settings
    {
        math::Vec3f cameraPosition    = m_camera.GetPosition();
        math::Quatf cameraOrientation = m_camera.GetOrientation();
        
        configVars.SetVariableValue(config::ConfigVariables::kPositionX, cameraPosition[0]);
        configVars.SetVariableValue(config::ConfigVariables::kPositionY, cameraPosition[1]);
        configVars.SetVariableValue(config::ConfigVariables::kPositionZ, cameraPosition[2]);
        
        configVars.SetVariableValue(config::ConfigVariables::kOrientationX, cameraOrientation.GetAxis()[0]);
        configVars.SetVariableValue(config::ConfigVariables::kOrientationY, cameraOrientation.GetAxis()[1]);
        configVars.SetVariableValue(config::ConfigVariables::kOrientationZ, cameraOrientation.GetAxis()[2]);
        configVars.SetVariableValue(config::ConfigVariables::kOrientationAngle, cameraOrientation.GetAngle());
        
        configVars.SetVariableValue(config::ConfigVariables::kLensFocalLength, m_camera.GetFocalLength());
        configVars.SetVariableValue(config::ConfigVariables::kApertureRadius,  m_camera.GetApertureRadius());
        
        configVars.SetVariableValue(config::ConfigVariables::kMovementSpeed, m_cameraMovementSpeed);
        configVars.SetVariableValue(config::ConfigVariables::kRotationSpeed, m_cameraRotationSpeed);
    }
    
    // Render settings
    {
        configVars.SetVariableValue(config::ConfigVariables::kFramebufferWidth, m_fboTexture.Width());
        configVars.SetVariableValue(config::ConfigVariables::kFramebufferHeight, m_fboTexture.Height());
        configVars.SetVariableValue(config::ConfigVariables::kRaysPerPixel, m_totalPassCount);
        configVars.SetVariableValue(config::ConfigVariables::kMaxRayDepth, m_maxRayDepth);
        configVars.SetVariableValue(config::ConfigVariables::kExposureCompensation, m_exposureCompensation);
        
        // Read the current status of the GI from the uniform block data.
        int giOn = 0;
        m_giBuffer.Bind();
        GIUniformBuffer *block = m_giBuffer.MapBuffer<GIUniformBuffer>();
        giOn = block->enabled;
        m_giBuffer.UnmapBuffer();
        m_giBuffer.Unbind();
        
        configVars.SetVariableValue(config::ConfigVariables::kGIOn, giOn);
    }
    
    // Write the config variables to a file.
    configVars.WriteConfigFile("scene.xml");
}



