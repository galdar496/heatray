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
#include <glm/glm/gtc/constants.hpp>

#include <algorithm>
#include <assert.h>
#include <functional>
#include <random>
#include <vector>

namespace util {

template<class T>
void uniformRandomFloats(T* results, const size_t count, unsigned int seed, float min, float max)
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
    for (unsigned int iIndex = 0; iIndex < count; ++iIndex) {
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

    for (unsigned int iIndex = 0; iIndex < count; ++iIndex) {
        results[iIndex][0] = generateValue(iIndex, base.x);
        results[iIndex][1] = generateValue(iIndex, base.y);
    }
}

// Generates random values on a disk such that the center is (0,0).
inline void radialPseudoRandom(glm::vec3* results, const unsigned int count, const unsigned int seed)
{
    assert(results);

    // We assume a radius of 1 and that the calling function will scale the values appropriately.
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    generator.seed(seed);

    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

    for (unsigned int iIndex = 0; iIndex < count; ++iIndex) {
        // Use rejection sampling.
        float x, y;
        do {
            x = distribution(generator);
            y = distribution(generator);
        } while ((x * x + y * y) > 1.0f);

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