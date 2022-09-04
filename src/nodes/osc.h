#pragma once

#include <cmath>
#include "node.h"
#include "node_types.h"

#include "imgui.h"

class OscillatorNode : public Node {
 public:
  OscillatorNode() {
    auto freq = std::make_shared<Input>("freq", PinDataType::kFloat, this, 440.0f);
    auto amp = std::make_shared<Input>("amp", PinDataType::kFloat, this, 0.5f);
    auto phase = std::make_shared<Input>("phase", PinDataType::kFloat, this, 0.0f);
    auto signal = std::make_shared<Output>("signal", PinDataType::kFloat, this, 0.0f);

    inputs = {freq, amp, phase};
    outputs = {signal};
  }
};

struct SineOscillatorNode : public OscillatorNode {
  static inline const std::string DISPLAY_NAME = "Sine wave";
  static inline const NodeType TYPE = NodeType::SINE_OSC;

  SineOscillatorNode() : OscillatorNode() { 
    type = TYPE;
    display_name = DISPLAY_NAME;
    
    freq_label = GenLabel("1", this, "freq");
    amp_label = GenLabel("2", this, "amp");
  }

  ~SineOscillatorNode() {}

  void Process(float time) override {
    if (inputs[0]->IsConnected()) {
      freq = inputs[0]->GetValue<float>();
    } else {
      freq = freq_param;
    }

    if (inputs[1]->IsConnected()) {
      amp = inputs[1]->GetValue<float>();
    } else {
      amp = amp_param;
    }
    
    float phase = inputs[2]->GetValue<float>();
    float wave = amp * sin(time * 2.0 * M_PI * freq + phase);
    outputs[0]->SetValue<float>(wave);
  }
  
  void Draw() override {
    ImGui::PushItemWidth(100.0f);
    ImGui::SliderFloat(freq_label.c_str(), &freq_param, 0.0f, 1000.0f, "%.2f", ImGuiSliderFlags_None);
    ImGui::SliderFloat(amp_label.c_str(), &amp_param, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_None);
    ImGui::PopItemWidth();
  }
  
  void Save(nlohmann::json& j) const override {
    JsonSetValue(j, "freq", freq_param);
    JsonSetValue(j, "amp", amp_param);
  }

  void Load(const nlohmann::json& j) override {
    JsonGetValue<float>(j, "freq", freq_param);
    JsonGetValue<float>(j, "amp", amp_param);
  }

 private:
  float freq;
  float amp;
  
  float freq_param = 440.0f;
  float amp_param = 0.5f;
  
  std::string freq_label;
  std::string amp_label;
};

struct SquareOscillatorNode : public OscillatorNode {
  static inline const std::string DISPLAY_NAME = "Square wave";
  static inline const NodeType TYPE = NodeType::SQUARE_OSC;

  SquareOscillatorNode() : OscillatorNode() { 
    type = TYPE;
    display_name = DISPLAY_NAME;
  }

  ~SquareOscillatorNode() {}

  void Process(float time) override {
    if (inputs[0]->IsConnected()) {
      freq = inputs[0]->GetValue<float>();
    }

    if (inputs[1]->IsConnected()) {
      amp = inputs[1]->GetValue<float>();
    }

    float t = std::fmod(time,  1.0f / freq) * freq;
    float wave = amp * (t > 0.5f ? 1.0f : -1.0f);
    outputs[0]->SetValue<float>(wave);
  }
  
 private: 
  float freq = 440.0f;
  float amp = 0.5f;
};