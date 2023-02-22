#include "FileIO.h"

#include "Log.h"

#include <fstream>

namespace util {

bool readTextFile(const std::string_view filename, std::string& content)
{
    std::ifstream fin;
    fin.open(std::string(filename));
    if (!fin) {
        // Attempt to look one directory back, just in case.
        fin.open("../" + std::string(filename));
        if (!fin) {
            LOG_ERROR("Unable to open file %s", filename);
            return false;
        }
    }

    // Size the string to get ready for the file.
    fin.seekg(0, std::ios::end);    // Move the stream to the end of the file.
    content.reserve(fin.gcount());  // Get the number of characters in the file.
    fin.seekg(0, std::ios::beg);    // Move the stream back to the beginning of the file.

    content.assign(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());
    fin.close();
    return true;
}

} // namespace util.
