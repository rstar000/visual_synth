#pragma once

#include <cmath>
#include <vector>
#include <string>
#include <map>

#include "util.h"

const float kFrequencyMultiplier = std::pow(2.0f, 1.0f / 12.0f);
const float kA440 = 440.0f;

inline float ComputeFrequency(int octave, int half_steps) {
  float octave_base = kA440 * std::pow(2.0f, octave);
  return octave_base * (std::pow(kFrequencyMultiplier, half_steps));
}

struct Note {
  Note(int octave = 0, int half_steps = 0)
    : octave(octave)
    , half_steps(half_steps)
    , frequency(ComputeFrequency(octave, half_steps)) {
  }
  int octave;
  int half_steps;
  float frequency;
};

struct Channel {
  Note note; 
  float begin;    // Exact time when the note began playing.
  float end;      // If pressed == true, this should be ignored.
  float velocity;          // Value in [0, 1] range. For organ/wind instruments.
  
  bool Pressed() const {
    return begin > end;
  }
};

struct Octave {
  Octave(int idx = 0) {
    std::vector<std::string> names = {
      "C", "Db", "D", "Eb", "E", "F",
      "Gb", "G", "Ab", "A", "Bb", "B"};

    for (int step = 0; step < 12; ++step) {
      // Base is A, start octave at C.
      notes_.emplace_back(idx, step - 10);
      codes_[names[step]] = step;
    }
  }

  const Note& Get(const std::string& code) const {
    auto it = codes_.find(code);
    ASSERT(it != codes_.end());
    return notes_[it->second];
  }

  const Note& Get(std::size_t index) const {
    ASSERT(index < 12);
    return notes_[index];
  }

 private:
  std::vector<Note> notes_;
  std::map<std::string, int> codes_;
};
