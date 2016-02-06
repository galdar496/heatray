//
//  Buffer.h
//  Heatray
//
//  Created by Cody White on 5/10/14.
//
//

#pragma once

#include <OpenRL/rl.h>

namespace gfx
{
   
///
/// Abstracts OpenRL buffer objects.
///

class Buffer
{
    public:
    
        ///
        /// Constructor.
        ///
        /// @param target Type of buffer to create.
        /// @param How will this buffer be used?
        ///
        explicit Buffer(const RLenum target = RL_ARRAY_BUFFER, const RLenum usage = RL_STATIC_DRAW);

        Buffer(const Buffer &other);
        ~Buffer();

        ///
        /// Generate the buffer id and load the data. Returns true on a successful load.
        ///
        /// @param data The data to populate the buffer with.
        /// @param size The size of the data in bytes.
        /// @param name Name of the buffer.
        ///
        bool Load(const void *data, const size_t size, const char *name = "<unnamed>");

        ///
        /// Destroy the buffer.
        ///
        void Destroy();

        ///
        /// Change the target type for this buffer.
        ///
        /// @param target Target type to set for this buffer.
        ///
        void SetTarget(const RLenum target);

        ///
        /// Change the usage type for this buffer.
        ///
        /// @param usage How will this buffer be used?
        ///
        void SetUsage(const RLenum usage);

        ///
        /// Bind this buffer for use at its stored target point.
        ///
        void Bind() const;

        ///
        /// Unbind this buffer from use at its stored target point.
        ///
        void Unbind() const;

        ///
        /// Set this buffer as an attribute for a vertex shader.
        ///
        /// @param location The location in of the vertex shader attribute to bind this buffer to.
        /// @param numComponents Number of components in one chunk of data (e.g. 3 for a vec3f).
        /// @param dataType Data type of the buffer data (RL_FLOAT, RL_INT, ...).
        /// @param stride Stride (in bytes) between one data chunk and another (e.g. 3 * sizeof(vec3f) for a vec3f).
        /// @param offset Offset (in bytes) of where to start using the buffer.
        ///
        void SetAsVertexAttribute(RLint location, RLint numComponents, RLenum dataType, RLsize stride, RLsize offset) const;

        ///
        /// Get the internal buffer.
        ///
        /// @return OpenRL buffer that is internal to this object.
        ///
        RLbuffer GetBuffer() const;

        ///
        /// Map a buffer to the CPU. This function is templated to support any type of casting.
        ///
        /// @param access Access type to use for mapping the buffer. Default is RL_READ_ONLY.
        ///
        /// @return Type-casted pointer to the buffer in CPU memory.
        ///
        template<typename T>
        inline T *MapBuffer(RLenum access = RL_READ_ONLY) const
        {
          return static_cast<T *>(rlMapBuffer(m_target, access));
        }

        ///
        /// Unmap a buffer that is currently mapped to the CPU.
        ///
        void UnmapBuffer() const;

        ///
        /// If true, this buffer is valid and ready for use.
        ///
        bool IsValid() const;

    private:

        Buffer & operator=(const Buffer &other) = delete;

        // Member variables.
        RLbuffer m_buffer;   ///< The RL buffer object itself.
        RLenum   m_target; 	 ///< Target type.
        RLenum   m_usage;    ///< How is this buffer to be used? (RL_STATIC_DRAW...).
        size_t   m_size;     ///< Size of the buffer (bytes).
};
   
} // namespace gfx


