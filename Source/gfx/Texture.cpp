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
#include <iostream>
#include <vector>

namespace gfx
{

/// Default constructor.
Texture::Texture() :
    m_texture_object(RL_NULL_TEXTURE),
    m_width(0),
    m_height(0),
    m_name("<unnamed>"),
    m_data(NULL)
{
    
}

/// Default destructor.
Texture::~Texture()
{
    if (m_texture_object != RL_NULL_TEXTURE)
    {
        rlDeleteTextures(1, &m_texture_object);
        m_texture_object = RL_NULL_TEXTURE;
    }
    
    if (m_data != NULL)
    {
        FreeImage_Unload(m_data);
        m_data = NULL;
    }
}
    
/// Sets the parameters used when creating the texture during load().
/// See Texture::Params for a list of members and default values.
void Texture::setParams(const Params &p)
{
    m_params = p;
}
    
/// Create a texture with a texture path. Returns success.
bool Texture::create(const std::string &path, const bool clear_data)
{
    if (!loadTextureData(path))
    {
        return false;
    }
    
    createFromLoadedData(clear_data);
    
	return true;
}
    
/// Create a texture with passed-in data. Returns success.
bool Texture::create(const RLint width, const RLint height, const RLenum data_type, const void *data, const std::string &name)
{
    m_width  = width;
	m_height = height;
    
	rlGenTextures(1, &m_texture_object);
	rlBindTexture(RL_TEXTURE_2D, m_texture_object);
    
	applyParams();
    
    rlTexImage2D(RL_TEXTURE_2D,
                 0,
                 m_params.internal_format,
                 m_width,
                 m_height,
                 0,
                 m_params.format,
                 data_type, 
                 data);
//    rlGenerateMipmap(RL_TEXTURE_2D);
    rlBindTexture(RL_TEXTURE_2D, 0);
    m_name = name;
    m_data_type = data_type;
    
    return !util::checkRLErrors("Texture::create2 - rlTexImage2D", true);
    
}
    
/// Load texture data but do not yet create the in OpenRL.
bool Texture::loadTextureData(const std::string &path)
{
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(path.c_str (), 0);
	FIBITMAP* unconverted_data = FreeImage_Load(format, path.c_str ());
	if (unconverted_data == NULL)
	{
        std::cout << "Texture::create() - Unable to load " << path << std::endl;
		return false;
	}
    
	m_data = FreeImage_ConvertTo32Bits(unconverted_data);
	FreeImage_Unload(unconverted_data);
	m_width = FreeImage_GetWidth(m_data);
	m_height = FreeImage_GetHeight(m_data);
    m_name = path;
    
    return true;
}

/// Create a texture from already loaded data.
bool Texture::createFromLoadedData(bool clear_loaded_data)
{
    if (!m_data)
    {
        return false;
    }
    
    rlGenTextures(1, &m_texture_object);
	rlBindTexture(RL_TEXTURE_2D, m_texture_object);
    
	applyParams();
    
    rlTexImage2D(RL_TEXTURE_2D, 0, m_params.internal_format, m_width, m_height, 0, RL_RGBA, RL_UNSIGNED_BYTE, FreeImage_GetBits(m_data));
    rlGenerateMipmap(RL_TEXTURE_2D);
    util::checkRLErrors("Texture::create1 - rlTexImage2D", true);
    
    rlBindTexture(RL_TEXTURE_2D, 0);
    m_data_type = RL_UNSIGNED_BYTE;
    
    if (clear_loaded_data)
	{
		FreeImage_Unload(m_data);
		m_data = NULL;
	}
    
    return true;
}
    
/// Generate a texture full of random values. Returns success.
bool Texture::randomize(const RLint width, const RLint height, const int components, const RLenum data_type, const float min, const float max, const std::string &name)
{
	m_params.min_filter = RL_LINEAR;
    
	if (components == 3)
	{
		// Set the format for a GL_RGB image.
		m_params.format = RL_RGB;
		m_params.internal_format = RL_RGB;
	}
	else if (components == 4)
	{
		// Set the format for a GL_RGBA image.
		m_params.format = RL_RGBA;
		m_params.internal_format = RL_RGBA;
	}
	else if (components == 1)
	{
		// Set to the red channel.
		m_params.format = RL_LUMINANCE;
		m_params.internal_format = RL_LUMINANCE;
	}
    
	// Generate the random values.
	size_t size = width * height * components;
    std::vector<float> data;
    data.resize(size);
	for (size_t i = 0; i < size; ++i)
	{
		data[i] = util::random(min, max);
	}
    
	rlGenTextures(1, &m_texture_object);
	rlBindTexture(RL_TEXTURE_2D, m_texture_object);
    
	applyParams();
	
	m_width  = width;
	m_height = height;
	
	rlTexImage2D(RL_TEXTURE_2D, 0, m_params.internal_format, m_width, m_height, 0, m_params.format, data_type, &data[0]);
	rlBindTexture(RL_TEXTURE_2D, 0);
    m_name = name;
    m_data_type = data_type;
    
	return !util::checkRLErrors("Texture::randomize()");
}
   
/// Generate a texture full of random values in a circular region. Returns success.
bool Texture::randomizeRadial(const RLint width, const RLint height, const RLenum data_type, const float radius, const std::string &name)
{
    m_params.min_filter = RL_LINEAR;
    
    // Set the format for a GL_RGB image.
    m_params.format = RL_RGB;
    m_params.internal_format = RL_RGB;
    
    // Generate the random values via rejection sampling.
    float radius_squared = radius * radius;
    size_t size = width * height * 3; // Only supports 3 components.
    std::vector<float> data;
    data.resize(size);
    for (size_t ii = 0; ii < size; ii += 3)
    {
        float x, y;
        do
        {
            x = radius * (2.0f * util::random(0.0f, 1.0f) - 1.0f);
            y = radius * (2.0f * util::random(0.0f, 1.0f) - 1.0f);
        } while ((x * x + y * y) > radius_squared);
        
        data[ii + 0] = x;
        data[ii + 1] = y;
        data[ii + 2] = util::random(0.0f, 1.0f);
    }
    
    rlGenTextures(1, &m_texture_object);
    rlBindTexture(RL_TEXTURE_2D, m_texture_object);
    
    applyParams();
    
    m_width  = width;
    m_height = height;
    
    rlTexImage2D(RL_TEXTURE_2D, 0, m_params.internal_format, m_width, m_height, 0, m_params.format, data_type, &data[0]);
    rlBindTexture(RL_TEXTURE_2D, 0);
    m_name = name;
    m_data_type = data_type;
    
    return !util::checkRLErrors("Texture::randomize()");
}
    
/// Destroy this texture.
void Texture::destroy()
{
    if (m_texture_object != RL_NULL_TEXTURE)
    {
        rlDeleteTextures(1, &m_texture_object);
        m_texture_object = RL_NULL_TEXTURE;
    }
    
    if (m_data != NULL)
    {
        FreeImage_Unload(m_data);
        m_data = NULL;
    }
}
    
/// Resize the texture to new dimensions. This will destroy the data currently residing in the texture.
/// The texture must have already been created.
void Texture::resize(const RLint width, const RLint height)
{
	if (m_texture_object == RL_NULL_TEXTURE)
    {
        return;
    }
    
    rlBindTexture(RL_TEXTURE_2D, m_texture_object);
    rlTexImage2D(RL_TEXTURE_2D, 0, m_params.internal_format, width, height, 0, m_params.format, m_data_type, NULL);
    rlBindTexture(RL_TEXTURE_2D, 0);
    
    m_width  = width;
    m_height = height;
}
    
/// Return the width of the texture in pixels.
const RLint Texture::width() const
{
	return m_width;
}

/// Return the height of the texture in pixels.
const RLint Texture::height() const
{
    return m_height;
}
    
/// Get the internal OpenRL texture object.
RLtexture Texture::getTexture() const
{
    return m_texture_object;
}
    
/// Determine if the underlying texture object is valid (created) or not.
bool Texture::isValid() const
{
    return (m_texture_object != RL_NULL_TEXTURE);
}
    
/// Set texture parameters through OpenRL API calls.
void Texture::applyParams()
{
	rlTexParameteri(RL_TEXTURE_2D, RL_TEXTURE_MIN_FILTER, m_params.min_filter);
	rlTexParameteri(RL_TEXTURE_2D, RL_TEXTURE_MAG_FILTER, m_params.mag_filter);
    
	rlTexParameteri(RL_TEXTURE_2D, RL_TEXTURE_WRAP_S, m_params.wrap_s);
	rlTexParameteri(RL_TEXTURE_2D, RL_TEXTURE_WRAP_T, m_params.wrap_t);
}
    
} // namespace gfx


