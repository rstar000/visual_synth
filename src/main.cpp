#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <limits>
#include <chrono>
#include <algorithm>

#include "output.h"
#include "waveform.h"
#include "gui.h"

SampleType WaveToSample(float waveform) {
  waveform = std::clamp(waveform, -1.0f, 1.0f);
  if (waveform > 0) {
    return waveform * std::numeric_limits<SampleType>::max();
  } else {
    return -waveform * std::numeric_limits<SampleType>::min();
  }
}

int main() {
  Gui gui;
  gui.Spin();
  size_t buf_size = 0.05 * kSampleRate;
  std::cout << "Buffer: " << buf_size << std::endl;
  auto ring_buf = std::make_shared<SampleBuffer>(buf_size);

  PulseAudioOutputHandler pulse(ring_buf);
  pulse.SetBuffer(ring_buf);
  pulse.Start();

  std::vector<SampleType> samples(buf_size);

  auto start_time = std::chrono::system_clock::now();

  while (true) {
    size_t ready_to_write = 0;
    while (!(ready_to_write = ring_buf->ReadyToWrite())) {
      // Busy wait until data is consumed.
      continue;
    }

    size_t begin = ring_buf->Position();

    std::cout << "Writing " << ready_to_write << std::endl;
    std::cout << "Measured latency: " << ring_buf->ReadyToRead() / static_cast<double>(kSampleRate) << std::endl;
    std::cout << "Timestamp buffer: " << begin  / static_cast<float>(kSampleRate) << std::endl;
    std::cout << "Timestamp clock: " <<
      std::chrono::duration_cast<
        std::chrono::milliseconds>(
          std::chrono::system_clock::now() - start_time).count() / 1000.0
              << std::endl;

    auto wave = Sine(440.0f, 0.2, 0.2);

    for (size_t i = 0; i < ready_to_write; ++i) {
      float timestamp = (begin + i) / static_cast<float>(kSampleRate);
      float waveform = wave.Value(timestamp);
      SampleType sample = WaveToSample(waveform);
      samples[i] = sample;
    }

    ring_buf->Write(samples, ready_to_write);
  }

  return 0;
}
