//
//  rng.h
//  Heatray
//
//  Created by Cody White on 5/12/14.
//
//

#pragma once

#include <cstdlib>
#include <random>
#include "../math/Vector.h"
    
namespace util
{
    
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
        int r = random(0, RAND_MAX);
        float fUnit = static_cast<float>((float)r / RAND_MAX);
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


