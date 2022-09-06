#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <chrono>
#include <limits>
#include <chrono>
#include <algorithm>
#include <thread>

#include "output.h"
#include "multigraph.h"


// Spins and provides data for audio backend
class AudioThread {
 public:
  AudioThread(
    std::shared_ptr<Multigraph> graph,
    const Context& ctx)
      : graph(graph)
      , writer(ctx.writer)
      , num_voices(ctx.num_voices) 
      , num_samples(ctx.num_samples) {
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
  
  void PlayPause() {
    if (running_) {
      Stop();
    } else {
      Start();
    }
  }
  
  bool IsPlaying() const {
    return running_;
  }
  
  float GetTimestamp(int sample_idx) {
    return writer->GetTimestamp(sample_idx);
  }

  float GetTimestamp() {
    return writer->GetTimestamp(0);
  }

 private:
  void Spin() {
    while (running_) {
      size_t ready_to_write = 0;
      while (running_ && !(ready_to_write = writer->ReadyToWrite())) {
        // Busy wait until data is consumed.
        continue;
      }
      
      if (!running_) {
        break;
      }

      {
      // Graph mutex locked
      auto access = graph->GetAccess();
      auto nodes = access->GetSortedNodes();
      
      for (int v = 0; v < num_voices; ++v) {
        for (auto node : nodes) {
          for (size_t s = 0; s < ready_to_write; ++s) {
            float timestamp = writer->GetTimestamp(s);
            node->Process(v, s, timestamp);
          }
        }
      }

      }
      
      // {
      //   TimeIt([] (const auto& x) { std::cout << "Flush " << x << std::endl;});
      writer->Flush(ready_to_write);
      // }
    }
  }

  std::shared_ptr<Multigraph> graph;
  std::shared_ptr<SampleWriter> writer;

  std::thread thread_;
  bool running_ = false;
  
  int num_voices;
  int num_samples;
};
