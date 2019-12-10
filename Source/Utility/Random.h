//
//  Random.h
//  Heatray
//
//  Generate random numbers with various distributions.
//
//

#pragma once

#include <Utility/BlueNoise.h>

#include <glm/glm/glm.hpp>

#include <algorithm>
#include <assert.h>
#include <functional>
#include <random>

namespace util
{

template<class T>
void uniformRandomFloats(T* results, const size_t count, unsigned int seed, float min, float max)
{
    assert(results);
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    generator.seed(seed);

    std::uniform_real_distribution<float> distribution(min, max);

    int elementCount = sizeof(T) / sizeof(float); // Note that this assumes all random numbers are floats.
    for (int iIndex = 0; iIndex < count; ++iIndex)
    {
        for (int iElement = 0; iElement < elementCount; ++iElement)
        {
            // Note that T must have an operator[].
            results[iIndex][iElement] = distribution(generator);
        }
    }
}

// This may or may not be valid....
inline void hammersley(glm::vec3* results, const unsigned int count, int sequenceIndex)
{
    assert(results);
    auto radicalInverse = [](unsigned int bits) {
        // Reverse the bits first.
        unsigned int b = (bits << 16u) | (bits >> 16u);
        b = (b & 0x55555555u) << 1u | (b & 0xAAAAAAAAu) >> 1u;
        b = (b & 0x33333333u) << 2u | (b & 0xCCCCCCCCu) >> 2u;
        b = (b & 0x0F0F0F0Fu) << 4u | (b & 0xF0F0F0F0u) >> 4u;
        b = (b & 0x00FF00FFu) << 8u | (b & 0xFF00FF00u) >> 8u;

        return float(b) * 2.3283064365386963e-10f;
    };

    float divisor = 1.0f / static_cast<float>(count);
    for (unsigned int iIndex = 0; iIndex < count; ++iIndex)
    {
        // Hammersley is a 2D sequence.
        results[iIndex][0] = static_cast<float>(iIndex) * divisor;
        results[iIndex][1] = radicalInverse(iIndex); 
    }

    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    generator.seed(sequenceIndex);
    std::shuffle(results, results + count, generator);
}

inline void blueNoise(glm::vec3* results, const unsigned int count, int sequenceIndex)
{
    LowDiscrepancyBlueNoiseGenerator generator(sequenceIndex);
    generator.GeneratePoints(count);
    for (int i = 0; i < count; ++i)
    {
        results[i] = glm::vec3(generator.GetPoints()[i], 0);
    }
}

inline void halton(glm::vec3* results, const unsigned int count, int sequenceIndex)
{
    glm::ivec2 coprimes[] = {
        glm::ivec2(2, 3),
        glm::ivec2(2, 5),
        glm::ivec2(2, 7),
        glm::ivec2(3, 7),
        glm::ivec2(4, 5),
        glm::ivec2(5, 7),
        glm::ivec2(5, 9),
        glm::ivec2(5, 11),
        glm::ivec2(6, 11),
        glm::ivec2(5, 11),
        glm::ivec2(8, 11),
        glm::ivec2(3, 5),
        glm::ivec2(11, 15),
        glm::ivec2(2, 15),
        glm::ivec2(3, 19),
        glm::ivec2(7, 10)
    };
    assert(sequenceIndex < sizeof(coprimes) / sizeof(glm::ivec2));

    auto generateValue = [](int index, int base)
    {
        float result = 0.0f;
        float f = 1.0f;
        float denom = float(base);
        unsigned int n = index;
        while (n > 0)
        {
            f = f / denom;
            result += f * (n % base);
            n = n / float(base);
        }
        return result;
    };

    glm::ivec2 base = coprimes[sequenceIndex];

    for (int iIndex = 0; iIndex < count; ++iIndex)
    {
        results[iIndex][0] = generateValue(iIndex, base.x);
        results[iIndex][1] = generateValue(iIndex, base.y);
    }
}

} // namespace util.