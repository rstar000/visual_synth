#pragma once

#include "util.h"
#include <cmath>
#include <cstdint>
#include <functional>

// Function signal = wave(phase)
// phase is [0 -> 2pi]
using WaveFunc = std::function<float(float)>;

struct WavetableOscillator
{
    WavetableOscillator(uint32_t numSamples, WaveFunc func)
        : m_numSamples(numSamples)
    {
        m_phaseLookup.resize(numSamples);
        for (int i = 0; i < numSamples; ++i) {
            float phase = 2.0f * M_PIf * (static_cast<float>(i) / m_numSamples);
            m_phaseLookup[i] = func(phase);
        }
    }

    float GetWave(float phase)
    {
        constexpr static float k_2pi = 2.0f * M_PIf;
        float phaseInt;
        float phaseFrac = std::modf(m_numSamples * phase / k_2pi, &phaseInt);
        uint32_t phaseIdx1 = static_cast<uint32_t>(phaseInt);
        float result = (1.0f - phaseFrac) * GetSample(phaseIdx1) + phaseFrac * GetSample(phaseIdx1 + 1);
        return result;
    }

    float GetSample(uint32_t idx)
    {
        return m_phaseLookup[idx % m_numSamples];
    }

private:
    std::vector<float> m_phaseLookup;
    uint32_t m_numSamples;
};