#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

using SampleType = std::int16_t;  // 16 bit.
const int kSampleRate = 44100;
const int kNumChannels = 2;

inline SampleType WaveToSample(float waveform) {
    waveform = std::clamp(waveform, -1.0f, 1.0f);
    if (waveform > 0) {
        return waveform * std::numeric_limits<SampleType>::max();
    } else {
        return -waveform * std::numeric_limits<SampleType>::min();
    }
}

struct PlaybackContext
{
    float timestamp;
    size_t sampleIdx;
    const uint32_t numSamples;
    const uint32_t numVoices;
    const uint32_t sampleRate;
};

class SampleWriter {
   public:
    SampleWriter() {}

    float GetTimestamp(int frameSampleIdx) const {
        // TODO: remove 360. Hack to get accurate float precision
        return (m_frameBeginIdx + frameSampleIdx) / static_cast<float>(kSampleRate);
    }

    size_t GetSample(int frameSampleIdx) const {
        return m_frameBeginIdx + frameSampleIdx;
    }

    void SetBuffer(SampleType* ptr) { m_ptr = ptr; }

    void Write(float wave, int sample_idx) {
        auto sample = WaveToSample(wave);
        m_ptr[sample_idx] += sample;
    }

    void Flush(int num_samples) { m_frameBeginIdx += num_samples; }

   public:
    SampleType* m_ptr;
    size_t m_frameBeginIdx = 0;
};