//
//  Random.h
//  Heatray
//
//  Generate random numbers with various distributions.
//
//

#pragma once

#include "BlueNoise.h"
#include "Hash.h"

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/constants.hpp>

#include <algorithm>
#include <assert.h>
#include <functional>
#include <limits>
#include <random>
#include <vector>

namespace util {

inline uint32_t toUint32(const float normalized_f)
{
    return uint32_t(normalized_f * std::numeric_limits<uint32_t>::max());
}

inline float toNormalizedFloat(const uint32_t u)
{
    return float(u) * (1.0f / std::numeric_limits<uint32_t>::max());
}

inline uint32_t burleyHash(uint32_t x)
{
    // See http://www.jcgt.org/published/0009/04/01/paper.pdf
    x ^= x >> 16;
    x *= 0x85ebca6bu;
    x ^= x >> 13;
    x *= 0xc2b2ae35u;
    x ^= x >> 16;
    return x;
}

inline uint32_t burleyHashCombine(uint32_t seed, uint32_t v)
{
    return seed ^ (v + (seed << 6) + (seed >> 2));
}

inline uint32_t laineKarrasPermutation(uint32_t x, uint32_t seed)
{
    x += seed;
    x ^= x * 0x6c50b47cu;
    x ^= x * 0xb82f1e52u;
    x ^= x * 0xc7afe638u;
    x ^= x * 0x8d22f6e6u;
    return x;
}

inline uint32_t reverseBits(uint32_t bits)
{
    uint32_t b = (bits << 16u) | (bits >> 16u);
    b = (b & 0x55555555u) << 1u | (b & 0xAAAAAAAAu) >> 1u;
    b = (b & 0x33333333u) << 2u | (b & 0xCCCCCCCCu) >> 2u;
    b = (b & 0x0F0F0F0Fu) << 4u | (b & 0xF0F0F0F0u) >> 4u;
    b = (b & 0x00FF00FFu) << 8u | (b & 0xFF00FF00u) >> 8u;
    return b;
}

inline uint32_t nestedUniformScramble(uint32_t x, uint32_t seed)
{
    x = reverseBits(x);
    x = laineKarrasPermutation(x, seed);
    x = reverseBits(x);
    return x;
}

// sampleIndex = the index to use when generating the sample
// arrayIndex = the array index where this sample will end up in our table.
using SequenceGenerator = std::function<glm::vec2(uint32_t sampleIndex, uint32_t arrayIndex)>;

// Adapted from http://www.jcgt.org/published/0009/04/01/paper.pdf
inline void owenScrambleSequence(glm::vec3* results, uint32_t count, uint32_t sequenceIndex, SequenceGenerator generator)
{
    enum Dimension {
        ZERO = 0,
        ONE,

        COUNT
    };

    // Randomize the seed (we use the sequenceIndex as the primary seed, +1 to avoid a 0 hash).
    uint32_t seed = burleyHash(sequenceIndex + 1);

    for (uint32_t iIndex = 0; iIndex < count; ++iIndex) {
        uint32_t index = nestedUniformScramble(iIndex, seed);

        glm::vec2 sample = generator(index, iIndex);

        sample.x = toNormalizedFloat(nestedUniformScramble(toUint32(sample.x), burleyHashCombine(seed, Dimension::ZERO)));
        sample.y = toNormalizedFloat(nestedUniformScramble(toUint32(sample.y), burleyHashCombine(seed, Dimension::ONE)));

        results[iIndex][0] = sample.x;
        results[iIndex][1] = sample.y;
    }
}

template<class T>
inline void uniformRandomFloats(T* results, const size_t count, unsigned int seed, float min, float max)
{
    assert(results);
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    generator.seed(seed);

    std::uniform_real_distribution<float> distribution(min, max);

    int elementCount = sizeof(T) / sizeof(float); // Note that this assumes all random numbers are floats.
    for (int iIndex = 0; iIndex < count; ++iIndex) {
        for (int iElement = 0; iElement < elementCount; ++iElement) {
            // Note that T must have an operator[].
            results[iIndex][iElement] = distribution(generator);
        }
    }
}

inline void hammersley(glm::vec3* results, const unsigned int count, int sequenceIndex)
{
    assert(results);
    auto radicalInverse = [](uint32_t bits) {
        // Reverse the bits first.
        uint32_t b = reverseBits(bits);

        return float(b) * 2.3283064365386963e-10f;
    };

    float divisor = 1.0f / static_cast<float>(count);
    auto generator = [radicalInverse, divisor](uint32_t sampleIndex, uint32_t arrayIndex) {
        glm::vec2 sample;

        sample.x = static_cast<float>(arrayIndex) * divisor;
        sample.y = radicalInverse(sampleIndex);
        return sample;
    };

    owenScrambleSequence(results, count, sequenceIndex, generator);
}

inline void blueNoise(glm::vec3* results, const unsigned int count, int sequenceIndex)
{
    LowDiscrepancyBlueNoiseGenerator generator(sequenceIndex);
    generator.GeneratePoints(count);
    for (unsigned int i = 0; i < count; ++i) {
        results[i] = glm::vec3(generator.GetPoints()[i], 0);
    }
}

inline void halton(glm::vec3* results, const unsigned int count, int sequenceIndex)
{
    assert(results);
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
        while (n > 0) {
            f = f / denom;
            result += f * (n % base);
            n = n / base;
        }
        return result;
    };

    glm::ivec2 base = coprimes[sequenceIndex];

    auto generator = [base, generateValue](uint32_t sampleIndex, uint32_t arrayIndex) {
        glm::vec2 sample;

        sample.x = generateValue(sampleIndex, base.x);
        sample.y = generateValue(sampleIndex, base.y);
        return sample;
    };

    owenScrambleSequence(results, count, sequenceIndex, generator);
}

inline void sobol(glm::vec3* results, const uint32_t count, uint32_t sequenceIndex)
{
    assert(results);

    auto sobolValue = [](uint32_t sampleIndex, uint32_t dimension) {
        constexpr static uint32_t directions[2][32] = {
            0x80000000, 0x40000000, 0x20000000, 0x10000000,
            0x08000000, 0x04000000, 0x02000000, 0x01000000,
            0x00800000, 0x00400000, 0x00200000, 0x00100000,
            0x00080000, 0x00040000, 0x00020000, 0x00010000,
            0x00008000, 0x00004000, 0x00002000, 0x00001000,
            0x00000800, 0x00000400, 0x00000200, 0x00000100,
            0x00000080, 0x00000040, 0x00000020, 0x00000010,
            0x00000008, 0x00000004, 0x00000002, 0x00000001,

            0x80000000, 0xc0000000, 0xa0000000, 0xf0000000,
            0x88000000, 0xcc000000, 0xaa000000, 0xff000000,
            0x80800000, 0xc0c00000, 0xa0a00000, 0xf0f00000,
            0x88880000, 0xcccc0000, 0xaaaa0000, 0xffff0000,
            0x80008000, 0xc000c000, 0xa000a000, 0xf000f000,
            0x88008800, 0xcc00cc00, 0xaa00aa00, 0xff00ff00,
            0x80808080, 0xc0c0c0c0, 0xa0a0a0a0, 0xf0f0f0f0,
            0x88888888, 0xcccccccc, 0xaaaaaaaa, 0xffffffff
        };

        uint32_t result = 0;
        for (uint32_t bit = 0; bit < 32; ++bit) {
            uint32_t mask = (sampleIndex >> bit) & 1;
            result ^= mask * directions[dimension][bit];
        }

        return toNormalizedFloat(result);
    };

    auto generator = [sobolValue](uint32_t sampleIndex, uint32_t arrayIndex) {
        glm::vec2 sample;

        sample.x = sobolValue(sampleIndex, 0);
        sample.y = sobolValue(sampleIndex, 1);
        return sample;
    };

    owenScrambleSequence(results, count, sequenceIndex, generator);
}

// Generates Sobol values on a disk such that the center is (0,0).
inline void radialSobol(glm::vec3* results, const unsigned int count, const unsigned int sequenceIndex)
{
    assert(results);
    sobol(results, count, sequenceIndex);

    // We now redistribute these points within a disk.
    for (unsigned int iIndex = 0; iIndex < count; ++iIndex) {
        float s = results[iIndex].x;
        float t = results[iIndex].y;

        float sqrt_t = std::sqrtf(t);
        float two_pi_s = glm::two_pi<float>() * s;
        float x = sqrt_t * std::cosf(two_pi_s);
        float y = sqrt_t * std::sinf(two_pi_s);

        // Get the values to be in the 0-1 range for storage in a texture.
        x = (x + 1.0f) * 0.5f;
        y = (y + 1.0f) * 0.5f;

        results[iIndex] = glm::vec3(x, y, 0.0f);
    }
}

inline void randomPolygonal(glm::vec3* results, const unsigned int numEdges, const unsigned int count, const unsigned int seed)
{
    std::vector<glm::vec2> vertices;
    vertices.resize(numEdges + 1); // Include the center.

    // Generate the vertices that make up this polygon.
    float stepSize = glm::two_pi<float>() / numEdges;
    for (unsigned int iIndex = 0; iIndex < numEdges; ++iIndex) {
        float theta = stepSize * float(iIndex);
        glm::vec2 vertex(std::cosf(theta), std::sinf(theta));
        vertices[iIndex] = vertex;
    }
    vertices[numEdges] = glm::vec2(0.0f);

    // Generate the triangles that make up this polygon by aggregating the above vertices.
    struct Triangle {
        glm::vec2 vertices[3];
    };

    std::vector<Triangle> triangles;
    triangles.resize(numEdges);

    for (unsigned int iIndex = 0; iIndex < numEdges; ++iIndex) {
        Triangle tri;
        tri.vertices[0] = vertices[numEdges]; // Center.
        tri.vertices[1] = vertices[iIndex];
        tri.vertices[2] = vertices[(iIndex + 1) % numEdges];
        triangles[iIndex] = tri;
    }

    // Randomly select a triangle and a point within that triangle N times.
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    generator.seed(seed);

    std::uniform_real_distribution<float> floatDistribution(0.0f, 1.0f); // For selecting the point within a triangle.
    std::uniform_int_distribution<int> intDistribution(0, numEdges - 1); // For selecting between the triangles.

    for (unsigned int iIndex = 0; iIndex < count; ++iIndex) {
        // First choose the triangle.
        int triangleIndex = intDistribution(generator);

        // Generate a random baycentric coordinate within the triangle using rejection sampling.
        float alpha, beta;
        do {
            alpha = floatDistribution(generator);
            beta = floatDistribution(generator);
        } while (alpha + beta > 1.0f);

        float gamma = 1.0f - (alpha + beta);
        Triangle tri = triangles[triangleIndex];
        glm::vec2 vertex = tri.vertices[0] * alpha +
                           tri.vertices[1] * beta  + 
                           tri.vertices[2] * gamma;

        // Get the values to be in the 0-1 range for storage in a texture.
        float x = (vertex.x + 1.0f) * 0.5f;
        float y = (vertex.y + 1.0f) * 0.5f;

        results[iIndex] = glm::vec3(x, y, 0.0f);
    }
}

} // namespace util.