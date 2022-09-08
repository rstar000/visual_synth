#pragma once

#include <cmath>
#include <vector>
#include <string>
#include <map>

#include "util.h"

const float kFrequencyMultiplier = std::pow(2.0f, 1.0f / 12.0f);
const float kA440 = 440.0f;
const int kHalfStepsPerOctave = 12;
const int kMidiOffset = 2 * kHalfStepsPerOctave;  

struct Note {
  Note(int octave = 0, int half_steps = 0)
    : octave(octave)
    , half_steps(half_steps)
    , frequency(Note::ComputeFrequency(octave, half_steps)) {
  }

  int octave;
  int half_steps;
  float frequency;
  
  static float ComputeFrequency(int octave, int half_steps) {
    float octave_base = kA440 * std::pow(2.0f, octave);
    return octave_base * (std::pow(kFrequencyMultiplier, half_steps));
  }
};

struct Channel {
  Note note; 
  float begin = 0.0f;    // Exact time when the note began playing.
  float end = 0.0f;      
  float velocity = 0.0f;     // Value in [0, 1] range. For organ/wind instruments.
  
  bool IsPlaying() const {
    return begin > end;
  }
};

enum class Tone {
  C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B
};

struct Octave {
  Octave(int idx = 0) {
    std::vector<std::string> names = {
      "C", "Db", "D", "Eb", "E", "F",
      "Gb", "G", "Ab", "A", "Bb", "B"};

    for (int step = 0; step < kHalfStepsPerOctave; ++step) {
      // Base is A, start octave at C.
      notes_.emplace_back(idx, step - 10);
      codes_[names[step]] = step;
    }
  }

  const Note& Get(const std::string& name) const {
    auto it = codes_.find(name);
    ASSERT(it != codes_.end());
    return notes_[it->second];
  }

  const Note& Get(Tone tone) const {
    return notes_[static_cast<size_t>(tone)];
  }

 private:
  std::vector<Note> notes_;
  std::map<std::string, int> codes_;
};


struct MidiNote {
  uint8_t note_idx = 0;
  uint8_t velocity = 0;;
  bool is_active = false;
};

struct MidiTracker {
  MidiTracker(int num_voices) 
    : num_voices(num_voices)
    , note_to_voice(128)
    , voices(num_voices) {}

  void NoteOn(uint8_t note_idx, uint8_t velocity) {
    int new_voice = FindFreeVoice();
    if (new_voice < 0) { 
      // All channels full
      return;
    }
    
    note_to_voice[note_idx] = new_voice;
    voices[new_voice] = MidiNote{
      .note_idx = note_idx,
      .velocity = velocity,
      .is_active = true
    };
  }

  void NoteOff(uint8_t note_idx, uint8_t velocity) {
    int voice_idx = note_to_voice[note_idx];
    if (voices[voice_idx].note_idx != note_idx) {
      // Note mismatch;
      return;
    }
    
    voices[voice_idx].is_active = false;
    voices[voice_idx].velocity = velocity;
  }
  
  int FindFreeVoice() const {
    for (size_t i = 0; i < voices.size(); ++i) {
      if (!voices[i].is_active) {
        return i;
      }
    }
    
    return -1;
  }
  
  int num_voices;
  std::vector<int> note_to_voice;
  std::vector<MidiNote> voices;
};