//
//  Texture.cpp
//  Heatray
//
//  Created by Cody White on 5/29/14.
//
//

#include "Texture.h"
#include "../utility/util.h"
#include "../utility/rng.h"
#include "../math/Constants.h"
#include <iostream>
#include <vector>
#include <assert.h>

namespace gfx
{

Texture::Texture() :
    m_textureObject(RL_NULL_TEXTURE),
    m_width(0),
    m_height(0),
    m_name("<unnamed>"),
    m_data(nullptr)
{
    
}

Texture::~Texture()
{
    if (m_textureObject != RL_NULL_TEXTURE)
    {
        rlDeleteTextures(1, &m_textureObject);
        m_textureObject = RL_NULL_TEXTURE;
    }
    
    if (m_data != nullptr)
    {
        FreeImage_Unload(m_data);
        m_data = nullptr;
    }
}
    
void Texture::SetParams(const Params &p)
{
    m_params = p;
}
    
bool Texture::Create(const std::string &path, const bool clear_data)
{
    if (!LoadTextureData(path, false))
    {
        return false;
    }
    
    CreateFromLoadedData(clear_data);
    
	return true;
}
    
bool Texture::Create(const RLint width, const RLint height, const RLenum dataType, const void *data, const std::string &name)
{
    m_width  = width;
	m_height = height;
    
	rlGenTextures(1, &m_textureObject);
	rlBindTexture(RL_TEXTURE_2D, m_textureObject);
    
	ApplyParams();
    
    rlTexImage2D(RL_TEXTURE_2D,
                 0,
                 m_params.internalFormat,
                 m_width,
                 m_height,
                 0,
                 m_params.format,
                 dataType,
                 data);
//    rlGenerateMipmap(RL_TEXTURE_2D);
    rlBindTexture(RL_TEXTURE_2D, 0);
    m_name = name;
    m_dataType = dataType;
    
    CheckRLErrors();
    return true;
}
    
bool Texture::LoadTextureData(const std::string &path, bool gammaCorrect)
{
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(path.c_str (), 0);
	FIBITMAP* unconvertedData = FreeImage_Load(format, path.c_str ());
	if (unconvertedData == nullptr)
	{
        std::cout << "Texture::Create() - Unable to load " << path << std::endl;
		return false;
	}
    
	m_data = FreeImage_ConvertTo32Bits(unconvertedData);
	FreeImage_Unload(unconvertedData);
	m_width = FreeImage_GetWidth(m_data);
	m_height = FreeImage_GetHeight(m_data);
    m_name = path;

    if (gammaCorrect)
    {
        // Apply inverse gamma correction in order to linearize the color data.
        const double gamma = 1.0 / math::GAMMA;
        FreeImage_AdjustGamma(m_data, gamma);
    }
    
    return true;
}

bool Texture::CreateFromLoadedData(bool clearLoadedData)
{
    if (!m_data)
    {
        return false;
    }
    
    rlGenTextures(1, &m_textureObject);
	rlBindTexture(RL_TEXTURE_2D, m_textureObject);
    
	ApplyParams();
    
    rlTexImage2D(RL_TEXTURE_2D, 0, m_params.internalFormat, m_width, m_height, 0, RL_RGBA, RL_UNSIGNED_BYTE, FreeImage_GetBits(m_data));
    rlGenerateMipmap(RL_TEXTURE_2D);
    CheckRLErrors();
    
    rlBindTexture(RL_TEXTURE_2D, 0);
    m_dataType = RL_UNSIGNED_BYTE;
    
    if (clearLoadedData)
	{
		FreeImage_Unload(m_data);
		m_data = nullptr;
	}
    
    return true;
}
    
bool Texture::Randomize(const RLint width, const RLint height, const int components, const RLenum dataType, const float min, const float max, const std::string &name)
{
	m_params.minFilter = RL_LINEAR;
    
	if (components == 3)
	{
		// Set the format for a GL_RGB image.
		m_params.format = RL_RGB;
		m_params.internalFormat = RL_RGB;
	}
	else if (components == 4)
	{
		// Set the format for a GL_RGBA image.
		m_params.format = RL_RGBA;
		m_params.internalFormat = RL_RGBA;
	}
	else if (components == 1)
	{
		// Set to the red channel.
		m_params.format = RL_LUMINANCE;
		m_params.internalFormat = RL_LUMINANCE;
	}
    else
    {
        assert(0 && "Unsupported component count");
    }
    
	// Generate the random values.
	size_t size = width * height * components;
    std::vector<float> data;
    util::GenerateRandomNumbers(min, max, size, data);

	rlGenTextures(1, &m_textureObject);
	rlBindTexture(RL_TEXTURE_2D, m_textureObject);
    
	ApplyParams();
	
	m_width  = width;
	m_height = height;
	
	rlTexImage2D(RL_TEXTURE_2D, 0, m_params.internalFormat, m_width, m_height, 0, m_params.format, dataType, &data[0]);
	rlBindTexture(RL_TEXTURE_2D, 0);
    m_name = name;
    m_dataType = dataType;
    
    CheckRLErrors();
    return true;
}
   
bool Texture::RandomizeRadial(const RLint width, const RLint height, const RLenum dataType, const float radius, const std::string &name)
{
    m_params.minFilter = RL_LINEAR;
    
    // Set the format for a RL_RGB image.
    m_params.format = RL_RGB;
    m_params.internalFormat = RL_RGB;
    
    // Generate the random values via rejection sampling.
    float radiusSquared = radius * radius;
    size_t size = width * height * 3; // Only supports 3 components.
    std::vector<float> data;
    data.resize(size);

    std::vector<float> randomValues;
    util::GenerateRandomNumbers(0.0f, 1.0f, size, randomValues);
    size_t randomIndex = 0;

    for (size_t ii = 0; ii < size; ii += 3)
    {
        if (randomIndex == randomValues.size())
        {
            // Regenerate the random value, we've ran out. There's a very low probability
            // of this code every actually executing.
            util::GenerateRandomNumbers(0.0f, 1.0f, size, randomValues);
            randomIndex = 0;
        }

        float x, y;
        do
        {
            x = radius * (2.0f * randomValues[randomIndex + 0] - 1.0f);
            y = radius * (2.0f * randomValues[randomIndex + 1] - 1.0f);
            ++randomIndex;
        } while ((x * x + y * y) > radiusSquared);
        
        data[ii + 0] = x;
        data[ii + 1] = y;

        // Unused since this generates values in a 2D plane yet we have to use 3 components because OpenRL doesn't support a 2 component texture.
        data[ii + 2] = 0.0f;
    }
    
    rlGenTextures(1, &m_textureObject);
    rlBindTexture(RL_TEXTURE_2D, m_textureObject);
    
    ApplyParams();
    
    m_width  = width;
    m_height = height;
    
    rlTexImage2D(RL_TEXTURE_2D, 0, m_params.internalFormat, m_width, m_height, 0, m_params.format, dataType, &data[0]);
    rlBindTexture(RL_TEXTURE_2D, 0);
    m_name = name;
    m_dataType = dataType;
    
    CheckRLErrors();
    return true;
}
    
void Texture::Destroy()
{
    if (m_textureObject != RL_NULL_TEXTURE)
    {
        rlDeleteTextures(1, &m_textureObject);
        m_textureObject = RL_NULL_TEXTURE;
    }
    
    if (m_data != nullptr)
    {
        FreeImage_Unload(m_data);
        m_data = nullptr;
    }
}
    
void Texture::Resize(const RLint width, const RLint height)
{
	if (m_textureObject == RL_NULL_TEXTURE)
    {
        return;
    }
    
    rlBindTexture(RL_TEXTURE_2D, m_textureObject);
    rlTexImage2D(RL_TEXTURE_2D, 0, m_params.internalFormat, width, height, 0, m_params.format, m_dataType, NULL);
    rlBindTexture(RL_TEXTURE_2D, 0);
    
    m_width  = width;
    m_height = height;
}
    
const RLint Texture::Width() const
{
	return m_width;
}

const RLint Texture::Height() const
{
    return m_height;
}
    
RLtexture Texture::GetTexture() const
{
    return m_textureObject;
}
    
bool Texture::IsValid() const
{
    return (m_textureObject != RL_NULL_TEXTURE);
}

bool Texture::HasData() const
{
    return (m_data != nullptr);
}
    
void Texture::ApplyParams()
{
	rlTexParameteri(RL_TEXTURE_2D, RL_TEXTURE_MIN_FILTER, m_params.minFilter);
	rlTexParameteri(RL_TEXTURE_2D, RL_TEXTURE_MAG_FILTER, m_params.magFilter);
    
	rlTexParameteri(RL_TEXTURE_2D, RL_TEXTURE_WRAP_S, m_params.wrapS);
	rlTexParameteri(RL_TEXTURE_2D, RL_TEXTURE_WRAP_T, m_params.wrapT);
}
    
} // namespace gfx


