//
//  ShaderLoader.h
//  Heatray
//
//  Loads shader files from disk. Importantly, this adds support to the shader language for proper
//  "#include" functionality directly in the shader language.
//
//

#pragma once

#include <RLWrapper/Program.h>

#include <string>
#include <vector>

namespace util
{

// Note that the result is a vector of strings to be sent to OpenRL for final compilation. All string concatenated
// together make up the final shader.
bool loadShaderSourceFile(const char* filepath, std::vector<std::string> &finalSourceCode);

openrl::Program buildShader(const char *vertexShaderPath, const char *rayShaderPath, const char* name, std::string const & shaderPrefix = "");

} // namespace util.