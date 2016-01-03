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

namespace util
{

/// Check the current OpenRL state for errors.
/// If an error is detected, the system will stop. Returns true if an error occured.
inline bool checkRLErrors(const std::string tag = "",        // IN: Optional character string to output along with the RL error.
                          const bool stop_execution = false  // IN: If true, the program will crash if an error is detected and report it to the user.
                         )
{
    RLenum errorID = rlGetError();
    
	if (errorID != RL_NO_ERROR)
	{      
		if (stop_execution)
		{
			// Throw an unhandled exception to stop the system.
			// WARNING: only set stop_execution to true if you
			// are checking for a serious system error.
			throw std::runtime_error(tag.c_str());
		}
        
		return true;
	}
    
	return false;
}
    
/// Write an pixel buffer to an image file. The file type is determined by the extension
/// on the filename path, e.g. "out.tiff" writes a TIFF file.
inline void writeImage(const std::string &filename, // IN: Filename to save the image to.
                       int width,                   // IN: Width of the output image in pixels.
                       int height,                  // IN: Height of the output image in pixels.
                       int channels,                // IN: Number of channels in the pixel data (e.g. 3 for RGB).
                       const float *pixels,         // IN: Pixel data.
                       float divisor = 1.0f         // IN: Divisor to scale the pixel data by. By default, no scaling is applied.
                      )
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
            color.rgbRed   = static_cast<BYTE>(std::max(std::min((pixels[index + 0] * 255.0f) / divisor, 255.0f), 0.0f));
            color.rgbGreen = static_cast<BYTE>(std::max(std::min((pixels[index + 1] * 255.0f) / divisor, 255.0f), 0.0f));
            color.rgbBlue  = static_cast<BYTE>(std::max(std::min((pixels[index + 2] * 255.0f) / divisor, 255.0f), 0.0f));
            FreeImage_SetPixelColor(bitmap, x, y, &color);
            
            index += channels;
        }
    }
    
    FreeImage_Save(FreeImage_GetFIFFromFilename(filename.c_str()), bitmap, filename.c_str(), 0);
    
    FreeImage_DeInitialise();
}
    
/// Read a text file and return its contents in a string.
inline bool readTextFile(const std::string &filename, //  IN: Filename to read the text data from.
                         std::string       &content   // OUT: String which will contain the read file contents.
                        )
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
    
//    str.assign((std::istreambuf_iterator<char>(t)),
//               std::istreambuf_iterator<char>());
//    
//    std::string tmp;
//    while (fin.good())
//    {
//        tmp = fin.get();
//        if (fin.good())
//        {
//            content += tmp;
//        }
//    }
//    
    fin.close();
    
    return true;
}
    
} // namespace util