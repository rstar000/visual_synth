#pragma once

#include <algorithm>
#include <iostream>
#include <cstdint>
#include <map>
#include <thread>

#include <SDL2/SDL.h>
#include "util.h"
#include "tracker.h"

const size_t kKeysInOctave = 12;

// Turns your qwerty keyboard into a one octave piano!
// This is a hacky way to get keyboard state in real time. 
// Also this is tied to SDL tick frequency, so latency is 
// the same as monitor FPS! TODO: find a better way.
struct KeyboardState {
 public:
  KeyboardState() {
    std::fill(octave.begin(), octave.end(), 0);
    FillKeysMap();
  }

  void ProcessEvent(const SDL_Event& event) {
    uint8_t value = 0;
    if (event.type == SDL_KEYDOWN) {
      value = 1;
    }

    auto sym = event.key.keysym.sym;
    if (auto it = key_to_note.find(sym); it != key_to_note.end()) {
      octave[it->second] = value;
    }
  }

  bool GetState(int key) const {
    ASSERT(key >= 0 && key <= 12);
    return octave[key];
  }

  std::array<uint8_t, 12> octave;
 private:
  void FillKeysMap() {
    std::vector<int> keys = {
      SDLK_z, SDLK_s, SDLK_x, SDLK_d, SDLK_c, SDLK_v, SDLK_g, SDLK_b, SDLK_h, SDLK_n, SDLK_j, SDLK_m
    };
    for (std::size_t i = 0; i < keys.size(); ++i) {
      key_to_note[keys[i]] = i;
    }
  }
  std::map<int, uint8_t> key_to_note;
};

inline std::optional<int> GetPressedKey(const KeyboardState& oct) {
  for (size_t i = 0; i < kKeysInOctave; ++i)  {
    if (oct.GetState(i)) {
      return i;
    }
  }

  return {};
}

// Can track multiple notes played on a virtual keyboard with 1 octave.
class KeyboardTracker : public Tracker {
 public:
  KeyboardTracker(int num_channels, std::shared_ptr<const KeyboardState> state)
    : Tracker(num_channels)
    , state_(state) {

    // -1 indicates that a key is not assigned to any channel
    std::fill(key_to_channel_.begin(), key_to_channel_.end(), -1);
  }

  void SetOctave(int oct) {
    oct_ = Octave(oct);
  }

  void Update(size_t timestamp) override {
    for (size_t key = 0; key < kKeysInOctave; ++key) {
      int active_channel = key_to_channel_[key];
      int is_pressed = state_->GetState(key);

      if (active_channel != -1 && is_pressed) {
        // Continue playing the note.
        continue;
      } else if (active_channel == -1 && is_pressed) {
        auto found = FindFreeChannel();
        if (!found) {
          continue;
        }

        auto& channel = channels_[*found];
        channel.pressed = true;
        channel.note = oct_.Get(key);
        channel.begin = timestamp;
        channel.force = 1.0f;
        key_to_channel_[key] = *found;
      } else if (active_channel != -1 && !is_pressed) {
        auto& channel = channels_[active_channel];
        channel.pressed = false;
        channel.end = timestamp;
        channel.force = 0.0f;
        key_to_channel_[key] = -1;
      }
    }
  }

 private:
  Octave oct_;
  std::shared_ptr<const KeyboardState> state_;
  std::array<int, kKeysInOctave> key_to_channel_;
};
