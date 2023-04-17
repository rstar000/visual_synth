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

class SampleWriter {
   public:
    SampleWriter() {}

    // ! Best way to get most accurate time  !
    float GetTimestamp(int sample_idx) const {
        // TODO: remove 360. Hack to get accurate float precision
        size_t position = cur_sample % (360 * kSampleRate);
        return (position + sample_idx) / static_cast<float>(kSampleRate);
    }

    void SetBuffer(SampleType* ptr) { _ptr = ptr; }

    void Write(float wave, int sample_idx) {
        auto sample = WaveToSample(wave);
        _ptr[sample_idx] += sample;
    }

    void Flush(int num_samples) { cur_sample += num_samples; }

   public:
    SampleType* _ptr;
    size_t cur_sample = 0;
};