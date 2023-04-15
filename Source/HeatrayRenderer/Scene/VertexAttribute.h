//
//  VertexAttribute.h
//  Heatray5 macOS
//
//  Created by Cody White on 4/15/23.
//

#pragma once

#include <stddef.h>

enum class VertexAttributeUsage : size_t {
    Position,
    Normal,
    TexCoord,
    Tangents,
    Bitangents,
    Colors,
    
    Count,
};

struct VertexAttribute {
    VertexAttributeUsage usage = VertexAttributeUsage::Position;
    int buffer = -1;
    int componentCount = 0;
    int size = 0;
    size_t offset = 0;
    int stride = 0;
};
