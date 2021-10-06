//
//  StringUtils.h
//  Heatray
//
//  Utilities to help make working with strings easier.
//
//

#pragma once

#include <assert.h>
#include <stdarg.h>
#include <string>

namespace util {

inline std::string createStringWithFormat(const char* format, va_list args) {
    va_list argsCopy;
    va_copy(argsCopy, args);

    std::string out;
    char str[2048] = { 0 };
    int length = vsnprintf(str, 2047, format, argsCopy);
    assert(length < 2048);
    out = str;
    va_end(argsCopy);

    return out;
}

inline std::string createStringWithFormat(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    std::string out = createStringWithFormat(format, args);
    va_end(args);
    
    return out;
}

}  // namespace util.

