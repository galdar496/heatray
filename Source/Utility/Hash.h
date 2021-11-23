//
//  Hash.h
//  Heatray
//
//  Convenience functions for dealing with hashing.
//
//

#pragma once

#include <cstring>   // for size_t
#include <functional>
#include <stdint.h>  // for uint64_t

namespace util {

constexpr inline uint64_t FNV1a(char const * bytes, size_t size)
{
    constexpr uint64_t kFNVOffsetBasis = 0xcbf29ce484222325;
    constexpr uint64_t kFNVPrime = 0x100000001b3;

    uint64_t hash = kFNVOffsetBasis;

    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= kFNVPrime;
    }

    return hash;
}

template <typename T>
constexpr uint64_t FNV1a(T const & t)
{
    return FNV1a((char const *)&t, sizeof(T));
}

//-------------------------------------------------------------------------
// Generate a new hash for 'newObject' and combine it with an existing hash.
template <class T>
size_t hashCombine(size_t existingHash, const T& newObject)
{
    std::hash<T> hasher;
    size_t hash = existingHash;
    hash ^= hasher(newObject) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}

} // namespace util.
