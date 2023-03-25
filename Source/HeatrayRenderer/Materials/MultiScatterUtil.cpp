#if 0

#include "MultiScatterUtil.h"

#include <Utility/Log.h>
#include <Utility/Random.h>
#include <Utility/TextureLoader.h>

#include <FreeImage/FreeImage.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/compatibility.hpp>

#include <algorithm>
#include <assert.h>
#include <vector>

namespace {
static char const *LUT_FILENAME = "Resources/multiscatter_lut.tiff";
static std::weak_ptr<openrl::Texture> multiscatterTexture;
}

inline float square(float f) { return f * f;  }

float G1_Smith_GGX(float NdotI, float roughnessAlpha)
{
    const float alpha2 = square(roughnessAlpha);
    const float denom = std::sqrtf(alpha2 + (1.0f - alpha2) * square(NdotI)) + NdotI;
    return (2.0f * NdotI) / std::max(denom, 1e-5f);
}

float G2_Smith_GGX(float NdotL, float NdotV, float roughnessAlpha)
{
    return G1_Smith_GGX(NdotL, roughnessAlpha) * G1_Smith_GGX(NdotV, roughnessAlpha);
}

glm::vec3 importanceSampleGGX(glm::vec2 random, float alpha) // Returns the half-vector.
{
    // Importance sample.
    float a2 = alpha * alpha;
    const float cosTheta = std::sqrtf(std::max(0.0f, (1.0f - random.x) / ((a2 - 1.0f) * random.x + 1.0f)));
    const float sinTheta = std::sqrtf(std::max(0.0f, 1.0f - square(cosTheta)));
    const float phi = glm::two_pi<float>() * random.y;

    // Convert to cartesian coordinates.
    const float x = sinTheta * std::cosf(phi);
    const float y = sinTheta * std::sinf(phi);
    const float z = cosTheta;
    return glm::normalize(glm::vec3(x, y, z));
}

float generateValue(float NdotV, float alpha, size_t sampleCount, const std::vector<glm::vec2>& randomSequence)
{
    // NOTE: this is working in a Z-up basis just to be easier for matching literature. In the end, it 
    // doesn't matter if this calculation is Y or Z up, so long as all calculations here are done with
    // the same assumptions.
    assert(sampleCount == randomSequence.size());

    const glm::vec3 N = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 V(std::sqrtf(1.0f - (NdotV * NdotV)), 0.0f, NdotV);
    
    float result = 0.0f;
    for (size_t iSample = 0; iSample < sampleCount; ++iSample) {
        const glm::vec2 random(randomSequence[iSample].x, randomSequence[iSample].y);
        const glm::vec3 H = importanceSampleGGX(random, alpha);
        const glm::vec3 L = 2.0f * glm::dot(V, H) * H - V; // Reflect about the half-vector.

        float NdotL = glm::saturate<float, glm::defaultp>(glm::dot(N, L));

        if (NdotL > 0.0f) {
            const float VdotH = glm::saturate<float, glm::defaultp>(glm::dot(V, H));
            const float NdotH = glm::saturate<float, glm::defaultp>(glm::dot(N, H));

            // Here we sample our specular microfacet BRDF as if F == 1. After accounting
            // for the GGX sample PDF we're left with:
            // (G * VdotH) / (NdotV * NdotH).
            const float G = (G2_Smith_GGX(NdotL, NdotV, alpha) * VdotH) / (NdotV * NdotH);
            result += G;
        }
    }

    return result / float(sampleCount);
}

void ErrorHandler(FREE_IMAGE_FORMAT fif, const char* message) {
    LOG_ERROR("FreeImage*** ");
    if (fif != FIF_UNKNOWN) {
        LOG_ERROR("%s Format", FreeImage_GetFormatFromFIF(fif));
    }
    LOG_ERROR(message);
    LOG_ERROR("***");
}

void generateMultiScatterTexture()
{
    // Using Monte Carlo, we'll randomly sample the GGX NDF and caluclate how much light
    // gets reflected among the microsurfaces for varying roughness values and view directions.
    // These results will be placed in a LUT that can be efficiently queried at runtime.

    static constexpr size_t IMAGE_DIMENSIONS = 128;
    static constexpr size_t SAMPLE_COUNT = 4096;

    // Generate our random sequence data to use for importance sampling.
    std::vector<glm::vec2> randomSequence;
    randomSequence.resize(SAMPLE_COUNT);
    util::sobol(randomSequence.data(), SAMPLE_COUNT, 0);

    std::vector<float> results;
    results.resize(IMAGE_DIMENSIONS * IMAGE_DIMENSIONS);

    size_t resultIndex = 0;
    // row: alpha, col: NdotV
    for (float row = 0.0f; row < IMAGE_DIMENSIONS; ++row) {
        const float roughness = glm::saturate<float, glm::defaultp>((row + 0.5f) / float(IMAGE_DIMENSIONS));

        // We use the perceptural roughness as defined by Disney (roughness^2).
        const float alpha = roughness * roughness;
        for (float col = 0.0f; col < IMAGE_DIMENSIONS; ++col) {
            const float NdotV = glm::saturate<float, glm::defaultp>((col + 0.5f) / float(IMAGE_DIMENSIONS));
            float value = generateValue(NdotV, alpha, SAMPLE_COUNT, randomSequence);
            value = ((1.0f - value) / value); // Value is stored like this to avoid shader operations later.

            results[resultIndex] = value;
            ++resultIndex;
        }
    }

    assert(resultIndex == results.size());

    // Write the image to the path specified.
    {
        FreeImage_Initialise();
        FreeImage_SetOutputMessage(ErrorHandler);

        FIBITMAP* bitmap = FreeImage_AllocateT(FIT_FLOAT, IMAGE_DIMENSIONS, IMAGE_DIMENSIONS, 32); // 32 bits per pixel (R32).
        void* pixels = FreeImage_GetBits(bitmap);
        std::memcpy(pixels, results.data(), sizeof(float) * results.size());

        FreeImage_Save(FreeImage_GetFIFFromFilename(LUT_FILENAME), bitmap, LUT_FILENAME, 0);
        FreeImage_DeInitialise();
    }
}

std::shared_ptr<openrl::Texture> loadMultiscatterTexture()
{
    std::shared_ptr<openrl::Texture> texture = multiscatterTexture.lock();
    if (!texture) {
        texture = util::loadTexture(LUT_FILENAME, false, false);
        multiscatterTexture = texture;
    }

    return texture;
}

#endif
