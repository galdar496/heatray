//
//  Program.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include "Shader.h"
#include <OpenRL/rl.h>

namespace gfx
{

///
/// Encapsulates a RLSL program.
///

class Program
{
    public:
    
        Program();
        ~Program();
    
        ///
        /// Attach the specified shader. The shader must already be valid.
        ///
        /// @param shader Shader to attach to this program. This shader must have already been compiled.
        ///
        /// @return If true, the shader was properly attached.
        ///
        bool Attach(const Shader &shader);
    
        ///
        /// Link the program. Call this after all shaders have been attached to finish program creation.
        ///
        /// @param name Optional parameter to name the program for debug output.
        ///
        bool Link(const std::string &name = "Program");
    
        ///
        /// Add a shader directly to the program.
        ///
        /// @param filename Path to the shader code.
        /// @param type Type of the shader.
        ///
        /// @return If true, the shader was successfully attached.
        ///
        bool AddShader(const std::string &filename, const Shader::ShaderType type);
    
        ///
        /// Destroy this program.
        ///
        void Destroy();
    
        ///
        /// Get the location of a uniform by name.
        ///
        RLint GetUniformLocation(const std::string &name) const;
    
        ///
        /// Set an integer uniform in the program.
        ///
        /// @param location Location of this uniform in the shader.
        /// @param i Value of the uniform.
        ///
        void Set1i(const RLint location, const int i) const;
    
        ///
        /// Set a float uniform in the program.
        ///
        /// @param location Location of this uniform in the shader.
        /// @param f Value of the uniform.
        ///
        void Set1f(const RLint location, const float f) const;
    
        ///
        /// Set a 2 component float uniform in the program.
        ///
        /// @param location Location of this uniform in the shader.
        /// @param f Value of the uniform.
        ///
        void Set2fv(const RLint location, const float *f) const;
    
        ///
        /// Set a 2 component int uniform in the program.
        ///
        /// @param location Location of this uniform in the shader.
        /// @param i Value of the uniform.
        ///
        void Set2iv(const RLint location, const int *i) const;
    
        ///
        /// Set a 3 component float uniform in the program.
        ///
        /// @param location Location of this uniform in the shader.
        /// @param f Value of the uniform.
        ///
        void Set3fv(const RLint location, const float *f) const;
    
        ///
        /// Set a 4 component float uniform in the program.
        ///
        /// @param location Location of this uniform in the shader.
        /// @param f Value of the uniform.
        ///
        void Set4fv(const RLint location, const float *f) const;
    
        ///
        /// Set a 4 component int uniform in the program.
        ///
        /// @param location Location of this uniform in the shader.
        /// @param fiValue of the uniform.
        ///
        void Set4iv(const RLint location, const int *i) const;
    
        ///
        /// Set a 4x4 matrix uniform in the program.
        ///
        /// @param location Location of this uniform in the shader.
        /// @param f Value of the uniform.
        ///
        void SetMatrix4fv(const RLint location, const float *f) const;
    
        ///
        /// Set a texture uniform in the program.
        ///
        /// @param location Location of this uniform in the shader.
        /// @param texture Value of the uniform.
        ///
    	void SetTexture(const RLint location, const RLtexture &texture);
    
        ///
        /// Set a primitive uniform in the program.
        ///
        /// @param location Location of this uniform in the shader.
        /// @param primitive Value of the uniform.
        ///
        void SetPrimitive(const RLint location, const RLprimitive &primitive);
    
        ///
        /// Bind this program for use.
        ///
        void Bind() const;
    
        ///
        /// Unbind this program from use.
        ///
        void Unbind() const;
    
        ///
        /// Get a location of an attribute variable in the program.
        ///
        /// @param name Name of the attribute variable.
        ///
        RLint GetAttributeLocation(const std::string &name) const;
    
        ///
    	/// Get access to the internal program object.
        ///
    	RLprogram GetProgram() const;
        
    private:
    
        ///
        /// Create the RL program.
        ///
        bool Create();
    
    	// Programs are not copyable.
    	Program(const Program &other) = delete;
    	Program & operator=(const Program &other) = delete;
        
        // Member variables.
        RLprogram   m_program;      ///< RL program object.
        std::string m_programName;	///< The name of this program (optional).
    };
    
}
