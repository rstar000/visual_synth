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

#include "node_graph.h"
#include "audio_thread.h"


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
    if (channel.Pressed()) {
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

int main() {
  auto bridge = std::make_shared<Bridge>();
  Gui gui(bridge);
  auto key_state = gui.GetKeyboardState();
  const size_t latency = 2;  // 1 ms
  size_t buf_size = kSampleRate * (latency / 1000.0f);
  auto buffer = std::make_shared<SampleBuffer>(buf_size);
  auto node_graph = std::make_shared<NodeGraph>();
  auto sample_writer = std::make_shared<SampleWriter>(buffer);
  auto tracker = std::make_shared<KeyboardTracker>(5, key_state);
  
  bridge->node_graph = node_graph;
  bridge->tracker = tracker;
  bridge->writer = sample_writer;

  auto tracker_node = std::make_shared<TrackerNode>(tracker, 0);
  auto unpack_node = std::make_shared<TrackerUnpackNode>();
  auto sine_node = std::make_shared<SineOscillatorNode>();
  auto output_node = std::make_shared<OutputNode>(sample_writer);

  // unpack_node->GetInputByName("channel")->Connect(tracker_node->GetOutputByName("channel"));
  // sine_node->GetInputByName("freq")->Connect(unpack_node->GetOutputByName("freq"));
  // sine_node->GetInputByName("amp")->Connect(unpack_node->GetOutputByName("vel"));
  // output_node->GetInputByName("signal")->Connect(sine_node->GetOutputByName("signal"));
  
  bridge->AddNode(tracker_node);
  bridge->AddNode(unpack_node);
  bridge->AddNode(sine_node);
  bridge->AddNode(output_node);

  AudioThread audio(tracker, sample_writer, node_graph);
  PulseAudioOutputHandler handler(buffer);
  handler.Start();
  audio.Start();
  gui.Spin();  // Block main thread
  return 0;
}
