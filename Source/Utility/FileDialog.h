//
//  FileDialog.h
//  Heatray
//
//  Platform-specific open/save dialogs.
//
//

#pragma once

#include <string>
#include <vector>

namespace util {

//-------------------------------------------------------------------------
// Create an open file dialog and return the user-selected paths.
std::vector<std::string> OpenFileDialog(const std::string& extension);

//-------------------------------------------------------------------------
// Create a save file dialog and return the user-selected paths.
std::vector<std::string> SaveFileDialog(const std::string& extension);

} // namespace util
