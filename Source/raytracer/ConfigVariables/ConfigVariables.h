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

#include "../../math/Vector.h"
#include "../../utility/tinyxml2.h"
#include <string>
#include <assert.h>
#include <unordered_map>
#include <vector>

namespace config
{

// Config variables that should be present in the config xml file. The parameters are marked as:
// "Variable Group", "Variable Name" (as it is present in the config file) "VariableType", and "Variable Default Value".

#define HEATRAY_CONFIG_VARIABLES \
    X(Mesh, ModelPath, StringVariable, "Resources/models/chess.obj")   \
    \
    X(Camera, Position,         Vec3Variable,  math::Vec3f(0.0f, 0.0f, 50.0f))   \
    X(Camera, Orientation,      Vec3Variable,  math::Vec3f(0.0f, 0.0f, 0.0f))    \
    X(Camera, OrientationAngle, FloatVariable, 1.0f)   \
    X(Camera, MovementSpeed,    FloatVariable, 5.5f)   \
    X(Camera, RotationSpeed,    FloatVariable, 0.2f)   \
    X(Camera, FocalDistance,    FloatVariable, 25.0f)  \
    X(Camera, ApertureRadius,   FloatVariable, 0.0f)   \
    \
    X(Render, FramebufferWidth,     IntVariable,   512)      \
    X(Render, FramebufferHeight,    IntVariable,   512)      \
    X(Render, RaysPerPixel,         IntVariable,   1024)     \
    X(Render, GIOn,                 IntVariable,   0)        \
    X(Render, MaxRayDepth,          IntVariable,   5)        \
    X(Render, ExposureCompensation, FloatVariable, 0.0f)     \
    \
    X(Shader, RayShaderPath,    StringVariable, "Resources/shaders/simpleRayShader.rsh") \
    X(Shader, LightShaderPath,  StringVariable, "Resources/shaders/simpleLight.rsh")

////////////////////////////////////////////////////////////////////////////////

class Variable;

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
        /// @param filename Path to the config XML file which contains the engine configuration.
        ///
        /// @return If true, the file was successfully parsed.
        ///
        bool ParseConfigFile(const std::string &filename);
    
        ///
        /// Write a new XML config file to disk using the current values of the config variables.
        ///
        /// @param filename Path to save the new config file to.
        ///
        /// @return If true, the new config file was successfully written.
        ///
        bool WriteConfigFile(const std::string &filename) const;
    
        ///
        /// Get the value of a specific config variable. If ParseConfigFile() has not
        /// been called before making a call to these functions then only the variable
        /// default values will be returned.
        ///
        /// @param variable The specific variable that is being requested.
        ///
        /// @return Requested variable.
        ///
        const Variable *GetVariable(const ConfigVariable &variable) const;
        Variable *GetVariable(const ConfigVariable &variable);
    
    private:
    
        // Table of all config variables in the system along with their default values.
        static Variable *m_configVariables[ConfigVariables::kNumConfigVariables];
        
        // Group the config variables based on their specified "VariableGroup" defined in HEATRAY_CONFIG_VARIABLES.
        typedef std::unordered_map<std::string, std::vector<Variable *> > VariableMap;
        static VariableMap m_configVariableMap;
};

// Define each variable type here. Any custom variable types need to inherit from Variable and override the
// Read and Write functions.

class Variable
{
    public:
        
        explicit Variable(const char *varName) :
        name(varName)
        {
        }
        
        virtual ~Variable() {}
        
        virtual void Read(const tinyxml2::XMLElement *element) = 0;
        virtual void Write(tinyxml2::XMLElement *element) const = 0;
        
        std::string name;
    
    protected:
    
        constexpr static const char* const s_attributeName   = "value";
        constexpr static const char* const s_attributeNameX  = "valueX";
        constexpr static const char* const s_attributeNameY  = "valueY";
        constexpr static const char* const s_attributeNameZ  = "valueZ";
};
    
class FloatVariable : public Variable
{
    public:
        
        explicit FloatVariable(const char *varName, float defaultVal) :
        Variable(varName),
        f(defaultVal)
        {
        }
        
        virtual ~FloatVariable() {}
        
        virtual void Read(const tinyxml2::XMLElement *element) override
        {
            element->QueryFloatAttribute(s_attributeName, &f);
        }
        
        virtual void Write(tinyxml2::XMLElement *element) const override
        {
            element->SetAttribute(s_attributeName, f);
        }
        
        float f;
};
    
class IntVariable : public Variable
{
public:
        
        explicit IntVariable(const char *varName, int defaultVal) :
        Variable(varName),
        i(defaultVal)
        {
        }
        
        virtual ~IntVariable() {}
        
        virtual void Read(const tinyxml2::XMLElement *element) override
        {
            element->QueryIntAttribute(s_attributeName, &i);
        }
        
        virtual void Write(tinyxml2::XMLElement *element) const override
        {
            element->SetAttribute(s_attributeName, i);
        }
        
        int i;
};
    
class StringVariable : public Variable
{
public:
        
        explicit StringVariable(const char *varName, const char *defaultVal) :
        Variable(varName),
        s(defaultVal)
        {
        }
        
        virtual ~StringVariable() {}
        
        virtual void Read(const tinyxml2::XMLElement *element) override
        {
            s = element->Attribute(s_attributeName);
        }
        
        virtual void Write(tinyxml2::XMLElement *element) const override
        {
            element->SetAttribute(s_attributeName, s.c_str());
        }
        
        std::string s;
};
    
class Vec3Variable : public Variable
{
    public:
        
        explicit Vec3Variable(const char *varName, const math::Vec3f &defaultVal) :
        Variable(varName),
        v(defaultVal)
        {
        }
        
        virtual ~Vec3Variable() {}
        
        virtual void Read(const tinyxml2::XMLElement *element) override
        {
            element->QueryFloatAttribute(s_attributeNameX, &v[0]);
            element->QueryFloatAttribute(s_attributeNameY, &v[1]);
            element->QueryFloatAttribute(s_attributeNameZ, &v[2]);
        }
        
        virtual void Write(tinyxml2::XMLElement *element) const override
        {
            element->SetAttribute(s_attributeNameX, v[0]);
            element->SetAttribute(s_attributeNameY, v[1]);
            element->SetAttribute(s_attributeNameZ, v[2]);
        }
        
        math::Vec3f v;
};

} // namespace config
