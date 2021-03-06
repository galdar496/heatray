#include "FileIO.h"

#include <fstream>
#include <iostream>

namespace util
{

bool readTextFile(const char* filename, std::string& content)
{
    std::ifstream fin;
    fin.open(filename);
    if (!fin)
    {
        std::cout << "Unable to open file " << filename << std::endl;
        return false;
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