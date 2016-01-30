//
//  ConfigVariables.h
//  Heatray
//
//  Created by Cody White on 7/27/15.
//  Copyright (c) 2015 Cody White. All rights reserved.
//

#pragma once

///
/// Define a configuration variables to read from an XML config file for each system.
///

#include <string>
#include <assert.h>

namespace config
{

// Config variables that should be present in the config xml file. The parameters are marked as:
// "Variable Group", "Variable Name" (as it is present in the config file) "VariableType", and "Variable Default Value".

#define HEATRAY_CONFIG_VARIABLES \
    X(Mesh, ModelPath, kString, "Resources/models/chess.obj")   \
    X(Mesh, Scale,     kFloat,  1.0f)                           \
    \
    X(Camera, PositionX,        kFloat, 0.0f)   \
    X(Camera, PositionY,        kFloat, 0.0f)   \
    X(Camera, PositionZ,        kFloat, 50.0f)  \
    X(Camera, OrientationX,     kFloat, 0.0f)   \
    X(Camera, OrientationY,     kFloat, 0.0f)   \
    X(Camera, OrientationZ,     kFloat, 0.0f)   \
    X(Camera, OrientationAngle, kFloat, 1.0f)   \
    X(Camera, MovementSpeed,    kFloat, 5.5f)   \
    X(Camera, RotationSpeed,    kFloat, 0.2f)   \
    X(Camera, LensFocalLength,  kFloat, 25.0f)  \
    X(Camera, ApertureRadius,   kFloat, 0.0f)   \
    \
    X(Render, FramebufferWidth,     kInt, 512)      \
    X(Render, FramebufferHeight,    kInt, 512)      \
    X(Render, RaysPerPixel,         kInt, 1024)     \
    X(Render, GIOn,                 kInt, 0)        \
    X(Render, MaxRayDepth,          kInt, 5)        \
    X(Render, ExposureCompensation, kFloat, 0.0f)   \
    \
    X(Shader, RayShaderPath,    kString, "Resources/shaders/simpleRayShader.rsh") \
    X(Shader, LightShaderPath,  kString, "Resources/shaders/simpleLight.rsh")

////////////////////////////////////////////////////////////////////////////////

class ConfigVariables
{
    public:

        // All config variables present in the system.
        enum ConfigVariable
        {
            #define X(variableGroup, variableName, type, defaultValue) k##variableName,

                HEATRAY_CONFIG_VARIABLES
                kNumConfigVariables

            #undef X
        };

        ConfigVariables();
        ~ConfigVariables();

        ///
        /// Read the config file and populate the list of config variables.
        /// 
        /// @filename Path to the config XML file which contains the engine configuration.
        /// @return If valid, the file was successfully parsed.
        ///
        bool ParseConfigFile(const std::string &filename);
    
        void GetVariableValue(const ConfigVariable &variable, int &value) const;
        void GetVariableValue(const ConfigVariable &variable, bool &value) const;
        void GetVariableValue(const ConfigVariable &variable, float &value) const;
        void GetVariableValue(const ConfigVariable &variable, std::string &value) const;
};

} // namespace config
