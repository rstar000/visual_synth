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
#include "note.h"
#include "waveform.h"
#include "tracker.h"
#include "node_graph.h"


// Spins and provides data for audio backend
class AudioThread {
 public:
  AudioThread(
    std::shared_ptr<Tracker> tracker,
    std::shared_ptr<SampleWriter> writer,
    std::shared_ptr<NodeGraph> graph)
      : graph_(graph)
      , tracker_(tracker)
      , writer_(writer) {
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
      while (running_ && !(ready_to_write = writer_->ReadyToWrite())) {
        // Busy wait until data is consumed.
        std::this_thread::yield();
        continue;
      }
      
      if (!running_) {
        break;
      }

      tracker_->Update(writer_->GetTimestamp());

      for (size_t i = 0; i < ready_to_write; ++i) {
        float timestamp = writer_->GetTimestamp();
        float signal = 0.0f;
        for (size_t channel_idx = 0; channel_idx < tracker_->NumChannels(); ++channel_idx) {
          tracker_->SetActiveChannel(channel_idx);
          signal += graph_->Run(timestamp);  
        }
        
        writer_->Write(signal);
      }
      
      writer_->Flush();
    }
  }

  std::shared_ptr<NodeGraph> graph_;
  std::shared_ptr<SampleWriter> writer_;
  std::shared_ptr<Tracker> tracker_;

  std::thread thread_;
  bool running_ = false;
};

