#pragma once

#include <cmath>
#include "node.h"
#include "node_types.h"

#include "imgui.h"
#include "ui/knob.h"

struct SineOscillatorNode : public Node {
  static inline const std::string DISPLAY_NAME = "Sine wave";
  static inline const NodeType TYPE = NodeType::SINE_OSC;

  SineOscillatorNode(const NodeParams& ctx) : Node(ctx) { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddInput("freq", PinDataType::kFloat, 440.0f);
    AddInput("amp", PinDataType::kFloat, 0.5f);
    AddInput("phase", PinDataType::kFloat, 0.0f);
    AddOutput("signal", PinDataType::kFloat, 0.0f);
    
    freq_label = GenLabel("1", this, "freq");
    amp_label = GenLabel("2", this, "amp");
  }

  ~SineOscillatorNode() {}

  void Process(float time) override {
    if (inputs[0]->IsConnected()) {
      freq = GetInputValue<float>(0);
    } else {
      freq = freq_param;
    }

    if (inputs[1]->IsConnected()) {
      amp = GetInputValue<float>(1);
    } else {
      amp = amp_param;
    }
    
    float phase = GetInputValue<float>(2);
    float wave = amp * sin(time * 2.0f * M_PIf * freq + phase);
    SetOutputValue<float>(0, wave);
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

struct SquareOscillatorNode : public Node {
  static inline const std::string DISPLAY_NAME = "Square wave";
  static inline const NodeType TYPE = NodeType::SQUARE_OSC;

  SquareOscillatorNode(const NodeParams& ctx) : Node(ctx) { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddInput("freq", PinDataType::kFloat, 440.0f);
    AddInput("amp", PinDataType::kFloat, 0.5f);
    AddInput("phase", PinDataType::kFloat, 0.0f);
    AddOutput("signal", PinDataType::kFloat, 0.0f);
  }

  ~SquareOscillatorNode() {}

  void Process(float time) override {
    if (inputs[0]->IsConnected()) {
      freq = GetInputValue<float>(0);
    }

    if (inputs[1]->IsConnected()) {
      amp = GetInputValue<float>(1);
    }

    float t = std::fmod(time,  1.0f / freq) * freq;
    float wave = amp * (t > 0.5f ? 1.0f : -1.0f);
    SetOutputValue<float>(0, wave);
  }
  
 private: 
  float freq = 440.0f;
  float amp = 0.5f;
};



struct SuperOscNode : public Node {
  enum class OscType {
    kSine, kSquare, kTriangle, kSaw
  };
  static inline const std::string DISPLAY_NAME = "Super wave";
  static inline const NodeType TYPE = NodeType::SUPER_OSC;

  SuperOscNode(const NodeParams& ctx) 
      : Node(ctx)
      , freq_knob("freq", 0.0f, 1600.0f) 
      , amp_knob("amp", 0.0f, 1.0f)
      
  { 
    type = TYPE;
    display_name = DISPLAY_NAME;
    
    GenLUT(kSampleRate * 4);

    AddInput("freq", PinDataType::kFloat, 440.0f);
    AddInput("amp", PinDataType::kFloat, 0.5f);
    AddInput("phase", PinDataType::kFloat, 0.0f);
    AddInput("time", PinDataType::kFloat, 0.0f);
    AddOutput("signal", PinDataType::kFloat, 0.0f);
  }

  ~SuperOscNode() {}
  
  void GenLUT(int num_samples) {
    phase_lookup.resize(num_samples);
    for (int i = 0; i < num_samples; ++i) {
      float ph = std::sin(2.0f * M_PIf * (static_cast<float>(i) / num_samples));
      phase_lookup[i] = ph;
    }
  }

  void Process(float time) override {
    /*
      phase_delta = N * f / Fs
      where:

      phase_delta is the number of LUT samples to increment
      freq is the desired output frequency
      Fs is the sample rate
      N is the size of the LUT
    */
    if (GetActiveVoice() != 0) {
      return;
    }
    
    if (inputs[0]->IsConnected()) {
      freq = GetInputValue<float>(0);
    } else {
      freq = freq_knob.GetValue();
    }

    if (inputs[1]->IsConnected()) {
      amp = GetInputValue<float>(1);
    } else {
      amp = amp_knob.GetValue();
    }
    
    int phase_delta = phase_lookup.size() * (freq / kSampleRate);
    phase_accum = (phase_accum + phase_delta) % phase_lookup.size();
    
    float wave = phase_lookup[phase_accum] * amp;
    // float phase = GetInputValue<float>(2);
    // float wave = amp * sin(time * 2.0 * M_PI * freq + phase);
    SetOutputValue<float>(0, wave);
  }
  
  void Draw() override {
    ImGui::PushItemWidth(100.0f);
    freq_knob.Draw();
    ImGui::SameLine();
    amp_knob.Draw();
    ImGui::PopItemWidth();
  }
  
  void Save(nlohmann::json& j) const override {
    freq_knob.Save(j["freq"]);
    amp_knob.Save(j["amp"]);
  }

  void Load(const nlohmann::json& j) override {
    if (j.find("freq") != j.end()) {
      freq_knob.Load(j["freq"]);
    }

    if (j.find("amp") != j.end()) {
      freq_knob.Load(j["amp"]);
    }
  }

 private:
  float freq;
  float amp;
  
  KnobFloat freq_knob;
  KnobFloat amp_knob;
  
  std::vector<float> phase_lookup;
  int phase_accum;
};