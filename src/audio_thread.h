#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
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
    std::shared_ptr<RtAudioOutputHandler> rt_out,
    std::shared_ptr<Multigraph> graph)
      : graph(graph)
      , rt_out(rt_out)
      , writer(rt_out->GetBuffer())
      , output(std::make_shared<AudioOutput>()) {
  }

  ~AudioThread() {
    Stop();
  }

  void Start() {
    rt_out->Start();
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
    
    rt_out->Stop();
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
  
  auto GetOutput() {
    return output;
  }
  
  float GetTimestamp() {
    return writer.GetTimestamp();
  }

 private:
  void Spin() {
    while (running_) {
      size_t ready_to_write = 0;
      while (running_ && !(ready_to_write = writer.ReadyToWrite())) {
        // Busy wait until data is consumed.
        std::this_thread::yield();
        continue;
      }
      
      if (!running_) {
        break;
      }

      {
      // Graph mutex locked
      auto access = graph->GetAccess();
      auto nodes = access->GetSortedNodes();

      for (size_t i = 0; i < ready_to_write; ++i) {
        float timestamp = writer.GetTimestamp();
        for (auto node : nodes) {
          node->Process(timestamp);
        }
        
        writer.Write(output->wave);
      }

      }
      
      writer.Flush();
    }
  }

  std::shared_ptr<Multigraph> graph;
  std::shared_ptr<RtAudioOutputHandler> rt_out;
  SampleWriter writer;
  std::shared_ptr<AudioOutput> output;

  std::thread thread_;
  bool running_ = false;
};
