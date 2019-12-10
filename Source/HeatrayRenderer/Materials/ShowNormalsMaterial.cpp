#include "ShowNormalsMaterial.h"

#include <Utility/ShaderCodeLoader.h>

void ShowNormalsMaterial::build()
{
    m_program = util::buildShader(m_vertexShader, m_shader, "ShowNormals");

    // NOTE: the association of the program and the uniform block needs to happen in the calling code.
    // This is because there is no RLprimitive at the material level to properly bind.
}