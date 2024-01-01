#pragma once

#include <cmath>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "util.h"

const float kFrequencyMultiplier = std::pow(2.0f, 1.0f / 12.0f);
const float kA440 = 440.0f;  // C4
const int kHalfStepsPerOctave = 12;
const int kMidiOffset = 2 * kHalfStepsPerOctave;

struct Note {
    Note(int octave = 0, int half_steps = 0)
        : octave(octave),
          half_steps(half_steps),
          frequency(Note::ComputeFrequency(octave, half_steps)) {}

    int octave;
    int half_steps;
    float frequency;

    static float ComputeFrequency(int octave, int half_steps) {
        float octave_base = kA440 * std::pow(2.0f, octave);
        // Offset between C and A is 10 semitones
        return octave_base * (std::pow(kFrequencyMultiplier, half_steps - 10));
    }
};

struct Channel {
    Note note;
    float begin = 0.0f;  // Exact time when the note began playing.
    float end = 0.0f;
    float velocity = 0.0f;  
    bool active = false;

    bool IsPlaying() const { return begin > end; }
};

enum class Tone { C = 0, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B };

struct Octave {
    static constexpr size_t NUM_NOTES = 12;
    Octave(int idx = 0) {
        auto names = GetNames();
        for (int step = 0; step < kHalfStepsPerOctave; ++step) {
            // Base is A, start octave at C.
            notes_.emplace_back(idx, step);
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

    const Note& Get(uint32_t toneIdx) const {
        return notes_.at(toneIdx);
    }

    static std::vector<std::string> GetNames() {
        return std::vector<std::string> {"C",  "Db", "D",  "Eb", "E",  "F",
                                          "Gb", "G",  "Ab", "A",  "Bb", "B"};

    }

   private:
    std::vector<Note> notes_;
    std::map<std::string, int> codes_;
};

struct NoteLookup {
    struct NoteDesc {
        std::string name;
        Note note;
    };
    static constexpr size_t NUM_OCTAVES = 8;

    NoteLookup() {
        auto names = Octave::GetNames();
        m_octaveOffset.resize(NUM_OCTAVES);
        for (int octaveIdx = 0; octaveIdx < NUM_OCTAVES; ++octaveIdx) {
            m_octaveOffset.push_back(m_notes.size());
            Octave oct(octaveIdx - 4);
            for (size_t noteIdx = 0; noteIdx < Octave::NUM_NOTES; ++noteIdx) {
                std::string noteName = string_format("%s%d", names.at(noteIdx).c_str(), octaveIdx);
                auto note = oct.Get(noteIdx);
                m_notes.push_back(NoteDesc {
                    .name = noteName,
                    .note = note
                });
            }
        }
    }

    NoteDesc const& Get(uint32_t noteIdx) const {
        return m_notes[noteIdx];
    }

    uint32_t Size() const {
        return m_notes.size();
    }

private:
    std::vector<NoteDesc> m_notes;
    std::vector<uint32_t> m_octaveOffset;
};

struct MidiNote {
    uint8_t note_idx = 0;
    uint8_t velocity = 0;
    bool is_active = false;
};

struct MidiTracker {
    MidiTracker(int num_voices) : num_voices(num_voices), note_to_voice(128) {
        voices.resize(num_voices);
        for (int i = 0; i < num_voices; ++i) {
            voices_queue.push(i);
        }
        controls.resize(256);
    }

    void NoteOn(uint8_t note_idx, uint8_t velocity) {
        int voice_id = GetVoice();
        if (voice_id < 0) {
            // All channels full
            return;
        }

        note_to_voice[note_idx] = voice_id;
        voices[voice_id] = MidiNote{
            .note_idx = note_idx, .velocity = velocity, .is_active = true};
    }

    void NoteOff(uint8_t note_idx, uint8_t velocity) {
        int voice_idx = note_to_voice[note_idx];
        note_to_voice[note_idx] = -1;
        if (voices[voice_idx].note_idx != note_idx) {
            // Note mismatch;
            return;
        }

        voices[voice_idx].is_active = false;
        voices[voice_idx].velocity = 0.0f;
    }

    void ControlChange(uint8_t controlIdx, uint8_t value)
    {
        controls[controlIdx] = value;
    }
    
    void PitchBend(uint8_t value)
    {

    }

    int GetVoice() {
        int voice_id = voices_queue.front();
        voices_queue.pop();
        voices_queue.push(voice_id);

        if (!voices[voice_id].is_active) {
            return voice_id;
        }

        return -1;
    }

    int num_voices;
    std::vector<int> note_to_voice;
    std::vector<MidiNote> voices;

    std::queue<int> voices_queue;

    std::vector<uint8_t> controls;
};
