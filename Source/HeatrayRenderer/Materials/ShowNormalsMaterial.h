//
//  ShowNormalsMaterial.h
//  Heatray
//
//  Debug material which shows normals.
//

#pragma once

#include "Material.h"

#include <RLWrapper/Shader.h>
#include <Utility/ShaderCodeLoader.h>

class ShowNormalsMaterial : public Material
{
public:
    ShowNormalsMaterial() = default;
    virtual ~ShowNormalsMaterial() = default;

    void build(); // This material has no parameters.

private:
    static constexpr char const * m_shader = "showNormals.rlsl"; ///< Shader file with corresponding material code.
};
