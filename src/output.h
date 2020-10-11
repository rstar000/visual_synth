#pragma once

#include <cstdio>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>

#include <pulse/pulseaudio.h>
#include "ring_buffer.h"

const int kSampleRate = 44100;
const pa_sample_format kSampleFormat = PA_SAMPLE_S16LE;
const int kNumChannels = 2;

using SampleType = std::int16_t;
using SampleBuffer = RingBuffer<SampleType>;

void context_state_cb(
    pa_context* context, void* mainloop);
void stream_state_cb(
    pa_stream *stream, void *mainloop);
void stream_success_cb(
    pa_stream *stream, int success, void *userdata);
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
