#pragma once

#include <optional>

#include "note.h"
#include "node.h"
#include "channel.h"
// Aggregates multiple musical channels. Each channel contains
// information about a currently playing (or silent) note on a
// real or virtual keyboard. A tracker is needed to transform
// discrete act of pressing a key into continuous time segments
// of when the notes were being played. Up to `num_channels` 
// notes can be playing at the same time.
//
// This is an abstract base class. It can be subclassed to implement
// adapters for different input devices like keyboard, MIDI or 
// a virtual modtracker.
class Tracker {
 public:
  Tracker(int num_channels) 
    : channels_(num_channels)
  {}

  // This method is called every audio frame.
  // It should pull all necessary data into the channels.
  // @param timestamp: beginning of the frame. 
  virtual void Update(float time) = 0;

  const Channel& GetChannel(size_t idx) const {
    return channels_[idx];
  }

  size_t GetNumChannels() const {
    return channels_.size();
  }

 protected:
  // Find a channed that is currently not playing a note.
  // Returns channel index. If not found, returns nullopt.
  std::optional<size_t> FindFreeChannel() const {
    for (size_t i = 0; i < channels_.size(); ++i) {
      if (!channels_[i].Pressed()) {
        return i;
      }
    }

    return {};
  }

  std::vector<Channel> channels_;
};


class TrackerNode : public Node {
 public:
  TrackerNode(std::shared_ptr<Tracker> tracker, int channel)
    : tracker(tracker), channel_idx(channel)
  {
    Channel default_channel;
    auto channel_output = std::make_shared<Output>(
      "channel", Dt::kChannel, this, default_channel);

    outputs = {channel_output};
  }

  void Process(float timestamp) override {
    const auto& channel =tracker->GetChannel(channel_idx);
    outputs[0]->SetValue<Channel>(channel);
  }

 private:
  std::shared_ptr<Tracker> tracker;
  int channel_idx;
};


class TrackerUnpackNode : public Node {
 public:
  TrackerUnpackNode() {
    inputs = {
      std::make_shared<Input>(
        "channel", Dt::kChannel, this, Channel())
    };

    outputs = {
      std::make_shared<Output>(
        "freq", Dt::kFloat, this, 440.0f),
      std::make_shared<Output>(
        "vel", Dt::kFloat, this, 0.0f),
      std::make_shared<Output>(
        "begin", Dt::kFloat, this, 0.0f),
      std::make_shared<Output>(
        "end", Dt::kFloat, this, 0.0f)
    };
  }

  void Process(float time) override {
    auto channel = inputs[0]->GetValue<Channel>();
    outputs[0]->SetValue<float>(channel.note.frequency);
    outputs[1]->SetValue<float>(channel.velocity);
    outputs[2]->SetValue<float>(channel.begin);
    outputs[3]->SetValue<float>(channel.end);
  }
};