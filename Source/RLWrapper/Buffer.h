//
//  Buffer.h
//  Heatray
//
//  
//
//

#pragma once

#include "Error.h"

#include <OpenRL/rl.h>

namespace openrl {

//
// Abstracts OpenRL buffer objects.
//

class Buffer
{
public:
    ~Buffer() {
        if (m_buffer != RL_NULL_BUFFER) {
            RLFunc(rlDeleteBuffers(1, &m_buffer));
            m_buffer = RL_NULL_BUFFER;
        }
    }
    Buffer(const Buffer& other) = default;
    Buffer& operator=(const Buffer& other) = default;

    //-------------------------------------------------------------------------
    // Generates an OpenRL buffer and returns a shared_ptr to it.
    static std::shared_ptr<Buffer> create(const RLenum target, const void* data, const size_t sizeInBytes, const char* name = "<unnamed>")
    {
        Buffer* buffer = new Buffer(target, sizeInBytes, name);
        buffer->modify(data, sizeInBytes);
        return std::shared_ptr<Buffer>(buffer);
    }

    //-------------------------------------------------------------------------
    // Modify the buffer contents. The new contents MUST be the same size as the previous ones.
    void modify(const void* data, size_t sizeInBytes) {
        assert(valid());

        // Bind the buffer as our current target and load data into it.
        RLFunc(rlBindBuffer(m_target, m_buffer));
        RLFunc(rlBufferData(m_target, sizeInBytes, data, m_usage));
        RLFunc(rlBindBuffer(m_target, RL_NULL_BUFFER));

        m_sizeInBytes = sizeInBytes;
    }

    //-------------------------------------------------------------------------
    // Getters and setters for various Buffer properties.
    inline bool valid() const { return (m_buffer != RL_NULL_BUFFER); }
    inline RLbuffer buffer() const { return m_buffer; }
    inline void setTarget(const RLenum target) { m_target = target; }
    inline void setUsage(const RLenum usage) { m_usage = usage; }
    inline void bind() const { RLFunc(rlBindBuffer(m_target, m_buffer)); }
    inline void unbind() const { RLFunc(rlBindBuffer(m_target, RL_NULL_BUFFER)); }
    inline void setAsVertexAttribute(RLint location, RLint numComponents, RLenum dataType, RLsize strideInBytes, RLsize offsetInBytes) const
    {
        RLFunc(rlBindBuffer(m_target, m_buffer));
        RLFunc(rlVertexAttribBuffer(location, numComponents, dataType, RL_FALSE, strideInBytes, offsetInBytes));
        RLFunc(rlBindBuffer(m_target, RL_NULL_BUFFER));
    }

    template<typename T>
    inline T* mapBuffer(RLenum access = RL_READ_ONLY) const
    {
        return RLFunc(static_cast<T*>(rlMapBuffer(m_target, access)));
    }
    inline void unmapBuffer() const { RLFunc(rlUnmapBuffer(m_target)); }

private:
    explicit Buffer(const RLenum target, const size_t sizeInBytes, const char* name)
        : m_target(target)
        , m_sizeInBytes(sizeInBytes)
    {
        RLFunc(rlGenBuffers(1, &m_buffer));
        RLFunc(rlBindBuffer(m_target, m_buffer));
        RLFunc(rlBufferParameterString(m_target, RL_BUFFER_NAME, name));
        RLFunc(rlBindBuffer(m_target, RL_NULL_BUFFER));
    }

    RLbuffer m_buffer      = RL_NULL_BUFFER;   // The RL buffer object itself.
    RLenum   m_target      = RL_ARRAY_BUFFER;  // Target type.
    RLenum   m_usage       = RL_STATIC_DRAW;   // How is this buffer to be used? (RL_STATIC_DRAW...).
    size_t   m_sizeInBytes = 0;                // Size of the buffer (bytes).
};

} // namespace openrl.

