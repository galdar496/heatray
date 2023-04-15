//
//  Submesh.h
//  Heatray5 macOS
//
//  Created by Cody White on 4/15/23.
//

#pragma once

#include "VertexAttribute.h"

#include <Utility/Util.h>

#include <simd/simd.h>
#include <string>

enum class DrawMode {
    Triangles,
    TriangleStrip,
};

struct Submesh {
    int vertexAttributeCount = 0;
    VertexAttribute vertexAttributes[util::to_underlying(VertexAttributeUsage::Count)];
    size_t indexBuffer = 0;
    size_t indexOffset = 0;
    size_t elementCount = 0;
    DrawMode drawMode = DrawMode::Triangles;
    int materialIndex = -1;
    simd::float4x4 localTransform = matrix_identity_float4x4;
    std::string name;
};
