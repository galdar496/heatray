//
//  ConfigVariables.h
//  Heatray
//
//  Created by Cody White on 7/27/15.
//  Copyright (c) 2015 Cody White. All rights reserved.
//

#include "ConfigVariables.h"
#include "../../utility/tinyxml2.h"
#include <iostream>
#include <unordered_map>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
    #define strcpy strcpy_s
#endif

namespace config
{

enum class VariableType
{
    kInt,
    kFloat,
    kBool,
    kString,
    kVec3
};

struct Variable
{
    // Supported config variables are stored within the following union.
    union Value
    {
        Value() {}
        Value(int integer)            { i = integer; }
        Value(bool boolean)           { b = boolean; }
        Value(float decimal)          { f = decimal; }
        Value(const char *string)     { strcpy(c, string); }
        Value(const math::Vec3f &vec) { v = vec; }

        int    i;
        bool   b;
        float  f;
        char   c[256];
        math::Vec3f v;
    };

    explicit Variable(const std::string varName, VariableType varType, Value varDefaultValue) :
        name(varName),
        type(varType),
        value(varDefaultValue)
    {
    }

    VariableType type;
    const std::string name;
    Value value;
};

#define X(variableGroup, variableName, type, defaultValue) Variable(#variableName, VariableType::type, (Variable::Value)defaultValue),

    // Table of all config variables in the system along with their default values.
    static Variable g_configVariables[] =
    {
        HEATRAY_CONFIG_VARIABLES
        Variable("INVALID_VALUE", VariableType::kBool, (Variable::Value)10)
    };

#undef X

// Group the config variables based on their specified "VariableGroup" defined in HEATRAY_CONFIG_VARIABLES.
typedef std::unordered_map<std::string, std::vector<Variable *> > VariableMap;
static VariableMap g_configVariableMap;

ConfigVariables::ConfigVariables()
{
    if (g_configVariableMap.empty())
    {
        // Read the config variables from the X macro.
        
        #define X(variableGroup, variableName, type, defaultValue) \
            g_configVariableMap[#variableGroup].push_back(&(g_configVariables[k##variableName]));
        
            HEATRAY_CONFIG_VARIABLES
        
        #undef X
    }
}

ConfigVariables::~ConfigVariables()
{
}

bool ConfigVariables::ParseConfigFile(const std::string &filename)
{
    bool result = true;

    // Read each known element of the XML config file.
    const tinyxml2::XMLElement *rootConfigNode = nullptr;
    tinyxml2::XMLDocument xmlConfigFile;

    tinyxml2::XMLError error = xmlConfigFile.LoadFile(filename.c_str());
    if (error != tinyxml2::XML_SUCCESS)
    {
        xmlConfigFile.PrintError();
        std::cout << "Unable to load configuration file " << filename << std::endl;
        result = false;
    }
    else
    {
        rootConfigNode = xmlConfigFile.FirstChildElement(s_rootConfigNodeName);
        if (rootConfigNode == nullptr)
        {
            std::cout << "Unable to load root config node " << s_rootConfigNodeName << " in configuration file " << filename << std::endl;
            result = false;
        }
        else
        {
            VariableMap::iterator iter = g_configVariableMap.begin();
            for (; iter != g_configVariableMap.end(); ++iter)
            {
                // Read the specific node for this config group.
                std::string groupName = iter->first;
                const tinyxml2::XMLElement *groupNode = rootConfigNode->FirstChildElement(groupName.c_str());
                if (!groupNode)
                {
                    std::cout << "Unable to read config group " << iter->first << " from config file" << std::endl;
                    continue;
                }
                
                // Read each variable in this config group. Every variable will have an attribute attached
                // to it that contains the final value of the config variable.
                for (size_t ii = 0; ii < iter->second.size(); ++ii)
                {

                    Variable *variable = iter->second[ii];
                    const tinyxml2::XMLElement *element = groupNode->FirstChildElement(variable->name.c_str());
                    if (element)
                    {
                        switch (variable->type)
                        {
                            case VariableType::kInt:
                            {
                                element->QueryAttribute(s_attributeName, &variable->value.i);
                                break;
                            }
                            case VariableType::kBool:
                            {
                                element->QueryAttribute(s_attributeName, &variable->value.b);
                                break;
                            }
                            case VariableType::kFloat:
                            {
                                element->QueryAttribute(s_attributeName, &variable->value.f);
                                break;
                            }
                            case VariableType::kString:
                            {
                                std::string value = element->Attribute(s_attributeName);
                                strcpy(variable->value.c, value.c_str());
                                break;
                            }
                            case VariableType::kVec3:
                            {
                                element->QueryAttribute(s_attributeNameX, &variable->value.v[0]);
                                element->QueryAttribute(s_attributeNameY, &variable->value.v[1]);
                                element->QueryAttribute(s_attributeNameZ, &variable->value.v[2]);
                                break;
                            }
                            default:
                            {
                                assert(0 && "Unsupported variable type");
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

bool ConfigVariables::WriteConfigFile(const std::string &filename) const
{
    tinyxml2::XMLDocument file;
    tinyxml2::XMLElement *rootElement = file.NewElement(s_rootConfigNodeName);
    file.InsertFirstChild(rootElement);
    
    // Create an XML element for each config group with a sub element for each variable.
    VariableMap::iterator iter = g_configVariableMap.begin();
    for (; iter != g_configVariableMap.end(); ++iter)
    {
        std::string groupName = iter->first;
        tinyxml2::XMLElement *groupNode = file.NewElement(groupName.c_str());
        
        // Write the variables for this config group.
        for (size_t ii = 0; ii < iter->second.size(); ++ii)
        {
            const Variable *variable = iter->second[ii];
            
            tinyxml2::XMLElement *newVariable = file.NewElement(variable->name.c_str());
            
            switch (variable->type)
            {
                case VariableType::kInt:
                {
                    newVariable->SetAttribute(s_attributeName, variable->value.i);
                    break;
                }
                case VariableType::kFloat:
                {
                    newVariable->SetAttribute(s_attributeName, variable->value.f);
                    break;
                }
                case VariableType::kBool:
                {
                    newVariable->SetAttribute(s_attributeName, variable->value.b);
                    break;
                }
                case VariableType::kString:
                {
                    newVariable->SetAttribute(s_attributeName, variable->value.c);
                    break;
                }
                case VariableType::kVec3:
                {
                    newVariable->SetAttribute(s_attributeNameX, variable->value.v[0]);
                    newVariable->SetAttribute(s_attributeNameY, variable->value.v[1]);
                    newVariable->SetAttribute(s_attributeNameZ, variable->value.v[2]);
                    break;
                }
                default:
                {
                    assert(0 && "Unimplemented config variable type");
                    break;
                }
            }
            
            groupNode->InsertEndChild(newVariable);
        }
        
        rootElement->InsertEndChild(groupNode);
    }
    
    return (file.SaveFile(filename.c_str()) == tinyxml2::XML_NO_ERROR);
}

void ConfigVariables::GetVariableValue(const ConfigVariable &variable, int &value) const
{
    value = g_configVariables[variable].value.i;
}

void ConfigVariables::GetVariableValue(const ConfigVariable &variable, bool &value) const
{
    value = g_configVariables[variable].value.b;
}

void ConfigVariables::GetVariableValue(const ConfigVariable &variable, float &value) const
{
    value = g_configVariables[variable].value.f;
}

void ConfigVariables::GetVariableValue(const ConfigVariable &variable, std::string &value) const
{
    value = std::string(g_configVariables[variable].value.c);
}

void ConfigVariables::GetVariableValue(const ConfigVariable &variable, math::Vec3f &value) const
{
    value = g_configVariables[variable].value.v;
}

void ConfigVariables::SetVariableValue(const ConfigVariable &variable, int value) const
{
    g_configVariables[variable].value.i = value;
}

void ConfigVariables::SetVariableValue(const ConfigVariable &variable, bool value) const
{
    g_configVariables[variable].value.b = value;
}

void ConfigVariables::SetVariableValue(const ConfigVariable &variable, float value) const
{
    g_configVariables[variable].value.f = value;
}

void ConfigVariables::SetVariableValue(const ConfigVariable &variable, const std::string &value) const
{
    strcpy(g_configVariables[variable].value.c, value.c_str());
}

void ConfigVariables::SetVariableValue(const ConfigVariable &variable, const math::Vec3f &value) const
{
    g_configVariables[variable].value.v = value;
}

} // namespace config