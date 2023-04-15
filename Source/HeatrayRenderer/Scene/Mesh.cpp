#include "Mesh.h"

#include "MeshProvider.h"

#include <HeatrayRenderer/Materials/Material.h>
#include <HeatrayRenderer/Materials/PhysicallyBasedMaterial.h>
#include <Utility/Log.h>
#include <Utility/Util.h>

#include <simd/simd.h>

Mesh::Mesh(MeshProvider* meshProvider,
           std::vector<std::shared_ptr<Material>>& materials,
           const simd::float4x4& transform,
           MTL::Device* device)
{
    m_materials = std::move(materials);

    // Build the materials.
    for (auto& material : m_materials) {
        material->build();
    }

    {
        LOG_INFO("Building Mesh data for provider %s", meshProvider->name().data());
        m_vertexBuffers.resize(meshProvider->GetVertexBufferCount());
        for (size_t bufferIndex = 0; bufferIndex < m_vertexBuffers.size(); ++bufferIndex) {
            MTL::Buffer* buffer = device->newBuffer(meshProvider->GetVertexBufferSize(bufferIndex), MTL::ResourceStorageModeShared);
            meshProvider->FillVertexBuffer(bufferIndex, static_cast<uint8_t*>(buffer->contents()));
            m_vertexBuffers[bufferIndex] = buffer;
        }
        
        m_indexBuffers.resize(meshProvider->GetIndexBufferCount());
        for (size_t bufferIndex = 0; bufferIndex < m_indexBuffers.size(); ++bufferIndex) {
            size_t indicesSize = meshProvider->GetIndexBufferSize(bufferIndex);
            if (indicesSize == 0) {
                LOG_WARNING("Found a 0-sized index buffer - skipping.");
                continue;
            }
            MTL::Buffer* buffer = device->newBuffer(indicesSize, MTL::ResourceStorageModeShared);
            meshProvider->FillIndexBuffer(bufferIndex, static_cast<uint8_t*>(buffer->contents()));
            m_indexBuffers[bufferIndex] = buffer;
        }
    }
    
    // Now that we've loaded all mesh data, get the submeshes info.
    m_submeshes.resize(meshProvider->GetSubmeshCount());
    for (int submeshIndex = 0; submeshIndex < meshProvider->GetSubmeshCount(); ++submeshIndex) {
        Submesh& submesh = m_submeshes[submeshIndex];
        submesh = meshProvider->GetSubmesh(submeshIndex);
    }
    
//
//    m_submeshes.resize(meshProvider->GetSubmeshCount());
//    for (int ii = 0; ii < meshProvider->GetSubmeshCount(); ++ii) {
//        MeshProvider::Submesh submesh = meshProvider->GetSubmesh(ii);
//        Mesh::Submesh & rlSubmesh = m_submeshes[ii];
//
//        // If submesh material index is set, use it to look up the material
//        if (submesh.materialIndex != -1) {
//            rlSubmesh.material = m_materials[submesh.materialIndex];
//        } else {
//            // Else use the index of the submesh itself, or material 0.
//            rlSubmesh.material = m_materials.size() > 1 ? m_materials[ii] : m_materials[0];
//        }
//
//        std::shared_ptr<Material> material = rlSubmesh.material;
//
//        rlSubmesh.primitive = openrl::Primitive::create();
//        rlSubmesh.primitive->attachProgram(material->program());
//        rlSubmesh.primitive->bind();
//
//        // If this material has a uniform block, bind it here.
//        RLint uniformBlockIndex = material->program()->getUniformBlockIndex("Material"); // Yeah - this is saying all shader's should use the name "Material" for their uniform blocks.
//        if (uniformBlockIndex != -1) {
//            material->program()->setUniformBlock(uniformBlockIndex, material->uniformBlock()->buffer());
//        }
//
//        // Let the system setup any required data for this primitive.
//        materialCreatedCallback(material->program());
//
//        int worldFromEntityLocation = material->program()->getUniformLocation("worldFromEntity");
//
//        rlSubmesh.transform = submesh.localTransform * transform;
//        float determinant = glm::determinant(rlSubmesh.transform);
//        if (determinant < 0.0) {
//            rlFrontFace(RL_CW);
//        } else {
//            rlFrontFace(RL_CCW);
//        }
//
//        // If this material is setup to do alpha masking (and is a PBR material), then tell RL that this is not an occluder so that
//        // occlusion rays will still run the shader code.
//        if (material->type() == Material::Type::PBR) {
//            std::shared_ptr<PhysicallyBasedMaterial> pbrMaterial = std::static_pointer_cast<PhysicallyBasedMaterial>(material);
//            if (pbrMaterial->parameters().alphaMask) {
//                rlPrimitiveParameter1i(RL_PRIMITIVE, RL_PRIMITIVE_IS_OCCLUDER, RL_FALSE);
//            }
//        }
//
//        material->program()->setMatrix4fv(worldFromEntityLocation, &(rlSubmesh.transform[0][0]));
//
//        for (int jj = 0; jj < submesh.vertexAttributeCount; ++jj) {
//            auto & attribute = submesh.vertexAttributes[jj];
//            int attributeLocation = -1;
//            switch (attribute.usage) {
//                case VertexAttributeUsage_Position:
//                    attributeLocation = material->program()->getAttributeLocation("positionAttribute");
//                    break;
//                case VertexAttributeUsage_Normal:
//                    attributeLocation = material->program()->getAttributeLocation("normalAttribute");
//                    break;
//                case VertexAttributeUsage_TexCoord:
//                    attributeLocation = material->program()->getAttributeLocation("texCoordAttribute");
//                    break;
//                case VertexAttributeUsage_Tangents:
//                    attributeLocation = material->program()->getAttributeLocation("tangentAttribute");
//                    break;
//                case VertexAttributeUsage_Bitangents:
//                    attributeLocation = material->program()->getAttributeLocation("bitangentAttribute");
//                    break;
//                case VertexAttributeUsage_Colors:
//                    attributeLocation = material->program()->getAttributeLocation("colorAttribute");
//                    break;
//                default:
//                    LOG_ERROR("Unknown vertex attribute usage %d for submesh %s\n", attribute.usage, submesh.name.c_str());
//            }
//            if (attributeLocation != -1) {
//                m_vertexBuffers[attribute.buffer]->setAsVertexAttribute(attributeLocation, attribute.componentCount, RL_FLOAT, attribute.stride, attribute.offset);
//            }
//        }
//
//        switch (submesh.drawMode) {
//            case DrawMode::Triangles:
//                rlSubmesh.mode = RL_TRIANGLES;
//                break;
//            case DrawMode::TriangleStrip:
//                rlSubmesh.mode = RL_TRIANGLE_STRIP;
//                break;
//            default:
//                LOG_ERROR("Unsupported draw mode for submesh %s!\n", submesh.name.c_str());
//                break;
//        }
//
//        rlSubmesh.elementCount = submesh.elementCount;
//        rlSubmesh.offset = submesh.indexOffset;
//
//        if (m_indexBuffers[submesh.indexBuffer]) {
//            LOG_INFO("\tSubmitting %s to OpenRL", submesh.name.c_str());
//            m_indexBuffers[submesh.indexBuffer]->bind();
//            RLFunc(rlDrawElements(rlSubmesh.mode, rlSubmesh.elementCount, RL_UNSIGNED_INT, rlSubmesh.offset));
//            rlSubmesh.primitive->unbind();
//        }
//    }
}

void Mesh::destroy() {
    for (MTL::Buffer* buffer : m_vertexBuffers) {
        buffer->release();
    }
    for (MTL::Buffer* buffer : m_indexBuffers) {
        buffer->release();
    }
    m_submeshes.clear();
    m_materials.clear();
}

