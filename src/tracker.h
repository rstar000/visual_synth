#pragma once

#include <optional>

#include "note.h"

struct Channel {
  Note note; 
  std::size_t begin;    // Exact time when the note began playing.
  std::size_t end;      // If pressed == true, this should be ignored.
  float force;          // Value in [0, 1] range. For organ/wind instruments.
  bool pressed = false;
};

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
  virtual void Update(size_t timestamp) = 0;

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
      if (!channels_[i].pressed) {
        return i;
      }
    }

    return {};
  }

  std::vector<Channel> channels_;
};

