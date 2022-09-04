#pragma once

#include <cmath>
#include <array>
#include <deque>
#include "node.h"
#include "node_types.h"

#include "imgui.h"


struct SliderNode : public Node {
  static inline const std::string DISPLAY_NAME = "Slider";
  static inline const NodeType TYPE = NodeType::SLIDER;

  SliderNode() {
    type = TYPE;
    display_name = DISPLAY_NAME;

    auto signal_out = std::make_shared<Output>("signal", PinDataType::kFloat, this, 0.0f);
    outputs = {signal_out};
    slider_label = GenLabel("slider", this);
    input_label = GenLabel("input", this);
  }

  ~SliderNode() {}

  void Process(float time) override {
    outputs[0]->SetValue<float>(signal);
  }
  
  void Draw() override {
    ImGui::PushItemWidth(100.0f);
    ImGui::SliderFloat(slider_label.c_str(), &signal, v_min_max[0], v_min_max[1], "%.2f", ImGuiSliderFlags_None);
    ImGui::InputFloat2(input_label.c_str(), v_min_max.data(), "%.2f", ImGuiInputTextFlags_None);
    ImGui::PopItemWidth();
  }

  void Save(nlohmann::json& j) const override {
    JsonSetValue(j, "v_min", v_min_max[0]);
    JsonSetValue(j, "v_max", v_min_max[1]);
    JsonSetValue(j, "signal", signal);
  }

  void Load(const nlohmann::json& j) override {
    JsonGetValue(j, "v_min", v_min_max[0]);
    JsonGetValue(j, "v_max", v_min_max[1]);
    JsonGetValue(j, "signal", signal);
  }

 protected:
  float signal;
  std::array<float, 2> v_min_max;
  
  // These labels are a hack to have a unique id for all input fields. 
  // Here we use pointers because at runtime no two objects can occupy one address.
  std::string slider_label;
  std::string input_label;
};

struct ConstantNode : public Node {
  static inline const std::string DISPLAY_NAME = "Constant";
  static inline const NodeType TYPE = NodeType::CONSTANT;

  ConstantNode() { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    auto signal_out = std::make_shared<Output>("signal", PinDataType::kFloat, this, 0.0f);
    outputs = {signal_out};
    input_label = GenLabel("input", this);
  }

  ~ConstantNode() {}

  void Process(float time) override {
    outputs[0]->SetValue<float>(signal);
  }
  
  void Draw() override {
    ImGui::PushItemWidth(100.0f);
    ImGui::InputFloat(input_label.c_str(), &signal, 0.0f, 0.0f, "%.2f", ImGuiInputTextFlags_None);
    ImGui::PopItemWidth();
  }

  void Save(nlohmann::json& j) const override {
    JsonSetValue(j, "signal", signal);
  }

  void Load(const nlohmann::json& j) override {
    JsonGetValue(j, "signal", signal);
  }

 protected:
  float signal;
  std::string input_label;
};

struct MixNode : public Node {
  static inline const std::string DISPLAY_NAME = "Mix";
  static inline const NodeType TYPE = NodeType::MIX;

  MixNode() { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    auto input_a = std::make_shared<Input>("input_a", PinDataType::kFloat, this, 0.0f);
    auto input_b = std::make_shared<Input>("input_b", PinDataType::kFloat, this, 0.0f);
    auto alpha = std::make_shared<Input>("alpha", PinDataType::kFloat, this, 0.0f);
    auto signal = std::make_shared<Output>("signal", PinDataType::kFloat, this, 0.0f);

    inputs = {input_a, input_b, alpha};
    outputs = {signal};

    slider_label = GenLabel("slider", this);
  }

  ~MixNode() {}

  void Process(float time) override {
    float alpha = inputs[2]->GetValue<float>();
    if (!inputs[2]->IsConnected()) {
      alpha = alpha_param;
    }

    float res = inputs[0]->GetValue<float>() * (1.0f - alpha) + 
                inputs[1]->GetValue<float>() * alpha;
    outputs[0]->SetValue<float>(res);
  }
  
  void Draw() override {
    ImGui::PushItemWidth(100.0f);
    ImGui::SliderFloat(slider_label.c_str(), &alpha_param, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_None);
    ImGui::PopItemWidth();
  }

  void Save(nlohmann::json& j) const override {
    JsonSetValue(j, "alpha", alpha_param);
  }

  void Load(const nlohmann::json& j) override {
    JsonGetValue(j, "alpha", alpha_param);
  }

 protected:
  float alpha_param;
  float signal;

  std::string slider_label;
};

struct AddNode : public Node {
  static inline const std::string DISPLAY_NAME = "Add";
  static inline const NodeType TYPE = NodeType::ADD;

  AddNode() { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    auto input_a = std::make_shared<Input>("a", PinDataType::kFloat, this, 0.0f);
    auto input_b = std::make_shared<Input>("b", PinDataType::kFloat, this, 0.0f);
    auto input_c = std::make_shared<Input>("c", PinDataType::kFloat, this, 0.0f);
    auto signal  = std::make_shared<Output>("signal", PinDataType::kFloat, this, 0.0f);

    inputs = {input_a, input_b, input_c};
    outputs = {signal};
  }

  ~AddNode() {}

  void Process(float time) override {
    float res = inputs[0]->GetValue<float>() + 
                inputs[1]->GetValue<float>() +
                inputs[2]->GetValue<float>();
    outputs[0]->SetValue<float>(res);
  }
};

struct MultiplyNode : public Node {
  static inline const std::string DISPLAY_NAME = "Multiply";
  static inline const NodeType TYPE = NodeType::MULTIPLY;

  MultiplyNode() { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    auto input_a = std::make_shared<Input>("a", PinDataType::kFloat, this, 1.0f);
    auto input_b = std::make_shared<Input>("b", PinDataType::kFloat, this, 1.0f);
    auto signal  = std::make_shared<Output>("signal", PinDataType::kFloat, this, 0.0f);

    inputs = {input_a, input_b};
    outputs = {signal};
  }

  ~MultiplyNode() {}

  void Process(float time) override {
    float res = inputs[0]->GetValue<float>() * 
                inputs[1]->GetValue<float>(); 
    outputs[0]->SetValue<float>(res);
  }
};

struct ClampNode : public Node {
  static inline const std::string DISPLAY_NAME = "Clamp";
  static inline const NodeType TYPE = NodeType::CLAMP;

  ClampNode() { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    inputs = {
      std::make_shared<Input>("x", PinDataType::kFloat, this, 1.0f),
      std::make_shared<Input>("v_min", PinDataType::kFloat, this, 1.0f),
      std::make_shared<Input>("v_max", PinDataType::kFloat, this, 1.0f)
    };
    outputs = {std::make_shared<Output>("signal", PinDataType::kFloat, this, 0.0f)};
  }

  ~ClampNode() {}

  void Process(float time) override {
    float x = inputs[0]->GetValue<float>();
    float v_min = inputs[1]->GetValue<float>();
    float v_max = inputs[2]->GetValue<float>();

    float res = std::clamp(x, v_min, v_max);
    outputs[0]->SetValue<float>(res);
  }
};

struct NegateNode : public Node {
  static inline const std::string DISPLAY_NAME = "Negate";
  static inline const NodeType TYPE = NodeType::NEGATE;

  NegateNode() { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    inputs = {std::make_shared<Input>("x", PinDataType::kFloat, this, 0.0f)};
    outputs = {std::make_shared<Output>("y", PinDataType::kFloat, this, 0.0f)};
  }

  ~NegateNode() {}

  void Process(float time) override {
    float x = inputs[0]->GetValue<float>();
    outputs[0]->SetValue<float>(-x);
  }
};

const int NUM_DEBUG_VALUES = 100;

struct DebugNode : public Node {
  static inline const std::string DISPLAY_NAME = "Debug";
  static inline const NodeType TYPE = NodeType::DEBUG;

  DebugNode() : values(NUM_DEBUG_VALUES) { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    inputs = {std::make_shared<Input>("x", PinDataType::kFloat, this, 0.0f)};
    
    plot_label = GenLabel("plot", this);
    min_max_label = GenLabel("mm", this);
    slider_label = GenLabel("slider", this);
  }

  ~DebugNode() {}

  void Process(float time) override {
    if (time < prev_time) {
      prev_time = time;
    }

    if (time - prev_time < resolution / 1000.0f) {
      return;
    }

    prev_time = time;
    float val = inputs[0]->GetValue<float>();
    values[cur_idx % NUM_DEBUG_VALUES] = val;
    ++cur_idx;
  }
  
  void Draw() override {
    auto data = std::vector<float>(values.begin(), values.end());
    ImGui::PushItemWidth(200.0f);
    ImGui::PlotLines(plot_label.c_str(), data.data(), data.size(), cur_idx, NULL, v_min_max[0], v_min_max[1], ImVec2(0, 80));
    ImGui::InputFloat2(min_max_label.c_str(), v_min_max.data(), "%.2f", ImGuiInputTextFlags_None);
    ImGui::SliderFloat(slider_label.c_str(), &resolution, 0.0f, 1000.0f, "%.2f", ImGuiSliderFlags_None);
    ImGui::PopItemWidth();
  }

  void Save(nlohmann::json& j) const override {
    JsonSetValue(j, "v_min_max", v_min_max);
    JsonSetValue(j, "resolution", resolution);
  }

  void Load(const nlohmann::json& j) override {
    JsonGetValue(j, "v_min_max", v_min_max);
    JsonGetValue(j, "resolution", resolution);
  }
  
 private:
  int cur_idx = 0;
  std::vector<float> values;
  std::array<float, 2> v_min_max;
  float resolution = 1.0f;
  
  float prev_time = 0.0f;
  
  std::string plot_label;
  std::string min_max_label;
  std::string slider_label;
};