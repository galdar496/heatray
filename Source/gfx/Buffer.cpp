//
//  Buffer.cpp
//  Heatray
//
//  Created by Cody White on 5/10/14.
//
//

#include "Buffer.h"
#include "../utility/util.h"

namespace gfx
{
    
/// Default constructor.
Buffer::Buffer(const RLenum target, const RLenum usage) :
    m_buffer(RL_NULL_BUFFER),
    m_target(target),
    m_usage(usage),
    m_size(0)
{
}
    
/// Copy constructor.
Buffer::Buffer(const Buffer &other)
{
	m_buffer = other.m_buffer;
    m_target = other.m_target;
    m_usage  = other.m_usage;
    m_size   = other.m_size;
}

/// Default destructor.
Buffer::~Buffer()
{
    destroy();
}

/// Generate the buffer id and load the data. Returns true on a successful load.
bool Buffer::load(const void *data, const size_t size, const char *name)
{
    bool returnValue = false;
    //if (data != NULL)
    {
        // Generate the buffer if necessary.
        if (m_buffer == RL_NULL_BUFFER)
        {
            rlGenBuffers(1, &m_buffer);
        }
        
        // Bind the buffer as our current target and load data into it.
        rlBindBuffer(m_target, m_buffer);
        rlBufferData(m_target, size, data, m_usage);
        rlBufferParameterString(m_target, RL_BUFFER_NAME, name);
        rlBindBuffer(m_target, RL_NULL_BUFFER);
        
        m_size = size;
        returnValue = !util::CheckRLErrors("Buffer::load() -- bind and load buffer data");
    }
    
    return returnValue;
}
    
/// Destroy the buffer.
void Buffer::destroy()
{
    if (m_buffer != RL_NULL_BUFFER)
    {
        rlDeleteBuffers(1, &m_buffer);
        m_buffer = RL_NULL_BUFFER;
    }
}
    
/// Change the target type for this buffer.
void Buffer::setTarget(const RLenum target)
{
    m_target = target;
}

// Change the usage type for this buffer.
void Buffer::setUsage(const RLenum usage)
{
    m_usage = usage;
}
    
/// Bind this buffer for use at its stored target point.
void Buffer::bind() const
{
	rlBindBuffer(m_target, m_buffer);
}

/// Unbind this buffer from use at its stored target point.
void Buffer::unbind() const
{
	rlBindBuffer(m_target, RL_NULL_BUFFER);
}

/// Set this buffer as an attribute for a vertex shader.
void Buffer::setAsVertexAttribute(RLint location, RLint numComponents, RLenum dataType, RLsize stride, RLsize offset) const
{
	rlBindBuffer(m_target, m_buffer);
    rlVertexAttribBuffer(location, numComponents, dataType, RL_FALSE, stride, offset);
    rlBindBuffer(m_target, RL_NULL_BUFFER);
    
    util::CheckRLErrors("Buffer::setAsVertexAttribute() - setting buffer");
}
    
/// Get the internal buffer.
RLbuffer Buffer::getBuffer() const
{
    return m_buffer;
}
    
/// Unmap a buffer that is currently mapped to the CPU.
void Buffer::unmapBuffer() const
{
   rlUnmapBuffer(m_target);
}
    
} // namespace gfx