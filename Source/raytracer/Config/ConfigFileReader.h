//
//  ConfigFileReader.h
//  Heatray
//
//  Created by Cody White on 7/27/15.
//  Copyright (c) 2015 Cody White. All rights reserved.
//

#pragma once

///
/// Abstracts the reading of an XML config file.
///

#include "../../utility/tinyxml2.h"
#include <assert.h>

namespace config
{

class ConfigFileReader
{
    public:

        explicit ConfigFileReader(const tinyxml2::XMLElement *configNode) :
            m_configNode(configNode)
        {}

        ///
        /// Read the config file and populate the passed-in variable. If the config variable does not
        /// exist in the config file, then the variable's default value will be used.
        ///
        /// @param variable ConfigVariable to read from the file and update the value of.
        ///
        template<class T>
        void ReadVariableValue(const std::string &variableName, T &value) const
        {
            const tinyxml2::XMLElement *element = m_configNode->FirstChildElement(variableName.c_str());
            if (element)
            {
                ReadVariable<T>(element, value);
            }
        }

    private:

        template<class T>
        void ReadVariable(const tinyxml2::XMLElement *element, T &value) const
        {
            assert(0 && "Unimplemented ConfigVariable type");
        }

        const tinyxml2::XMLElement *m_configNode; ///< Root config node for a group of options. Should be ready by a call to SystemBase::GetConfigNodeName().
};

// Template specializations.
template<>
void ConfigFileReader::ReadVariable(const tinyxml2::XMLElement *element, int &value) const
{
    element->QueryIntText(&value);
}

template<>
void ConfigFileReader::ReadVariable(const tinyxml2::XMLElement *element, float &value) const
{
    element->QueryFloatText(&value);
}

template<>
void ConfigFileReader::ReadVariable(const tinyxml2::XMLElement *element, bool &value) const
{
    element->QueryBoolText(&value);
}

template<>
void ConfigFileReader::ReadVariable(const tinyxml2::XMLElement *element, std::string &value) const
{
    value = element->GetText();
}

} // namespace config
