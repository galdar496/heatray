//
//  rng.h
//  Heatray
//
//  Created by Cody White on 5/12/14.
//
//

#pragma once

#include "../math/Vector.h"

#include <cstdlib>
#include <random>
#include <vector>
#include <ctime>

///
/// Random number generation utility functions. All random numbers are generated with a
/// uniform distibution. To ensure the best possible distribution you should always use
/// GenerateRandomNumbers().
///

namespace util
{

///
/// Generate a group of random floats using a uniform distribution.
///
/// @param minValue Minimum value to generate from.
/// @param maxValue Maximum value to generate from.
/// @param count Number of random values to generate.
/// @param randomNumbers Generated random values. Any previous values in this vector will be cleared.
///
inline void GenerateRandomNumbers(const float minValue, const float maxValue, const size_t count, std::vector<float> &randomNumbers)
{
    static std::random_device randomDevice;
    static std::mt19937 generator(randomDevice());
    
    // Always seed the generator with the current system time.
    generator.seed(static_cast<unsigned int>(time(nullptr)));

    std::uniform_real_distribution<float> distribution(minValue, maxValue);

    randomNumbers.resize(count);
    for (size_t ii = 0; ii < count; ++ii)
    {
        randomNumbers[ii] = distribution(generator);
    }
}


///
/// Generate a random integer within a specified range.
///
/// @param minValue Minimum value to generate from.
/// @param maxValue Maximum value to generate to.
///
/// @return A random int.
///
inline int Random(const int minValue, const int maxValue)
{
    static std::default_random_engine randomEngine;
    using Distribution = std::uniform_int_distribution<int>;
    static Distribution rand;
    
    return rand(randomEngine, Distribution::param_type(minValue, maxValue));
}

///
/// Generate a random floating-point value within a specified range.
///
/// @param minValue Minimum value to generate from.
/// @param maxValue Maximum value to generate to.
///
/// @return A random float.
///
inline float Random(const float minValue, const float maxValue)
{
    int maxInt = std::numeric_limits<int>::max();

    int r = Random(0, maxInt);
    float fUnit = static_cast<float>((float)r / maxInt);
    float fDiff = maxValue - minValue;
    
    return minValue + fUnit * fDiff;
}

///
/// Generate a random vector.
/// Each component of the passed-in vectors specify the range for that component.
///
/// @param minRange Low-range input vector to generate from.
/// @param maxRange Hight-range input vector to generate to.
///
/// @return A randomized vector value.
template <class T, unsigned int N>
inline math::Vector<T,N> Random(const math::Vector<T,N> &minRange, const math::Vector<T,N> &maxRange)
{
    math::Vector<T,N> result;
    
    for (unsigned int ii = 0; ii < N; ++ii)
    {
        result[ii] = Random(minRange[ii], maxRange[ii]);
    }
    
    return result;
}
    
} // namespace util


