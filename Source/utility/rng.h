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
    
namespace util
{
    /// Generate a group of random floats using a uniform distribution.
    inline void generateRandomNumbers(const float min_value,            // IN: Minimum value to generate from.
                                      const float max_value,            // IN: Maximum value to generate from.
                                      const size_t count,               // IN: Number of random values to generate.
                                      std::vector<float> &randomNumbers // OUT: Generated random values. Any previous values in this vector will be cleared.
                                     )
    {
        static std::random_device random_device;
        static std::mt19937 generator(random_device());
        generator.seed(static_cast<unsigned int>(time(nullptr)));

        std::uniform_real_distribution<float> distribution(min_value, max_value);

        randomNumbers.resize(count);
        for (size_t ii = 0; ii < count; ++ii)
        {
            randomNumbers[ii] = distribution(generator);
        }
    }


    /// Generate a random integer within a specified range.
    inline int random(const int nMin, // IN: Minimum value to generate from.
                      const int nMax  // IN: Maximum value to generate to.
                     )
    {
        static std::default_random_engine randomEngine;
        using Distribution = std::uniform_int_distribution<int>;
        static Distribution rand;
        return rand(randomEngine, Distribution::param_type(nMin, nMax));
    }
    
    /// Generate a random floating-point value within a specified range.
    inline float random(const float fMin, // IN: Minimum value to generate from.
                        const float fMax  // IN: Maximum value to generate to.
                       )
    {
        int max_int = std::numeric_limits<int>::max();

        int r = random(0, max_int);
        float fUnit = static_cast<float>((float)r / max_int);
        float fDiff = fMax - fMin;
        
        return fMin + fUnit * fDiff;
    }
    
    /// Generate a random 1 or -1 to change the sign of a value.
    inline int randomSign()
    {
        if (random(0, 100) < 50)
        {
            return 1;
        }
        
        return -1;
    }
    
    /// Generate a random vector.
    /// Each component of the passed-in vectors specify the range for that component.
    template <class T, unsigned int N>
    inline math::Vector<T,N> random(const math::Vector<T,N> &a, // IN: Low-range input vector to generate from.
                                    const math::Vector<T,N> &b  // IN: Hight-range input vector to generate to.
                                   )
    {
        math::Vector<T,N> result;
        
        for (unsigned int i = 0; i < N; ++i)
        {
            result[i] = random(a[i], b[i]);
        }
        
        return result;
    }
    
} // namespace util


