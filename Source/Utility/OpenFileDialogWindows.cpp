#include "Utility/OpenFileDialog.h"

#include <Windows.h>

namespace util
{

std::vector<std::string> OpenFileDialog()
{
    char path[256];

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = path;
    path[0] = '\0';
    ofn.nMaxFile = sizeof(path) / sizeof(path[0]);
    ofn.lpstrFilter = "*.*\0\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
    {
        std::vector<std::string> retVal;
        retVal.push_back(path);
        return retVal;
    }

    return std::vector<std::string>();
}

} // namespace util
