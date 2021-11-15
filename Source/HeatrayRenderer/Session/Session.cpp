//
//  Session.cpp
//  Heatray
//
//

#include "Session.h"

#include <Utility/Log.h>

#include <tinyxml2/tinyxml2.h>

#include <unordered_map>
#include <vector>

enum class VariableType {
    kInt,
    kUInt,
    kFloat,
    kBool,
    kString
};

struct Variable {
    // Supported session variables are stored within the following union.
    union Value
    {
        Value() {}
        Value(int integer) { i = integer; }
        Value(uint32_t uinteger) { u = uinteger; }
        Value(bool boolean) { b = boolean; }
        Value(float decimal) { f = decimal; }
        Value(const char* string) { strcpy(c, string); }

        int		     i = 0;
        uint32_t     u;
        bool		 b;
        float		 f;
        char		 c[512];
    };

    explicit Variable(const std::string &varName, VariableType varType, Value varDefaultValue) :
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

// Table of all session variables in the system along with their default values.
static Variable g_sessionVariables[] =
{
    HEATRAY_SESSION_VARIABLES
    Variable("INVALID_VALUE", VariableType::kBool, (Variable::Value)10)
};

#undef X

// Group the session variables based on their specified "VariableGroup" defined in HEATRAY_SESSION_VARIABLES.
typedef std::unordered_map<std::string, std::vector<Variable*>> VariableMap;
static VariableMap g_sessionVariableMap;

Session::Session()
{
    if (g_sessionVariableMap.empty()) {
        // Read the session variables from the X macro.

#define X(variableGroup, variableName, type, defaultValue) \
        g_sessionVariableMap[#variableGroup].push_back(&(g_sessionVariables[k##variableName]));

        HEATRAY_SESSION_VARIABLES

#undef X
    }
}

Session::~Session()
{
}

bool Session::parseSessionFile(const std::string& filename)
{
    bool result = true;

    // Read each known element of the XML session file.
    const tinyxml2::XMLElement* rootSessionNode = nullptr;
    tinyxml2::XMLDocument xmlSessionFile;

    tinyxml2::XMLError error = xmlSessionFile.LoadFile(filename.c_str());
    if (error != tinyxml2::XML_SUCCESS) {
        xmlSessionFile.PrintError();
        LOG_ERROR("Unable to load session file %s", filename.c_str());
        result = false;
    }
    else {
        rootSessionNode = xmlSessionFile.FirstChildElement(s_rootSessionNodeName);
        if (rootSessionNode == nullptr) {
            LOG_ERROR("Unable to load root session node %s in session file %s", s_rootSessionNodeName, filename.c_str());
            result = false;
        } else {
            VariableMap::iterator iter = g_sessionVariableMap.begin();
            for (auto &iter : g_sessionVariableMap) {
                // Read the specific node for this session group.
                const tinyxml2::XMLElement* groupNode = rootSessionNode->FirstChildElement(iter.first.c_str());
                if (!groupNode) {
                    LOG_ERROR("Unable to read group '%s' from session file", iter.first.c_str());
                    continue;
                }

                // Read each variable in this group. Every variable will have an attribute attached
                // to it that contains the final value of the session variable.
                for (size_t ii = 0; ii < iter.second.size(); ++ii) {
                    Variable* variable = iter.second[ii];
                    const tinyxml2::XMLElement* element = groupNode->FirstChildElement(variable->name.c_str());

                    switch (variable->type) {
                        case VariableType::kInt:
                        {
                            element->QueryAttribute(s_attributeName, &variable->value.i);
                            break;
                        }
                        case VariableType::kUInt:
                        {
                            element->QueryAttribute(s_attributeName, &variable->value.u);
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
                    }
                }
            }
        }
    }

    return result;
}

bool Session::writeSessionFile(const std::string& filename) const
{
    tinyxml2::XMLDocument file;
    tinyxml2::XMLElement* rootElement = file.NewElement(s_rootSessionNodeName);
    file.InsertFirstChild(rootElement);

    // Create an XML element for each group with a sub element for each variable.	
    for (auto &iter : g_sessionVariableMap) {
        std::string groupName = iter.first;
        tinyxml2::XMLElement* groupNode = file.NewElement(groupName.c_str());

        // Write the variables for this group.
        for (size_t ii = 0; ii < iter.second.size(); ++ii) {
            const Variable* variable = iter.second[ii];

            tinyxml2::XMLElement* newVariable = file.NewElement(variable->name.c_str());

            switch (variable->type) {
                case VariableType::kInt:
                {
                    newVariable->SetAttribute(s_attributeName, variable->value.i);
                    break;
                }
                case VariableType::kUInt:
                {
                    newVariable->SetAttribute(s_attributeName, variable->value.u);
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
                default:
                {
                    assert(0 && "Unimplemented session variable type");
                    break;
                }
            }

            groupNode->InsertEndChild(newVariable);
        }

        rootElement->InsertEndChild(groupNode);
    }

    bool success = (file.SaveFile(filename.c_str()) == tinyxml2::XML_NO_ERROR);
    if (success) {
        LOG_INFO("Saved Heatray session to %s", filename.c_str());
    } else {
        LOG_ERROR("Unable to save Heatray session file");
    }
    return success;
}

void Session::getVariableValue(const SessionVariable& variable, int& value) const
{
    value = g_sessionVariables[variable].value.i;
}

void Session::getVariableValue(const SessionVariable& variable, uint32_t& value) const
{
    value = g_sessionVariables[variable].value.u;
}

void Session::getVariableValue(const SessionVariable& variable, bool& value) const
{
    value = g_sessionVariables[variable].value.b;
}

void Session::getVariableValue(const SessionVariable& variable, float& value) const
{
    value = g_sessionVariables[variable].value.f;
}

void Session::getVariableValue(const SessionVariable& variable, std::string& value) const
{
    value = std::string(g_sessionVariables[variable].value.c);
}

void Session::setVariableValue(const SessionVariable& variable, int value) const
{
    g_sessionVariables[variable].value.i = value;
}

void Session::setVariableValue(const SessionVariable& variable, uint32_t value) const
{
    g_sessionVariables[variable].value.u = value;
}

void Session::setVariableValue(const SessionVariable& variable, bool value) const
{
    g_sessionVariables[variable].value.b = value;
}

void Session::setVariableValue(const SessionVariable& variable, float value) const
{
    g_sessionVariables[variable].value.f = value;
}

void Session::setVariableValue(const SessionVariable& variable, const std::string& value) const
{
    strcpy(g_sessionVariables[variable].value.c, value.c_str());
}
