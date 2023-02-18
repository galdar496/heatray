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

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace util {

//-------------------------------------------------------------------------
// Load a shader source file and resolve any "#include" statements. Each
// source file will be placed in the resulting vector.
bool loadShaderSourceFile(const std::string_view filepath, std::vector<std::string> &finalSourceCode);

//-------------------------------------------------------------------------
// Convenience function to build a program directly given shader paths.
// This function internally uses 'loadShaderSourceFile()'.
std::shared_ptr<openrl::Program> buildProgram(const std::string_view vertexShaderPath, const std::string_view rayShaderPath, const std::string_view name, const std::string_view shaderPrefix = "");

} // namespace util.
