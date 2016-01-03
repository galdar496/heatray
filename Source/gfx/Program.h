//
//  Program.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include <OpenRL/rl.h>
#include "Shader.h"

namespace gfx
{
    
/// Encapsulates a RLSL program.
class Program
{
    public:
    
        /// Default constructor.
        Program();
    
        /// Destructor.
        ~Program();
    
        /// Attach the specified shader. The shader must already be valid.
        bool attach(const Shader &shader // IN: Shader to attach to this program.
                   );
    
        /// Link the program. Call this after all shaders have been attached.
        bool link(const std::string &name = "Program" // IN: Name of the program (optional).
                 );
    
        /// Add a shader directly to the program.
        bool addShader(const std::string &filename, // IN: Path to the shader code.
                       const Shader::Type type      // IN: Type of the shader.
                      );
    
        /// Destroy this program.
        void destroy();
    
        /// Set an integer uniform in the program.
        void set1i(const std::string &name, // IN: Name of the uniform.
                   const int i              // IN: Value of the uniform.
                  ) const;
    
        /// Set a float uniform in the program.
        void set1f(const std::string &name, // IN: Name of the uniform.
                   const float f            // IN: Value of the uniform.
                  ) const;
    
        /// Set a 2 component float uniform in the program.
        void set2fv(const std::string &name, // IN: Name of the uniform.
                    const float *f           // IN: Value of the uniform.
                   ) const;
    
        /// Set a 2 component int uniform in the program.
        void set2iv(const std::string &name, // IN: Name of the uniform.
                    const int *i             // IN: Value of the uniform.
                   ) const;
    
        /// Set a 3 component float uniform in the program.
        void set3fv(const std::string &name, // IN: Name of the uniform.
                    const float *f           // IN: Value of the uniform.
                   ) const;
    
        /// Set a 4 component float uniform in the program.
        void set4fv(const std::string &name, // IN: Name of the uniform.
                    const float *f           // IN: Value of the uniform.
                   ) const;
    
        /// Set a 4 component int uniform in the program.
        void set4iv(const std::string &name, // IN: Name of the uniform.
                    const int *i             // IN: Value of the uniform.
                   ) const;
    
        /// Set a 4x4 matrix uniform in the program.
        void setMatrix4fv(const std::string &name, // IN: Name of the uniform.
                          const float *f           // IN: Value of the uniform.
                         ) const;
    
    	/// Set a texture uniform in the program.
    	void setTexture(const std::string &name, // IN: Name of the uniform.
                        const RLtexture &texture // IN: Value of the uniform.
                       );
    
        /// Set a primitive uniform in the program.
        void setPrimitive(const std::string &name,     // IN: Name of the uniform.
                          const RLprimitive &primitive // IN: Value of the uniform.
                         );
    
        /// Bind this program for use.
        void bind() const;
    
        /// Unbind this program from use.
        void unbind() const;
    
        /// Get a location of an attribute variable in the program.
        RLint getAttributeLocation(const std::string &name // IN: Name of the attribute variable.
                                  ) const;
    
    	/// Get access to the internal program object.
    	RLprogram getProgram() const;
        
    private:
    
        // Create the RL program.
        bool create();
    
    	// Programs are not copyable.
    	Program(const Program &other);
    	Program & operator=(const Program &other);
        
        // Member variables.
        RLprogram   m_program;      // RL program object.
        std::string m_program_name;	// The name of this program (optional).
    };
    
}
