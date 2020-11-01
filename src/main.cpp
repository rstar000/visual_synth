#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <limits>
#include <chrono>
#include <algorithm>
#include <thread>

#include "output.h"
#include "keyboard.h"
#include "note.h"
#include "waveform.h"
#include "gui.h"
#include "tracker.h"

SampleType WaveToSample(float waveform) {
  waveform = std::clamp(waveform, -1.0f, 1.0f);
  if (waveform > 0) {
    return waveform * std::numeric_limits<SampleType>::max();
  } else {
    return -waveform * std::numeric_limits<SampleType>::min();
  }
}

// Computes output for one channel. 
class Pipeline {
 public:
  Pipeline(std::shared_ptr<const Tracker> tracker, size_t idx)
    : tracker_(tracker)
    , channel_(idx)
    , osc_(440.0, 0.0, 0.1)
  {
  }

  float Value(float time) {
    auto& channel = tracker_->GetChannel(channel_);
    if (channel.pressed) {
      osc_.SetAmp(0.2f);
      osc_.SetFreq(channel.note.frequency);
    } else {
      osc_.SetAmp(0.0f);
    }

    return osc_.Value(time);
  }

 private:
  std::shared_ptr<const Tracker> tracker_;
  size_t channel_;
  Square osc_;
};

// Mixes multiple channels in to one output
class Synthesizer {
 public:
  Synthesizer(std::shared_ptr<const Tracker> tracker)
    : tracker_(tracker) {
    for (size_t i = 0; i < tracker_->GetNumChannels(); ++i) {
      pipelines_.emplace_back(tracker_, i);
    }
  }

  float Value(float time) {
    float total = 0.0;
    for (size_t i = 0; i < tracker_->GetNumChannels(); ++i) {
      total += pipelines_[i].Value(time);
    }

    return total;
  }

 private:
  std::shared_ptr<const Tracker> tracker_;
  std::vector<Pipeline> pipelines_;
};

// Spins and provides data for audio backend
class AudioThread {
 public:
  AudioThread(std::shared_ptr<Tracker> tracker, float latency_ms)
      : buf_size_((static_cast<float>(latency_ms) / 1000) * kSampleRate)
      , ring_buffer_(std::make_shared<SampleBuffer>(buf_size_))
      , tracker_(tracker)
      , synth_(tracker_)
      , output_(ring_buffer_)
      , samples(buf_size_)
  {
    output_.SetBuffer(ring_buffer_);
    output_.Start();
  }

  ~AudioThread() {
    Stop();
  }

  void Start() {
    running_ = true;
    thread_ = std::thread(&AudioThread::Spin, this);
    std::cout << "Audio thread started." << std::endl;
  }

  void Stop() {
    running_ = false;
    if (thread_.joinable()) {
      thread_.join();
    }
    std::cout << "Audio thread stopped." << std::endl;
  }

 private:
  void Spin() {
    while (running_) {
      size_t ready_to_write = 0;
      while (!(ready_to_write = ring_buffer_->ReadyToWrite())) {
        // Busy wait until data is consumed.
        std::this_thread::yield();
        continue;
      }

      size_t begin_ts = ring_buffer_->Position();
      tracker_->Update(begin_ts);

      for (size_t i = 0; i < ready_to_write; ++i) {
        float time_sec = (begin_ts + i) / static_cast<float>(kSampleRate);
        float waveform = synth_.Value(time_sec);
        SampleType sample = WaveToSample(waveform);
        samples[i] = sample;
      }

      ring_buffer_->Write(samples, ready_to_write);
    }
  }

  std::size_t buf_size_;
  std::shared_ptr<SampleBuffer> ring_buffer_;
  std::shared_ptr<Tracker> tracker_;
  Synthesizer synth_;
  PulseAudioOutputHandler output_;
  std::vector<SampleType> samples;

  std::thread thread_;
  bool running_ = false;
};

int main() {
  Gui gui;
  auto key_state = gui.GetKeyboardState();
  const size_t latency = 1;  // 1 ms

  // Input data for the synthesizer.
  std::shared_ptr<Tracker> tracker = std::make_shared<KeyboardTracker>(5, key_state);
  AudioThread audio(tracker, latency);
  audio.Start();
  gui.Spin();  // Block main thread
  return 0;
}
