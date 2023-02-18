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
#include <string_view>

namespace util {

//-------------------------------------------------------------------------
// Create a string using printf-style syntax.
template <class ... Args>
inline std::string createStringWithFormat(const std::string_view format, Args &&... args) {
    std::string out;
    char str[4096] = { 0 };
    int length = snprintf(str, 4095, format.data(), args...);
    assert(length < 4096);
    out = str;

    return out;
}

}  // namespace util.

