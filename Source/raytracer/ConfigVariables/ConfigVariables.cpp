//
//  ConfigVariables.h
//  Heatray
//
//  Created by Cody White on 7/27/15.
//  Copyright (c) 2015 Cody White. All rights reserved.
//

#include "ConfigVariables.h"
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
    #define strcpy strcpy_s
#endif

namespace config
{

static const char* const s_rootConfigNodeName = "HeatRayConfig";

Variable *ConfigVariables::m_configVariables[ConfigVariables::kNumConfigVariables];
ConfigVariables::VariableMap ConfigVariables::m_configVariableMap;

ConfigVariables::ConfigVariables()
{
    if (m_configVariableMap.empty())
    {
        // Create the config variables from the X macro.
        
        #define X(variableGroup, variableName, type, defaultValue)    \
            m_configVariables[k##variableName] = new type(#variableName, defaultValue);
        
            HEATRAY_CONFIG_VARIABLES
        
        #undef X
        
        #define X(variableGroup, variableName, type, defaultValue) \
            m_configVariableMap[#variableGroup].push_back(m_configVariables[k##variableName]);
        
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
            VariableMap::iterator iter = m_configVariableMap.begin();
            for (; iter != m_configVariableMap.end(); ++iter)
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
                        variable->Read(element);
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
    VariableMap::iterator iter = m_configVariableMap.begin();
    for (; iter != m_configVariableMap.end(); ++iter)
    {
        std::string groupName = iter->first;
        tinyxml2::XMLElement *groupNode = file.NewElement(groupName.c_str());
        
        // Write the variables for this config group.
        for (size_t ii = 0; ii < iter->second.size(); ++ii)
        {
            const Variable *variable = iter->second[ii];
            
            tinyxml2::XMLElement *newVariable = file.NewElement(variable->name.c_str());
            variable->Write(newVariable);
            groupNode->InsertEndChild(newVariable);
        }
        
        rootElement->InsertEndChild(groupNode);
    }
    
    return (file.SaveFile(filename.c_str()) == tinyxml2::XML_NO_ERROR);
}

const Variable *ConfigVariables::GetVariable(const ConfigVariable &variable) const
{
    assert(variable < kNumConfigVariables);
    return m_configVariables[variable];
}

Variable *ConfigVariables::GetVariable(const ConfigVariable &variable)
{
    assert(variable < kNumConfigVariables);
    return m_configVariables[variable];
}

} // namespace config