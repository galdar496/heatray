#include "ShaderCodeLoader.h"

#include "FileIO.h"

#include <RLWrapper/Shader.h>
#include <Utility/Hash.h>

#include <assert.h>
#include <set>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace util {

using HashTable = std::set<std::string>; // key = shader name.
constexpr std::string_view kShaderDir = "Resources/shaders/";

namespace {
// We keep track of all full shaders read by these utility functions. If we're asked to load
// the same shader twice, the second time will just used the cached version.
std::unordered_map<size_t, std::vector<std::string>> shaderCache;
} // empty namespace.

// This function recursively adds the shader code to the passed-in vector. For each file it reads, it searches for "#include" and then
// calls itself with the new filename etc etc. Note that it just looks for "#include" directly, this means that if there is an include
// statement anywhere (even in comments) then it will try to process it!
bool loadShaderSourceFileRecursive(const std::string_view filename, std::vector<std::string>& finalSourceCode, HashTable &filesRead)
{
    std::string sourceCode;
    if (readTextFile(filename.data(), sourceCode)) {
        // Check the source code for any "#include" files.
        size_t offset = 0;
        while ((offset = sourceCode.find("#include", 0)) != std::string::npos) {
            // Should be formatted something like: #include "shader.rlsl"
            size_t nameStartOffset = sourceCode.find("\"", offset);
            assert(nameStartOffset != std::string::npos);
            size_t nameEndOffset = sourceCode.find("\"", nameStartOffset + 1);
            assert(nameEndOffset != std::string::npos);
            std::string shaderName = sourceCode.substr(nameStartOffset + 1, nameEndOffset - nameStartOffset - 1);

            // Get rid of the "#include "blah.rlsl" in the source code.
            sourceCode.erase(offset, nameEndOffset - offset + 1);

            if (filesRead.find(shaderName) != filesRead.end()) {
                // We've already added this source file to the final shader code.
                continue;
            }
            filesRead.insert(shaderName);

            std::string nextFile{kShaderDir};
            nextFile.append(shaderName);
            if (!loadShaderSourceFileRecursive(nextFile, finalSourceCode, filesRead)) {
                return false;
            }
        }

        finalSourceCode.push_back(sourceCode);

        return true;
    }

    return false;
}

bool loadShaderSourceFile(const std::string_view filepath, std::vector<std::string>& finalSourceCode)
{
    // Check our cache first.
    size_t shaderHash = hashCombine(0, filepath);
    {
        auto iter = shaderCache.find(shaderHash);
        if (iter != shaderCache.end()) {
            for (std::string &code : iter->second) {
                finalSourceCode.push_back(code);
            }
            return true;
        }
    }

    HashTable filesRead;
    std::string fullPath;
    fullPath.append(kShaderDir);
    fullPath.append(filepath);
    bool result = loadShaderSourceFileRecursive(fullPath, finalSourceCode, filesRead);
    if (result) {
        // Update the shader cache.
        shaderCache[shaderHash] = finalSourceCode;
    }

    return result;
}

std::shared_ptr<openrl::Program> buildProgram(const std::string_view vertexShaderPath, const std::string_view rayShaderPath, const std::string_view name, const std::string_view shaderPrefix)
{
    assert(vertexShaderPath.data());
    assert(rayShaderPath.data());
    assert(name.data());
    
    std::vector<std::string> vertexShaderSource;
    if (shaderPrefix.size()) {
        vertexShaderSource.emplace_back(std::string{shaderPrefix});
    }
    util::loadShaderSourceFile(vertexShaderPath, vertexShaderSource);
    std::shared_ptr<openrl::Shader> vertex = openrl::Shader::createFromMultipleStrings(vertexShaderSource, openrl::Shader::ShaderType::kVertex, name);
    if (!vertex) {
        assert(0 && "Unable to create vertex shader");
    }

    std::vector<std::string> rayShaderSource;
    if (shaderPrefix.size()) {
        rayShaderSource.emplace_back(std::string{shaderPrefix});
    }
    util::loadShaderSourceFile(rayShaderPath, rayShaderSource);
    std::shared_ptr<openrl::Shader> ray = openrl::Shader::createFromMultipleStrings(rayShaderSource, openrl::Shader::ShaderType::kRay, name);
    if (!ray) {
        assert(0 && "Unable to create ray shader");
    }

    std::shared_ptr<openrl::Program> program = openrl::Program::create();
    program->attach(vertex);
    program->attach(ray);
    if (!program->link(name)) {
        assert(0 && "Unable to create program");
        return nullptr;
    }

    return program;
}

} // namespace util.
