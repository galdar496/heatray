#include "ShaderCodeLoader.h"

#include "FileIO.h"

#include <RLWrapper/Shader.h>

#include <assert.h>
#include <set>

namespace util
{

using HashTable = std::set<std::string>; // key = shader name.
constexpr char const * kShaderDir = "Resources/shaders/";

// This function recursively adds the shader code all together. For each file it reads, it searches for "#include" and then
// calls itself with the new filename etc etc. Note that it just looks for "#include" directly, this means that if there is an include
// statement anywhere (even in comments) then it will try to process it!
bool loadShaderSourceFileRecursive(const std::string &filename, std::vector<std::string>& finalSourceCode, HashTable &filesRead)
{
    std::string sourceCode;
    if (readTextFile(filename.c_str(), sourceCode))
    {
        // Check the source code for any "#include" files.
        size_t offset = 0;
        while ((offset = sourceCode.find("#include", 0)) != std::string::npos)
        {
            // Should be formatted something like: #include "shader.rlsl"
            size_t nameStartOffset = sourceCode.find("\"", offset);
            assert(nameStartOffset != std::string::npos);
            size_t nameEndOffset = sourceCode.find("\"", nameStartOffset + 1);
            assert(nameEndOffset != std::string::npos);
            std::string shaderName = sourceCode.substr(nameStartOffset + 1, nameEndOffset - nameStartOffset - 1);

            // Get rid of the "#include "blah.rlsl" in the source code.
            sourceCode.erase(offset, nameEndOffset - offset + 1);

            if (filesRead.find(shaderName) != filesRead.end())
            {
                // We've already added this source file to the final shader code.
                continue;
            }
            filesRead.insert(shaderName);

            if (!loadShaderSourceFileRecursive((std::string(kShaderDir) + shaderName).c_str(), finalSourceCode, filesRead))
            {
                return false;
            }
        }

        finalSourceCode.push_back(sourceCode);

        return true;
    }

    return false;
}

bool loadShaderSourceFile(const char* filepath, std::vector<std::string>& finalSourceCode)
{
    HashTable filesRead;
    std::string fullPath = std::string(kShaderDir) + std::string(filepath);
    return loadShaderSourceFileRecursive(fullPath, finalSourceCode, filesRead);
}

openrl::Program buildShader(const char* vertexShaderPath, const char* rayShaderPath, const char *name, std::string const & shaderPrefix)
{
    assert(vertexShaderPath);
    assert(rayShaderPath);
    assert(name);

    std::string programName(name);

    std::vector<std::string> vertexShaderSource;
    if (shaderPrefix.size())
    {
        vertexShaderSource.push_back(shaderPrefix);
    }
    util::loadShaderSourceFile(vertexShaderPath, vertexShaderSource);
    openrl::Shader vertex;
    if (!vertex.createFromMultipleStrings(vertexShaderSource, RL_VERTEX_SHADER, (programName + " vertex shader").c_str()))
    {
        assert(0 && "Unable to create vertex shader");
    }

    std::vector<std::string> rayShaderSource;
    if (shaderPrefix.size())
    {
        rayShaderSource.push_back(shaderPrefix);
    }
    util::loadShaderSourceFile(rayShaderPath, rayShaderSource);
    openrl::Shader ray;
    if (!ray.createFromMultipleStrings(rayShaderSource, RL_RAY_SHADER, (programName + " ray shader").c_str()))
    {
        assert(0 && "Unable to create ray shader");
    }

    openrl::Program program;
    program.create();
    program.attach(vertex);
    program.attach(ray);
    if (!program.link((programName + " program").c_str()))
    {
        assert(0 && "Unable to create program");
    }

    return program;
}

} // namespace util.