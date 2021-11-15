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

///
/// Abstracts OpenRL buffer objects.
///

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

    ///
    /// Generate the buffer id and load the data.
    ///
    /// @param target The OpenRL target for this buffer.
    /// @param data The data to populate the buffer with.
    /// @param size The size of the data in bytes.
    /// @param name Name of the buffer.
    ///
    static std::shared_ptr<Buffer> create(const RLenum target, const void* data, const size_t size, const char* name = "<unnamed>")
    {
        Buffer* buffer = new Buffer(target, size, name);
        buffer->modify(data, size);
        return std::shared_ptr<Buffer>(buffer);
    }

    ///
    /// Modify the buffer contents. The new contents MUST be the same size as the previous ones.
    ///
    /// @param target Target type to set for this buffer.
    /// @param size The size of the data in bytes.
    ///
    void modify(const void* data, size_t size) {
        assert(valid());

        // Bind the buffer as our current target and load data into it.
        RLFunc(rlBindBuffer(m_target, m_buffer));
        RLFunc(rlBufferData(m_target, size, data, m_usage));
        RLFunc(rlBindBuffer(m_target, RL_NULL_BUFFER));

        m_size = size;
    }

    ///
    /// Change the target type for this buffer.
    ///
    /// @param target Target type to set for this buffer.
    ///
    inline void setTarget(const RLenum target) { m_target = target; }

    ///
    /// Change the usage type for this buffer.
    ///
    /// @param usage How will this buffer be used?
    ///
    inline void setUsage(const RLenum usage) { m_usage = usage; }

    ///
    /// Bind this buffer for use at its stored target point.
    ///
    inline void bind() const { RLFunc(rlBindBuffer(m_target, m_buffer)); }

    ///
    /// Unbind this buffer from use at its stored target point.
    ///
    inline void unbind() const { RLFunc(rlBindBuffer(m_target, RL_NULL_BUFFER)); }

    ///
    /// Set this buffer as an attribute for a vertex shader.
    ///
    /// @param location The location in of the vertex shader attribute to bind this buffer to.
    /// @param numComponents Number of components in one chunk of data (e.g. 3 for a vec3f).
    /// @param dataType Data type of the buffer data (RL_FLOAT, RL_INT, ...).
    /// @param stride Stride (in bytes) between one data chunk and another (e.g. 3 * sizeof(vec3f) for a vec3f).
    /// @param offset Offset (in bytes) of where to start using the buffer.
    ///
    inline void setAsVertexAttribute(RLint location, RLint numComponents, RLenum dataType, RLsize stride, RLsize offset) const
    {
        RLFunc(rlBindBuffer(m_target, m_buffer));
        RLFunc(rlVertexAttribBuffer(location, numComponents, dataType, RL_FALSE, stride, offset));
        RLFunc(rlBindBuffer(m_target, RL_NULL_BUFFER));
    }

    ///
    /// Get the internal buffer.
    ///
    /// @return OpenRL buffer that is internal to this object.
    ///
    inline RLbuffer buffer() const { return m_buffer; }

    ///
    /// Map a buffer to the CPU. This function is templated to support any type of casting.
    ///
    /// @param access Access type to use for mapping the buffer. Default is RL_READ_ONLY.
    ///
    /// @return Type-casted pointer to the buffer in CPU memory.
    ///
    template<typename T>
    inline T* mapBuffer(RLenum access = RL_READ_ONLY) const
    {
        return RLFunc(static_cast<T*>(rlMapBuffer(m_target, access)));
    }

    ///
    /// Unmap a buffer that is currently mapped to the CPU.
    ///
    inline void unmapBuffer() const { RLFunc(rlUnmapBuffer(m_target)); }

    ///
    /// If true, this buffer is valid and ready for use.
    ///
    inline bool valid() const { return (m_buffer != RL_NULL_BUFFER); }

private:
    explicit Buffer(const RLenum target, const size_t size, const char* name)
        : m_target(target)
        , m_size(size)
    {
        RLFunc(rlGenBuffers(1, &m_buffer));
        RLFunc(rlBindBuffer(m_target, m_buffer));
        RLFunc(rlBufferParameterString(m_target, RL_BUFFER_NAME, name));
        RLFunc(rlBindBuffer(m_target, RL_NULL_BUFFER));
    }

    RLbuffer m_buffer   = RL_NULL_BUFFER;   ///< The RL buffer object itself.
    RLenum   m_target   = RL_ARRAY_BUFFER; 	///< Target type.
    RLenum   m_usage    = RL_STATIC_DRAW;   ///< How is this buffer to be used? (RL_STATIC_DRAW...).
    size_t   m_size     = 0;                ///< Size of the buffer (bytes).
};

} // namespace openrl.

