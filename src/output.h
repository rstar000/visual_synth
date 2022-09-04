#pragma once

#include <cstdio>
#include <cassert>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>

#include <pulse/pulseaudio.h>

#include "ring_buffer.h"

#include "rtaudio/RtAudio.h"

// Audio constants are all hardcoded here for now.
const int kSampleRate = 44100;
const pa_sample_format kSampleFormat = PA_SAMPLE_S16LE;
const int kNumChannels = 2;

using SampleType = std::int16_t;  // 16 bit.
using SampleBuffer = RingBuffer<SampleType>;

struct AudioOutput {
  float wave;
};

inline SampleType WaveToSample(float waveform) {
  waveform = std::clamp(waveform, -1.0f, 1.0f);
  if (waveform > 0) {
    return waveform * std::numeric_limits<SampleType>::max();
  } else {
    return -waveform * std::numeric_limits<SampleType>::min();
  }
}

class RtAudioOutputHandler {
 public:
  RtAudioOutputHandler(int buf_size);
  ~RtAudioOutputHandler();
  void Start();
  void Stop();
  bool IsPlaying() const;
  auto GetBuffer() { return buf; }

 private:
  RtAudio dac;
  std::shared_ptr<SampleBuffer> buf;
};

// Helper for writing to intermediate buffer and ring buffer
class SampleWriter {
 public:
  SampleWriter(std::shared_ptr<SampleBuffer> buffer) 
    : buffer_(std::move(buffer))
    , samples_(buffer_->Size()) {
  }
  
  // ! Best way to get most accurate time  !
  float GetTimestamp() const {
    size_t position = buffer_->Position() % (360 * kSampleRate);
    return (position + sample_idx_) / static_cast<float>(kSampleRate);
  }
  
  std::size_t ReadyToWrite() {
    return buffer_->ReadyToWrite();
  }
  
  void Write(float wave) {
    ASSERT(sample_idx_ < samples_.size());
    auto sample = WaveToSample(wave);
    samples_[sample_idx_] = sample;
    ++sample_idx_;
  }

  void Flush() {
    buffer_->Write(samples_, sample_idx_);
    sample_idx_ = 0;
  }

 public:
  std::shared_ptr<SampleBuffer> buffer_;
  std::vector<SampleType> samples_;
  size_t sample_idx_ = 0;
};