//
//  StringUtils.h
//  Heatray
//
//  Utilities to help make working with strings easier.
//
//

#pragma once

#include <assert.h>
#include <string>

namespace util {

//-------------------------------------------------------------------------
// Create a string using printf-style syntax.
template <class ... Args>
inline std::string createStringWithFormat(const char* format, Args &&... args) {
    std::string out;
    char str[4096] = { 0 };
    int length = snprintf(str, 2047, format, args...);
    assert(length < 4096);
    out = str;

    return out;
}

}  // namespace util.

