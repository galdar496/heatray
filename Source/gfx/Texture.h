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
 
///
/// Container for a single OpenRL texture object.
///

class Texture
{
    public:
    
        Texture();
        ~Texture();
        
        struct Params;
    
        ///
        /// Sets the parameters used when creating the texture during Load().
        /// See Texture::Params for a list of members and default values. To have
        /// any affect, this function must be called before loading the texture.
        ///
        /// @param p Parameters to set as defined by struct Params.
        ///
        void SetParams(const Params &p);
    
        ///
        /// Create a texture with a texture path. Returns success.
        ///
        /// @param path Absolute filepath to the texture on disk.
        /// @param clearData If true, the CPU data is cleared once the data is sent to OpenRL.
        ///
        /// @return If true, the texture was successfully created.
        ///
        bool Create(const std::string &path, const bool clearData = true);
    
        ///
        /// Create a texture with passed-in data.
        ///
        /// @param width Width of the texture.
        /// @param height Height of the texture.
        /// @param dataType Data type to make the texture.
        /// @param data Data to pass to the texture once created.
        /// @param name Name of the texture.
        ///
        /// @return If true, the texture was successfully created.
        ///
        bool Create(const RLint width, const RLint height, const RLenum dataType, const void *data, const std::string &name);
    
        ///
        /// Load texture data but do not yet create the texture in OpenRL.
        ///
        /// @path Path to the texture.
        /// @param gammaCorrect If true, perform gamma correction on the image before creating it.
        ///
        /// @return If true, the texture data was found and loaded.
        ///
        bool LoadTextureData(const std::string &path, bool gammaCorrect);
    
        ///
        /// Create a texture from already loaded data.
        ///
        /// @param clearLoadedData If true, clear the data loaded for the texture after uploading it to OpenRL.
        ///
        /// @return If true, the OpenRL texture was successfully created.
        ///
        bool CreateFromLoadedData(bool clearLoadedData);
    
        ///
        /// Generate a texture full of random values.
        ///
        /// @param width Width of the texture.
        /// @param height Height of the texture.
        /// @param components Number of color components the texture has.
        /// @param dataType Data type of the texture.
        /// @param min Minimum value for each component of the texture data.
        /// @param max Maximum value for each component of the texture data.
        /// @param name Name of the texture.
        ///
        /// @return If true, the texture was successfully created.
        ///
        bool Randomize(const RLint width,
                       const RLint height,
                       const int components,
                       const RLenum dataType,
                       const float min,
                       const float max,
                       const std::string &name);
    
        ///
        /// Generate a texture full of random values in a circular region with a local 0 as the center.
        ///
        /// @param width Width of the texture.
        /// @param height Height of the texture.
        /// @param dataType Data type of the texture.
        /// @param radius Radius of the circle.
        /// @param name Name of the texture.
        ///
        /// @return If true, the texture was succesfully created.
        ///
        bool RandomizeRadial(const RLint width, const RLint height, const RLenum dataType, const float radius, const std::string &name);
    
        ///
        /// Destroy this texture.
        ///
        void Destroy();
    
        ///
    	/// Resize the texture to new dimensions. This will destroy the data currently residing in the texture.
    	/// The texture must have already been created.
        ///
        /// @param width New width of the texture.
        /// @param height New height of the texture.
        ///
        void Resize(const RLint width, const RLint height);
    
        ///
        /// Return the width of the texture in pixels.
        ///
        const RLint Width() const;
    
        ///
        /// Return the height of the texture in pixels.
        ///
        const RLint Height() const;
    
        ///
    	/// Get the internal OpenRL texture object.
        ///
        RLtexture GetTexture() const;
    
        ///
        /// Determine if the underlying texture object is valid (created) or not.
        ///
        bool IsValid() const;

        ///
        /// Check to see if the texture has valid data waiting to be downloaded to OpenRL.
        ///
        bool HasData() const;
    
        /// Contains parameters for texture operation.
        /// Pass this into SetParams().
        struct Params
        {
            /// Number of color components.
            RLint internalFormat;
            
            /// Format of a color component
            RLenum format;
            
            /// Determines how texture coordinates outside of [0..1] are handled.
            RLenum wrapS;
            RLenum wrapT;
            
            /// Minification filter used when a pixel contains multiple texels (e.g. the textured object is far).
            RLenum minFilter;
            
            /// Magnification filter used when a pixel is contained within a texel (e.g. the textured object is close).
            RLenum magFilter;
            
            Params() :
            	internalFormat(RL_RGBA),
            	format(RL_RGBA),
				wrapS(RL_REPEAT),
            	wrapT(RL_REPEAT),
            	minFilter(RL_LINEAR_MIPMAP_LINEAR),
            	magFilter(RL_LINEAR)
            {
            }
        };
        
    private:
    
        ///
        /// Set texture parameters through OpenRL API calls.
        ///
        void ApplyParams();
        
        // Member variables.
    	RLtexture m_textureObject; ///< OpenRL texture object itself.
    
        RLint m_width;	    ///< Width of the texture.
        RLint m_height;     ///< Height of the texture.
    	RLenum m_dataType;  ///< Data type of this texture.
    
    	std::string m_name;	///< Name of this texture.
        
        Params m_params;	///< Texture parameters.
        
        FIBITMAP *m_data;	///< Raw data for the texture. 
};
    
} // namespace gfx
