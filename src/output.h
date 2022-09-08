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
// #include "multigraph.h"


// Audio constants are all hardcoded here for now.
const int kSampleRate = 44100;
const pa_sample_format kSampleFormat = PA_SAMPLE_S16LE;
const int kNumChannels = 2;

using SampleType = std::int16_t;  // 16 bit.

class SampleWriter;
class Multigraph;

inline SampleType WaveToSample(float waveform) {
  waveform = std::clamp(waveform, -1.0f, 1.0f);
  if (waveform > 0) {
    return waveform * std::numeric_limits<SampleType>::max();
  } else {
    return -waveform * std::numeric_limits<SampleType>::min();
  }
}


class AudioOutputHandler {
 public:
  struct Params {
    std::shared_ptr<Multigraph> graph;
    std::shared_ptr<SampleWriter> writer;
    
    int buf_size;
  };

  AudioOutputHandler(Params ctx);
  ~AudioOutputHandler();

  void Start();
  void Stop();
  bool IsPlaying() const;

 private:
  Params ctx;
  RtAudio dac;
};

// Helper for writing to intermediate buffer and ring buffer
class SampleWriter {
 public:
  SampleWriter() {}
  
  // ! Best way to get most accurate time  !
  float GetTimestamp(int sample_idx) const {
    // TODO: remove 360. Hack to get accurate float precision
    size_t position = cur_sample % (360 * kSampleRate);
    return (position + sample_idx) / static_cast<float>(kSampleRate);
  }
  
  void SetBuffer(SampleType* ptr) {
    _ptr = ptr;
  }
  
  void Write(float wave, int sample_idx) {
    auto sample = WaveToSample(wave);
    _ptr[sample_idx] += sample;
  }

  void Flush(int num_samples) {
    cur_sample += num_samples;
  }

 public:
  SampleType* _ptr;
  size_t cur_sample = 0;
};