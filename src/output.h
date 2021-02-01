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
#include "node.h"

// Audio constants are all hardcoded here for now.
const int kSampleRate = 44100;
const pa_sample_format kSampleFormat = PA_SAMPLE_S16LE;
const int kNumChannels = 2;

using SampleType = std::int16_t;  // 16 bit.
using SampleBuffer = RingBuffer<SampleType>;

void context_state_cb(
    pa_context* context, void* mainloop);
void stream_state_cb(
    pa_stream *stream, void *mainloop);
void stream_success_cb(
    pa_stream *stream, int success, void *userdata);


// All "magic" happens here. Userdata is a pointer to ringbuffer.
void stream_write_cb(
    pa_stream *stream, std::size_t requested_bytes, void *userdata);

class PulseAudioOutputHandler {
 public:
  PulseAudioOutputHandler(const std::shared_ptr<SampleBuffer>& buffer);
  ~PulseAudioOutputHandler();

  void SetBuffer(const std::shared_ptr<SampleBuffer>& buffer);
  void Stop();
  void Start();

 private:
  pa_threaded_mainloop *mainloop;
  pa_mainloop_api *mainloop_api;
  pa_context *context;
  pa_stream *stream;
};

inline SampleType WaveToSample(float waveform) {
  waveform = std::clamp(waveform, -1.0f, 1.0f);
  if (waveform > 0) {
    return waveform * std::numeric_limits<SampleType>::max();
  } else {
    return -waveform * std::numeric_limits<SampleType>::min();
  }
}

// Helper for writing to intermediate buffer and ring buffer
class SampleWriter {
 public:
  SampleWriter(std::shared_ptr<SampleBuffer> buffer) 
    : buffer_(std::move(buffer))
    , samples_(buffer_->Size()) {
  }
  
  // ! Best way to get most accurate time  !
  float GetTimestamp() const {
    auto position = buffer_->Position();
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

class OutputNode : public Node {
 public:
  OutputNode(std::shared_ptr<SampleWriter> writer) 
      : writer_(writer) {
    auto signal = std::make_shared<Input>("signal", Dt::kFloat, this, 0.0f);
    inputs = {signal};
  }
 protected:
  void Process(float time) override {
    float signal = inputs[0]->GetValue<float>();
    writer_->Write(signal);
  }
  
  std::shared_ptr<SampleWriter> writer_;
};
