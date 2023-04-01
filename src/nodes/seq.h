#pragma once

#include <cmath>
#include "node.h"
#include "node_types.h"
#include "midi.h"
#include "util.h"

#include "imgui.h"

#include "ImGui_Piano_imp.h"

// Will enable the note every tick in the measure for a specified amount of time
struct ClockNode : public Node {
  static inline const std::string DISPLAY_NAME = "Clock";
  static inline const NodeType TYPE = NodeType::CLOCK;

  ClockNode(const NodeParams& ctx) : Node(ctx), oct(1) {
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddOutput("ch", PinDataType::kChannel, Channel{});

    bpm_slider_label = GenLabel("slider", this);
    bpm_label = GenLabel("bpm", this);
    measure_label = GenLabel("measure", this);
    note_size_label = GenLabel("size", this);
  }

  ~ClockNode() {}

  void Process(float time) override {
    Channel& value = GetOutputValue<Channel>(0);
    float quater_beat_length = 60.0f / bpm_param;
    float measure_beat_length = quater_beat_length * 4.0f / static_cast<float>(measure_param[1]);
    int beat_id = static_cast<int>(time / measure_beat_length) % measure_param[0];

    value.note = beat_id == 0 ? oct.Get(Tone::C) : oct.Get(Tone::G);
    value.begin = std::floor(time / measure_beat_length) * measure_beat_length;
    value.end = value.begin + measure_beat_length * note_size;
    value.velocity = time > value.end ? 0.0f : 1.0f;
  }
  
  void Draw() override {
    ImGui::PushItemWidth(100.0f);
    ImGui::SliderFloat(bpm_slider_label.c_str(), &bpm_param, 0.0f, 1000.0f, "%.2f", ImGuiSliderFlags_None);
    ImGui::InputFloat(bpm_label.c_str(), &bpm_param, 0.0f, 0.0f, "%.2f", ImGuiInputTextFlags_None);
    ImGui::InputInt2(measure_label.c_str(), measure_param.data(), ImGuiInputTextFlags_None);
    ImGui::InputFloat(note_size_label.c_str(), &note_size, 0.0f, 0.0f, "%.2f", ImGuiInputTextFlags_None);
    ImGui::PopItemWidth();
  }
  
  void Save(nlohmann::json& j) const override {
    JsonSetValue(j, "bpm", bpm_param);
    JsonSetValue(j, "measure", measure_param);
    JsonSetValue(j, "note_size", note_size);
  }

  void Load(const nlohmann::json& j) override {
    JsonGetValue(j, "bpm", bpm_param);
    JsonGetValue(j, "measure", measure_param);
    JsonGetValue(j, "note_size", note_size);
  }

 protected:
  Octave oct;
  float bpm_param = 100.0f;  // Quater notes per minute
  std::array<int, 2> measure_param = {4, 4};  // Ex. 3 / 4
  float note_size = 0.5f;  // Fraction of the note in relation to measure.
  
  std::string bpm_slider_label;
  std::string bpm_label;
  std::string measure_label;
  std::string note_size_label;
};

struct ChannelUnpackNode : public Node {
  static inline const std::string DISPLAY_NAME = "Channel unpack";
  static inline const NodeType TYPE = NodeType::CHANNEL_UNPACK;

  ChannelUnpackNode(const NodeParams& ctx) : Node(ctx) {
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddInput("ch", PinDataType::kChannel, Channel{});

    AddOutput("freq",  PinDataType::kFloat, 0.0f);
    AddOutput("begin", PinDataType::kFloat, 0.0f);
    AddOutput("end",   PinDataType::kFloat, 0.0f);
    AddOutput("vel",   PinDataType::kFloat, 0.0f);
  }
  
  ~ChannelUnpackNode() {}

  void Process(float time) override {
    auto in = GetInputValue<Channel>(0);
    SetOutputValue<float>(0, in.note.frequency);
    SetOutputValue<float>(1, in.begin);
    SetOutputValue<float>(2, in.end);
    SetOutputValue<float>(3, in.velocity);
  }
};

struct KeyboardData {
  std::array<std::uint8_t, 128> keys;
};

inline bool TestPianoBoardFunct(void* UserData, int Msg, int Key, float Vel) {
    KeyboardData* kbd = static_cast<KeyboardData*>(UserData);
		if (Key >= 128) return false; // midi max keys
		if (Msg == NoteGetStatus) {
      return kbd->keys[Key];
    }
    
		if (Msg == NoteOn) { 
      kbd->keys[Key] = 1;
    }

		if (Msg == NoteOff) { 
      kbd->keys[Key] = 0;
    }
		return true;
}


struct PianoNode : public Node {
  static inline const std::string DISPLAY_NAME = "Piano";
  static inline const NodeType TYPE = NodeType::PIANO;

  PianoNode(const NodeParams& ctx) : Node(ctx) {
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddOutput("ch",  PinDataType::kChannel, Channel{});
    std::fill(kbd.keys.begin(), kbd.keys.end(), 0);
    for (int i = 0; i < 7; ++i) {
      oct.push_back(Octave(i - 2));
    }
  }
  
  ~PianoNode() {}

  void Process(float time) override {
    if (GetActiveVoice() != 0) {
      return;
    }
    auto& out = GetOutputValue<Channel>(0);
    out.velocity = 1.0f;
    
    bool pressed = false;
    for (int i = 0; i < kbd.keys.size(); ++i) {
      if (kbd.keys[i]) {
        if (i < 3) {
          break;
        }
        
        int note_idx = i - kMidiOffset;  // We have some invisible keys...
        if (note_idx < 0) {
          break;
        }
        int oct_idx = note_idx / kHalfStepsPerOctave;
        if (oct_idx >= oct.size()) {
          break;
        }
        
        int oct_note_idx = note_idx % kHalfStepsPerOctave;
        auto& note = oct[oct_idx].Get(static_cast<Tone>(oct_note_idx));
        out.note = note;
        pressed = true;

        // Just pressed
        if (!prev_pressed) {
          prev_pressed = true;
          press_begin = time;
        }
        out.begin = press_begin;
        out.end = time + 100.0f;
        out.velocity = 1.0f;
        break;
      }
    }
      
    if (!pressed) {
      if (prev_pressed) {
        // Just released
        // prev_note = -1;
        prev_pressed = false;
        press_end = time;
      }
      
      out.end = press_end;
      out.velocity = 1.0f;
    }
  }
  
  void Draw() {
    ImGui_PianoKeyboard("##piano", ImVec2(1024, 100), &prev_note, 21, 108, TestPianoBoardFunct, &kbd, nullptr);
  }
  
 private:
  KeyboardData kbd;
  int prev_note;
  std::vector<Octave> oct;
  
  bool prev_pressed = false;
  float press_begin = 0.0f;
  float press_end = 0.0f;
};

struct MidiNode : public Node {
  static inline const std::string DISPLAY_NAME = "Midi in";
  static inline const NodeType TYPE = NodeType::MIDI;

  struct VoiceState {
    bool is_active = false;
    float begin_ts = 0.0f;  // Last change
    float end_ts = 0.0f;  // Last change
  };

  MidiNode(const NodeParams& ctx) 
      : Node(ctx)
      , tracker(ctx.tracker)
      , state(NumVoices()) {

    type = TYPE;
    display_name = DISPLAY_NAME;

    AddOutput("ch",  PinDataType::kChannel, Channel{});
    for (int i = 0; i < 7; ++i) {
      oct.push_back(Octave(i - 2));
    }
  }
  
  ~MidiNode() {}
  
  bool GetNote(int note_idx, Note* dst) {
    if (note_idx < 3) {
      return false;
    }
    
    note_idx -= 24;  // We have some invisible keys...
    int oct_idx = note_idx / 12;
    if (oct_idx >= oct.size()) {
      return false;
    }
    
    int oct_note_idx = note_idx % 12;
    *dst = oct[oct_idx].Get(static_cast<Tone>(oct_note_idx));
    return true;
  }

  void Process(float time) override {
    int voice_idx = GetActiveVoice();
    auto& out = GetOutputValue<Channel>(0);
    
    if (tracker->voices[voice_idx].is_active && !state[voice_idx].is_active) {
      // Note started playing
      state[voice_idx].is_active = true;
      state[voice_idx].begin_ts = time;
      state[voice_idx].end_ts = time + 100.0f;
    } else if (!tracker->voices[voice_idx].is_active && state[voice_idx].is_active) {
      // Note stopped playing
      state[voice_idx].is_active = false;
      state[voice_idx].end_ts = time;
    } 

    // if (!state[voice_idx].is_active) {
    //   out.velocity = tracker->voices[voice_idx].velocity / 128.0f;
    //   out.begin = state[voice_idx].begin_ts;
    //   out.end = state[voice_idx].end_ts;
    // } else {
    // }
    out.velocity = tracker->voices[voice_idx].velocity / 128.0f;
    GetNote(tracker->voices[voice_idx].note_idx, &out.note);
    out.begin = state[voice_idx].begin_ts;
    out.end = state[voice_idx].end_ts;
  }
  
  std::shared_ptr<MidiTracker> tracker;
  std::vector<VoiceState> state;
  std::vector<Octave> oct;
};
