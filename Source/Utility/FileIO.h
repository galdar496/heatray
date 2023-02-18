//
//  FileIO.h
//  Heatray
//
//  Simple wrappers around FileIO concepts necessary for loading/saving text files.
//
//

#pragma once

#include <string>
#include <string_view>

namespace util {
 
//-------------------------------------------------------------------------
// Read a text file and return its contents in a string. Returns true if
// the file was successfully read.
bool readTextFile(const std::string_view filename, std::string& content);

}  // namespace util.
