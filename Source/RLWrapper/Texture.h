//
//  Texture.h
//  Heatray
//
//  
//
//

#pragma once

#include "Error.h"

#include <OpenRL/rl.h>
#include <assert.h>
#include <memory>

namespace openrl {

///
/// Container for a single OpenRL texture object.
///

class Texture
{
public:
    struct Descriptor
    {
        ///< Number of color components.
        RLint internalFormat = RL_RGBA;

        ///< Format of a color component.
        RLenum format = RL_RGBA;

        ///< Data type stored per-channel.
        RLenum dataType = RL_FLOAT;

        RLint width  = 0;
        RLint height = 0;
        RLint depth  = 0; // For 3D textures.
    };

    struct Sampler
    {
        /// Determines how texture coordinates outside of [0..1] are handled.
        RLenum wrapS = RL_REPEAT;
        RLenum wrapT = RL_REPEAT;
        RLenum wrapR = RL_REPEAT; // For 3D textures.

        /// Minification filter used when a pixel contains multiple texels (e.g. the textured object is far).
        RLenum minFilter = RL_LINEAR_MIPMAP_LINEAR;

        /// Magnification filter used when a pixel is contained within a texel (e.g. the textured object is close).
        RLenum magFilter = RL_LINEAR;
    };

	Texture(const Texture& other) = default;
	Texture& operator=(const Texture& other) = default;
	~Texture() {
		if (m_texture != RL_NULL_TEXTURE) {
			RLFunc(rlDeleteTextures(1, &m_texture));
			m_texture = RL_NULL_TEXTURE;
		}
	}

    ///
    /// Create a texture with passed-in data.
    ///
    /// @param data Data to pass to the texture once created.
    /// @param desc Texture descriptor used to initialize the texture format etc.
    /// @param sampler Sampler to apply to the texture.
    /// @param generateMips If true, mipmaps will be generated after creating the texture.
    ///
    /// @return If non-null, the texture was successfully created.
    ///
    static std::shared_ptr<Texture> create(const void* data, const Descriptor& desc, const Sampler& sampler, bool generateMips = true)
    {
		Texture* texture = new Texture(desc, sampler);

        RLFunc(rlBindTexture(RL_TEXTURE_2D, texture->texture()));

		texture->applySampler();

        RLFunc(rlTexImage2D(RL_TEXTURE_2D,
							0,
							desc.internalFormat,
							desc.width,
							desc.height,
							0,
							desc.format,
							desc.dataType,
							data));
        if (generateMips) {
            RLFunc(rlGenerateMipmap(RL_TEXTURE_2D));
        }
        RLFunc(rlBindTexture(RL_TEXTURE_2D, RL_NULL_TEXTURE));

        return std::shared_ptr<Texture>(texture);
    }

    ///
    /// Create a 3D texture with passed-in data.
    ///
    /// @param data Data to pass to the texture once created.
    /// @param desc Texture descriptor used to initialize the texture format etc.
    /// @param sampler Sampler to apply to the texture.
    /// @param generateMips If true, mipmaps will be generated after creating the texture.
    ///
    /// @return If non-null, the texture was successfully created.
    ///
    static std::shared_ptr<Texture> create3D(const void* data, const Descriptor& desc, const Sampler& sampler, bool generateMips = true)
    {
		Texture* texture = new Texture(desc, sampler);

        RLFunc(rlBindTexture(RL_TEXTURE_3D, texture->texture()));

		texture->applySampler(RL_TEXTURE_3D);

        RLFunc(rlTexImage3D(RL_TEXTURE_3D,
							0,
							desc.internalFormat,
							desc.width,
							desc.height,
							desc.depth,
							0,
							desc.format,
							desc.dataType,
							data));
        if (generateMips) {
            RLFunc(rlGenerateMipmap(RL_TEXTURE_3D));
        }
        RLFunc(rlBindTexture(RL_TEXTURE_3D, RL_NULL_TEXTURE));

		return std::shared_ptr<Texture>(texture);
    }

    ///
    /// Resize the texture using the same descriptor/sampler as when this texture was created.
    ///
    /// @param newWidth New width to make the texture (in pixels).
    /// @param newHeight New height to make the texture (in pixels).
    ///
    inline void resize(const RLint newWidth, const RLint newHeight)
    {
        assert(valid());

        RLFunc(rlBindTexture(RL_TEXTURE_2D, m_texture));
        RLFunc(rlTexImage2D(RL_TEXTURE_2D,
							0,
							m_desc.internalFormat,
							newWidth,
							newHeight,
							0,
							m_desc.format,
							m_desc.dataType,
							nullptr));
        RLFunc(rlBindTexture(RL_TEXTURE_2D, 0));

        m_desc.width = newWidth;
        m_desc.height = newHeight;
    }

    ///
    /// Resize the texture using the same descriptor/sampler as when this texture was created.
    ///
    /// @param newWidth New width to make the texture (in pixels).
    /// @param newHeight New height to make the texture (in pixels).
    /// @param newDepth New depth to make the texture (in pixels). Only for 3D textures.
    ///
    inline void resize(const RLint newWidth, const RLint newHeight, const RLint newDepth)
    {
        assert(valid());

        RLFunc(rlBindTexture(RL_TEXTURE_3D, m_texture));
        RLFunc(rlTexImage3D(RL_TEXTURE_3D,
							0,
							m_desc.internalFormat,
							newWidth,
							newHeight,
							newDepth,
							0,
							m_desc.format,
							m_desc.dataType,
							nullptr));
        RLFunc(rlBindTexture(RL_TEXTURE_3D, 0));

        m_desc.width = newWidth;
        m_desc.height = newHeight;
    }

    ///
    /// Return the width of the texture in pixels.
    ///
    inline const RLint width() const { return m_desc.width; }

    ///
    /// Return the height of the texture in pixels.
    ///
    inline const RLint height() const { return m_desc.height; }

    ///
    /// Get the internal OpenRL texture object.
    ///
    inline RLtexture texture() const { return m_texture; }

    ///
    /// Determine if the underlying texture object is valid (created) or not.
    ///
    inline bool valid() const { return (m_texture != RL_NULL_TEXTURE); }

	static std::shared_ptr<Texture> getDummyTexture()
	{
		static bool dummyIsInitialized = false;
		static std::shared_ptr<Texture> dummyTexture;

		if (!dummyIsInitialized) {
			Texture::Descriptor desc;
			desc.width = desc.height = 1;
			float whitePixel[4] = { 1.f, 1.f, 1.f, 1.f };
			dummyTexture = Texture::create(whitePixel, desc, Texture::Sampler(), false);
			dummyIsInitialized = true;
		}

		return dummyTexture;
	}

private:
	explicit Texture(const Descriptor& desc, const Sampler& sampler)
		: m_desc(desc)
		, m_sampler(sampler)
	{
		RLFunc(rlGenTextures(1, &m_texture));
	}

    ///
    /// Set texture sampling parameters through OpenRL API calls.
    ///
    inline void applySampler(RLenum type = RL_TEXTURE_2D) const
    {
        RLFunc(rlTexParameteri(type, RL_TEXTURE_MIN_FILTER, m_sampler.minFilter));
        RLFunc(rlTexParameteri(type, RL_TEXTURE_MAG_FILTER, m_sampler.magFilter));

        RLFunc(rlTexParameteri(type, RL_TEXTURE_WRAP_S, m_sampler.wrapS));
        RLFunc(rlTexParameteri(type, RL_TEXTURE_WRAP_T, m_sampler.wrapT));
        if (type == RL_TEXTURE_3D) {
            RLFunc(rlTexParameteri(type, RL_TEXTURE_WRAP_R, m_sampler.wrapR));
        }
    }

    RLtexture   m_texture = RL_NULL_TEXTURE;  ///< OpenRL texture object itself.
    Descriptor  m_desc;     ///< Descriptor used to create the texture.
    Sampler     m_sampler;  ///< Sampler to use for this texture.
};

} // namespace openrl

