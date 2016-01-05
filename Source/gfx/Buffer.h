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
   
/// Abstracts OpenRL buffer objects.
class Buffer
{
    public:
       
       /// Default constructor.
       explicit Buffer(const RLenum target = RL_ARRAY_BUFFER, // IN: Type of buffer to create.
                       const RLenum usage = RL_STATIC_DRAW);  // IN: How will this buffer be used?
    
       /// Copy constructor.
       Buffer(const Buffer &other);
        
       /// Default destructor.
       ~Buffer();
    
       /// Generate the buffer id and load the data. Returns true on a successful load.
       bool load(const void *data,              // IN: The data to populate the buffer with.
                 const size_t size,             // IN: The size of the data in bytes.
                 const char *name = "<unnamed>" // IN: Name of the buffer.
                );
   
       /// Destroy the buffer.
       void destroy();
    
       /// Change the target type for this buffer.
       void setTarget(const RLenum target // IN: Type of buffer to change this object to.
    				  );

       /// Change the usage type for this buffer.
       void setUsage(const RLenum usage // IN: How will this buffer be used?
                    );
    
       /// Bind this buffer for use at its stored target point.
       void bind() const;
    
       /// Unbind this buffer from use at its stored target point.
       void unbind() const;
       
       /// Set this buffer as an attribute for a vertex shader.
       void setAsVertexAttribute(RLint  location, 	   // IN: The location in of the vertex shader attribute to bind this buffer to.
                                 RLint  numComponents, // IN: Number of components in one chunk of data (e.g. 3 for a vec3f).
                                 RLenum dataType, 	   // IN: Data type of the buffer data (RL_FLOAT, RL_INT, ...).
                                 RLsize stride,   	   // IN: Stride (in bytes) between one data chunk and another (e.g. 3 * sizeof(vec3f) for a vec3f).
                                 RLsize offset    	   // IN: Offset (in bytes) of where to start using the buffer.
                                ) const;
    
       /// Get the internal buffer.
       RLbuffer getBuffer() const;
    
       /// Map a buffer to the CPU. This function is templated to support any type of casting.
       template<typename T>
       inline T *mapBuffer(RLenum access = RL_WRITE_ONLY // IN: Access type to use for mapping the buffer.
                          ) const
       {
          return static_cast<T *>(rlMapBuffer(m_target, access));
       }
    
       /// Unmap a buffer that is currently mapped to the CPU.
       void unmapBuffer() const;
        
    
    private:
    
    	// Do not implement this as a buffer should not be assignable.
    	Buffer & operator=(const Buffer &other);
       
       // Member variables.
       RLbuffer m_buffer;   // The RL buffer object itself.
       RLenum   m_target; 	// Target type.
       RLenum   m_usage;    // How is this buffer to be used? (RL_STATIC_DRAW...).
       size_t   m_size;     // Size of the buffer (bytes).
};
   
} // namespace gfx


