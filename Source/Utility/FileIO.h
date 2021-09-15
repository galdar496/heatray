//
//  FileIO.h
//  Heatray
//
//  Simple wrappers around FileIO concepts necessary for loading/saving text files.
//
//

#pragma once

#include <string>

namespace util {
 
///
/// Read a text file and return its contents in a string.
///
/// @param filename Filename to read the text data from.
/// @param content String which will contain the read file contents.
///
/// @return If true, the file was successfully opened and read.
///
bool readTextFile(const char* filename, std::string& content);

}  // namespace util.
