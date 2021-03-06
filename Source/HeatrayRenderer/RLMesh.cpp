#include "RLMesh.h"
#include "Materials/Material.h"


#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

RLMesh::RLMesh(MeshProvider* meshProvider, std::vector<Material*> materials, SetupSystemBindingsCallback callback, glm::mat4 transform)
{
    m_materials = std::move(materials);

    size_t vertexBufferCount = meshProvider->GetVertexBufferCount();
    for (size_t ii = 0; ii < vertexBufferCount; ++ii)
    {
        openrl::Buffer buffer;
        buffer.load(nullptr, meshProvider->GetVertexBufferSize(ii), "Vertex Buffer");
        buffer.bind();
        uint8_t * mapping = buffer.mapBuffer<uint8_t>(RL_READ_WRITE);
        meshProvider->FillVertexBuffer(ii, mapping);
        buffer.unmapBuffer();
        m_vertexBuffers.push_back(std::move(buffer));
    }

    size_t indexBufferCount = meshProvider->GetIndexBufferCount();
    for (size_t ii = 0; ii < indexBufferCount; ++ii)
    {
        openrl::Buffer buffer;
        buffer.setTarget(RL_ELEMENT_ARRAY_BUFFER);
        buffer.load(nullptr, meshProvider->GetIndexBufferSize(ii), "Index Buffer");
        buffer.bind();
        uint8_t * mapping = buffer.mapBuffer<uint8_t>(RL_READ_WRITE);
        meshProvider->FillIndexBuffer(ii, mapping);
        buffer.unmapBuffer();
        m_indexBuffers.push_back(std::move(buffer));
    }

    m_submeshes.resize(meshProvider->GetSubmeshCount());
    for (int ii = 0; ii < meshProvider->GetSubmeshCount(); ++ii)
    {
        MeshProvider::Submesh submesh = meshProvider->GetSubmesh(ii);
        RLMesh::Submesh & rlSubmesh = m_submeshes[ii];

        // If submesh material index is set, use it to look up the material
        if (submesh.materialIndex != -1)
        {
            rlSubmesh.material = m_materials[submesh.materialIndex];
        }
        else
        {
            // Else use the index of the submesh itself, or material 0.
            rlSubmesh.material = m_materials.size() > 1 ? m_materials[ii] : m_materials[0];
        }

        Material * material = rlSubmesh.material;

        rlSubmesh.primitive.create();
        rlSubmesh.primitive.attachProgram(material->program());
        rlSubmesh.primitive.bind();

        // If this material has a uniform block, bind it here.
        RLint uniformBlockIndex = material->program().getUniformBlockIndex("Material"); // Yeah - this is saying all shader's should use the name "Material" for their uniform blocks.
        if (uniformBlockIndex != -1) 
        {
            material->program().setUniformBlock(uniformBlockIndex, material->uniformBlock().buffer());
        }

        // Let the system setup any required data for this primitive.
        callback(material->program()); 

        int worldFromEntityLocation = material->program().getUniformLocation("worldFromEntity");

        glm::mat4 finalTransform = submesh.localTransform * transform;
        material->program().setMatrix4fv(worldFromEntityLocation, &(finalTransform[0][0]));

        for (int jj = 0; jj < submesh.vertexAttributeCount; ++jj)
        {
            auto & attribute = submesh.vertexAttributes[jj];
            int attributeLocation;
            switch (attribute.usage)
            {
                case VertexAttributeUsage_Position:
                    attributeLocation = material->program().getAttributeLocation("positionAttribute");
                    break;
                case VertexAttributeUsage_Normal:
                    attributeLocation = material->program().getAttributeLocation("normalAttribute");
                    break;
                case VertexAttributeUsage_TexCoord:
                    attributeLocation = material->program().getAttributeLocation("texCoordAttribute");
                    break;
                case VertexAttributeUsage_TangentSpace:
                    attributeLocation = material->program().getAttributeLocation("tangentSpaceAttribute");
                    break;
                default:
                    printf("Unknown vertex attribute usage %d\n", attribute.usage);
            }
            if (attributeLocation != -1)
            {
                m_vertexBuffers[attribute.buffer].setAsVertexAttribute(attributeLocation, attribute.componentCount, RL_FLOAT, attribute.stride, attribute.offset);
            }
        }
        
        switch (submesh.drawMode)
        {
            case DrawMode::Triangles:
                rlSubmesh.mode = RL_TRIANGLES;
                break;
            case DrawMode::TriangleStrip:
                rlSubmesh.mode = RL_TRIANGLE_STRIP;
                break;
            default:
                printf("Unsupported draw mode!\n");
                break;
        }

        rlSubmesh.elementCount = submesh.elementCount;
        rlSubmesh.offset = submesh.indexOffset;
        
        m_indexBuffers[submesh.indexBuffer].bind();
        RLFunc(rlDrawElements(rlSubmesh.mode, rlSubmesh.elementCount, RL_UNSIGNED_INT, rlSubmesh.offset));
        rlSubmesh.primitive.unbind();
    }
}

void RLMesh::destroy()
{
    for (openrl::Buffer& b : m_vertexBuffers)
    {
        b.destroy();
    }
    m_vertexBuffers.clear();

    for (openrl::Buffer& b : m_indexBuffers)
    {
        b.destroy();
    }
    m_vertexBuffers.clear();

    for (Submesh& s : m_submeshes)
    {
        s.primitive.destroy();
        s.material->program().destroy();
        s.material->uniformBlock().destroy();
        s.material = nullptr;
    }
    m_submeshes.clear();

    for (Material* m : m_materials)
    {
        delete m;
    }
}

