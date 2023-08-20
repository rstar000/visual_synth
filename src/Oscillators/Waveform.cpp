#include "Oscillators/Waveform.hpp"

#include <cmath>
#include <cstdlib>
#include <cstdint>


float GenWaveSine(float phase)
{
    return std::sin(phase);
}

float GenWaveSquare(float phase)
{
    return phase > M_PIf ? 1.0f : -1.0f;

    // Fourier approx for smoother peaks
    float f = M_2_PI;
    constexpr uint32_t NUM_WAVES = 20;

    float sum = 0.0f;
    for (uint32_t k = 1; k <= NUM_WAVES; ++k) {
        float a = (2.0f * k - 1.0f);
        float frac = std::sin(2.0f * M_PIf * a * f * phase) / a;
        sum += frac;
    }
    float res = 4.0f * M_1_PIf * sum;
    return res;
}

float GenWaveTri(float phase)
{
    return 0.0f;
}

float GenWaveSaw(float phase)
{
    // Fourier approx for smoother peaks
    float f = 1.0f / (2.0f * M_PIf);
    constexpr uint32_t NUM_WAVES = 50;

    float sum = 0.0f;
    for (uint32_t k = 1; k <= NUM_WAVES; ++k) {
        float one = (k % 2) ? 1.0f : -1.0f;
        float frac = std::sin(2.0f * M_PIf * k * f * phase) / k;
        sum += frac * one;
    }
    float res = 0.5f - M_1_PIf * sum;
    return (res - 0.5f) * 2.0f;
}

float GenWaveNoise(float phase)
{
    int r = rand();
    return 2.0f * (static_cast<float>(r) / RAND_MAX) - 1.0f;
}