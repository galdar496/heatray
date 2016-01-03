//
//  Texture.h
//  Heatray
//
//  Created by Cody White on 5/29/14.
//
//

#pragma once

#include <OpenRL/rl.h>
#include <FreeImage/FreeImage.h>
#include <string>

namespace gfx
{
 
/// Container for a single OpenRL texture object.
class Texture
{
    public:
        
        /// Default constructor.
        Texture();
    
        /// Default destructor.
        ~Texture();
        
        struct Params;
    
        /// Sets the parameters used when creating the texture during load().
        /// See Texture::Params for a list of members and default values.
        void setParams(const Params &p // IN: Parameters to set as defined by struct Params.
                      );
    
        /// Create a texture with a texture path. Returns success.
        bool create(const std::string &path,      // IN: Path to the texture.
                    const bool clear_data = true  // IN: If true, the CPU data is cleared once the data is sent to OpenRL.
                   );
    
        /// Create a texture with passed-in data. Returns success.
        bool create(const RLint width,       // IN: Width of the texture.
                    const RLint height,      // IN: Height of the texture.
                    const RLenum data_type,  // IN: Data type to make the texture.
                    const void *data,        // IN: Data to pass to the texture once created.
    			    const std::string &name  // IN: Name of the texture.
                   );
    
        /// Load texture data but do not yet create the in OpenRL.
        bool loadTextureData(const std::string &path // IN: Path to the texture.
                            );
        
        /// Create a texture from already loaded data.
        bool createFromLoadedData(bool clear_loaded_data // IN: If true, clear the data loaded for the texture after uploading it to OpenRL.
                                 );
    
        /// Generate a texture full of random values. Returns success.
        bool randomize(const RLint width,       // IN: Width of the texture.
                       const RLint height,      // IN: Height of the texture.
                       const int components,    // IN: Number of color components the texture has.
                       const RLenum data_type,  // IN: Data type of the texture.
                       const float min,         // IN: Minimum value for each component of the texture data.
                       const float max,         // IN: Maximum value for each component of the texture data.
                       const std::string &name  // IN: Name of the texture.
                      );
    
        /// Generate a texture full of random values in a circular region. Returns success.
        bool randomizeRadial(const RLint width,      // IN: Width of the texture.
                             const RLint height,     // IN: Height of the texture.
                             const RLenum data_type,  // IN: Data type of the texture.
                             const float radius,      // IN: Radius of the circle.
                             const std::string &name  // IN: Name of the texture.
        );
    
        /// Destroy this texture.
        void destroy();
    
    	/// Resize the texture to new dimensions. This will destroy the data currently residing in the texture.
    	/// The texture must have already been created.
        void resize(const RLint width,  // IN: New width of the texture.
    			    const RLint height  // IN: New height of the texture.
                   );
    
        /// Return the width of the texture in pixels.
        const RLint width() const;
    
        /// Return the height of the texture in pixels.
        const RLint height() const;
    
    	/// Get the internal OpenRL texture object.
        RLtexture getTexture() const;
    
        /// Determine if the underlying texture object is valid (created) or not.
        bool isValid() const;
    
        /// Contains parameters for texture operation.
        /// Pass this into setParams().
        struct Params
        {
            /// Number of color components.
            RLint internal_format;
            
            /// Format of a color component
            RLenum format;
            
            /// Determines how texture coordinates outside of [0..1] are handled.
            RLenum wrap_s;
            RLenum wrap_t;
            
            /// Minification filter used when a pixel contains multiple texels (e.g. the textured object is far).
            RLenum min_filter;
            
            /// Magnification filter used when a pixel is contained within a texel (e.g. the textured object is close).
            RLenum mag_filter;
            
            /// Default constructor.
            Params() :
            	internal_format(RL_RGBA),
            	format(RL_RGBA),
				wrap_s(RL_REPEAT),
            	wrap_t(RL_REPEAT),
            	min_filter(RL_LINEAR_MIPMAP_LINEAR),
            	mag_filter(RL_LINEAR)
            {
            }
        };
        
    private:
    
        /// Set texture parameters through OpenRL API calls.
        void applyParams();
        
        // Member variables.
    	RLtexture m_texture_object; // OpenRL texture object itself.
    
        RLint m_width;	    // Width of the texture.
        RLint m_height;    // Height of the texture.
    	RLenum m_data_type; // Data type of this texture.
    
    	std::string m_name;	// Name of this texture.
        
        Params m_params;	 // Texture parameters.
        
        FIBITMAP *m_data;	// Data for the texture. 
};
    
} // namespace gfx
