#pragma once

#include <cmath>
#include "node.h"
#include "node_types.h"
#include "util.h"

#include "imgui.h"

// Will enable the note every tick in the measure for a specified amount of time
struct ClockNode : public Node {
  static inline const std::string DISPLAY_NAME = "Clock";
  static inline const NodeType TYPE = NodeType::CLOCK;

  ClockNode() : oct(1) {
    type = TYPE;
    display_name = DISPLAY_NAME;

    inputs = {};
    outputs = {
      std::make_shared<Output>("ch", PinDataType::kChannel, this, Channel{})
    };

    bpm_slider_label = GenLabel("slider", this);
    bpm_label = GenLabel("bpm", this);
    measure_label = GenLabel("measure", this);
    note_size_label = GenLabel("size", this);
  }

  ~ClockNode() {}

  void Process(float time) override {
    Channel& value = outputs[0]->GetValue<Channel>();
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

  ChannelUnpackNode() {
    type = TYPE;
    display_name = DISPLAY_NAME;

    inputs = {
      std::make_shared<Input>("ch", PinDataType::kChannel, this, Channel{})
    };

    outputs = {
      std::make_shared<Output>("freq",  PinDataType::kFloat, this, 0.0f),
      std::make_shared<Output>("begin", PinDataType::kFloat, this, 0.0f),
      std::make_shared<Output>("end",   PinDataType::kFloat, this, 0.0f),
      std::make_shared<Output>("vel",   PinDataType::kFloat, this, 0.0f)
    };
  }
  
  ~ChannelUnpackNode() {}

  void Process(float time) override {
    auto in = inputs[0]->GetValue<Channel>();
    outputs[0]->SetValue<float>(in.note.frequency);
    outputs[1]->SetValue<float>(in.begin);
    outputs[2]->SetValue<float>(in.end);
    outputs[3]->SetValue<float>(in.velocity);
  }
};
