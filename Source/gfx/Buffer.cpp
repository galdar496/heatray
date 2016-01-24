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
    
Buffer::Buffer(const RLenum target, const RLenum usage) :
    m_buffer(RL_NULL_BUFFER),
    m_target(target),
    m_usage(usage),
    m_size(0)
{
}
    
Buffer::Buffer(const Buffer &other)
{
	m_buffer = other.m_buffer;
    m_target = other.m_target;
    m_usage  = other.m_usage;
    m_size   = other.m_size;
}

Buffer::~Buffer()
{
    Destroy();
}

bool Buffer::Load(const void *data, const size_t size, const char *name)
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
        returnValue = true;
        CheckRLErrors();
    }
    
    return returnValue;
}
    
void Buffer::Destroy()
{
    if (m_buffer != RL_NULL_BUFFER)
    {
        rlDeleteBuffers(1, &m_buffer);
        m_buffer = RL_NULL_BUFFER;
    }
}
    
void Buffer::SetTarget(const RLenum target)
{
    m_target = target;
}

void Buffer::SetUsage(const RLenum usage)
{
    m_usage = usage;
}
    
void Buffer::Bind() const
{
	rlBindBuffer(m_target, m_buffer);
}

void Buffer::Unbind() const
{
	rlBindBuffer(m_target, RL_NULL_BUFFER);
}

void Buffer::SetAsVertexAttribute(RLint location, RLint numComponents, RLenum dataType, RLsize stride, RLsize offset) const
{
	rlBindBuffer(m_target, m_buffer);
    rlVertexAttribBuffer(location, numComponents, dataType, RL_FALSE, stride, offset);
    rlBindBuffer(m_target, RL_NULL_BUFFER);
    
    CheckRLErrors();
}
    
RLbuffer Buffer::GetBuffer() const
{
    return m_buffer;
}
    
void Buffer::UnmapBuffer() const
{
   rlUnmapBuffer(m_target);
}
    
} // namespace gfx