//
//  util.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include <OpenRL/rl.h>
#include <FreeImage/FreeImage.h>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>

///
/// Floating utility functions.
///

#define CheckRLErrors() util::CheckError(__FILE__, __LINE__)

namespace util
{

///
/// Check the current OpenRL state for errors.
/// If an error is detected, the system will stop. It is recommended to use the macro
/// CheckRLErrors() instead of this function directly.
///
/// @param filename Filename where the error check is being performed.
/// @param lineNumber Line number where this error check is being performed.
///
///
inline void CheckError(const char *filename, int lineNumber)
{
    RLenum errorID = rlGetError();
    
	if (errorID != RL_NO_ERROR)
	{
        // An OpenRL error has occured, report it to the user.
        std::stringstream stream;
        stream << filename << " (" << lineNumber << ") - An OpenRL error occured: " << errorID;
       
        // Throw an unhandled exception to stop the system.
        throw std::runtime_error(stream.str().c_str());
	}
}

///
/// Write an pixel buffer to an image file. The file type is determined by the extension
/// on the filename path, e.g. "out.tiff" writes a TIFF file.
///
/// @param filename Filename to save the image to.
/// @param width Width of the output image in pixels.
/// @param height Height of the output image in pixels.
/// @param channels Number of channels in the pixel data (e.g. 3 for RGB).
/// @param pixels Pixel data to write into the image.
/// @param divisor Divisor to scale the pixel data by. By default, no scaling is applied.
///
inline void WriteImage(const std::string &filename, int width, int height, int channels, const float *pixels, float divisor = 1.0f)
{
    FreeImage_Initialise();
    
    FIBITMAP *bitmap = FreeImage_Allocate(width, height, 24); // 24 bits per pixel.
    RGBQUAD color;
    
    // Copy the data into the FreeImage bitmap structure.
    int index = 0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Make sure values are clamped within the 0-255 range.
            color.rgbRed   = static_cast<BYTE>(std::max(std::min((pixels[index + 0] * 255.0f) * divisor, 255.0f), 0.0f));
            color.rgbGreen = static_cast<BYTE>(std::max(std::min((pixels[index + 1] * 255.0f) * divisor, 255.0f), 0.0f));
            color.rgbBlue  = static_cast<BYTE>(std::max(std::min((pixels[index + 2] * 255.0f) * divisor, 255.0f), 0.0f));
            FreeImage_SetPixelColor(bitmap, x, y, &color);
            
            index += channels;
        }
    }
    
    FreeImage_Save(FreeImage_GetFIFFromFilename(filename.c_str()), bitmap, filename.c_str(), 0);
    
    FreeImage_DeInitialise();
}
    
///
/// Read a text file and return its contents in a string.
///
/// @param filename Filename to read the text data from.
/// @param content String which will contain the read file contents.
///
/// @return If true, the file was successfully opened and read.
///
inline bool ReadTextFile(const std::string &filename, std::string &content)
{
    
    std::ifstream fin;
    fin.open(filename.c_str());
    if (!fin)
    {
        std::cout << "Unable to open file " << filename << std::endl;
        return false;
    }
    
    // Size the string to great ready for the file.
    fin.seekg(0, std::ios::end);   // Move the stream to the end of the file.
    content.reserve(fin.gcount()); // Get the number of characters in the file.
    fin.seekg(0, std::ios::beg);   // Move the stream back to the beginning of the file.
    
    content.assign(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());
    
    fin.close();
    
    return true;
}
    
} // namespace util