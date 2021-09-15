#pragma once

#include <string>
#include <vector>

namespace util {

std::vector<std::string> OpenFileDialog();
std::vector<std::string> SaveFileDialog(const std::string &extension);

} // namespace util
