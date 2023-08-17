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

  SliderNode(const NodeParams& ctx) : Node(ctx) {
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddOutput("signal", PinDataType::kFloat, 0.0f);
    slider_label = GenLabel("slider", this);
    input_label = GenLabel("input", this);
  }

  ~SliderNode() {}

  void Process(float time) override {
    SetOutputValue<float>(0, signal);
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

  ConstantNode(const NodeParams& ctx) : Node(ctx) { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddOutput("signal", PinDataType::kFloat, 0.0f);
    input_label = GenLabel("input", this);
  }

  ~ConstantNode() {}

  void Process(float time) override {
    SetOutputValue<float>(0, signal);
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

  MixNode(const NodeParams& ctx) : Node(ctx) { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddInput("input_a", PinDataType::kFloat, 0.0f);
    AddInput("input_b", PinDataType::kFloat, 0.0f);
    AddInput("alpha", PinDataType::kFloat, 0.0f);
    AddOutput("signal", PinDataType::kFloat, 0.0f);

    slider_label = GenLabel("slider", this);
  }

  ~MixNode() {}

  void Process(float time) override {
    float alpha = GetInputValue<float>(2);
    if (!inputs[2]->IsConnected()) {
      alpha = alpha_param;
    }

    float res = GetInputValue<float>(0) * (1.0f - alpha) + 
                GetInputValue<float>(1) * alpha;
    SetOutputValue<float>(0, res);
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

  AddNode(const NodeParams& ctx) : Node(ctx) { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddInput("a", PinDataType::kFloat, 0.0f);
    AddInput("b", PinDataType::kFloat, 0.0f);
    AddInput("c", PinDataType::kFloat, 0.0f);
    AddOutput("signal", PinDataType::kFloat, 0.0f);
  }

  ~AddNode() {}

  void Process(float time) override {
    float res = GetInputValue<float>(0) + 
                GetInputValue<float>(1) +
                GetInputValue<float>(2);
    SetOutputValue<float>(0, res);
  }
};

struct MultiplyNode : public Node {
  static inline const std::string DISPLAY_NAME = "Multiply";
  static inline const NodeType TYPE = NodeType::MULTIPLY;

  MultiplyNode(const NodeParams& ctx) : Node(ctx) { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddInput("a", PinDataType::kFloat, 0.0f);
    AddInput("b", PinDataType::kFloat, 0.0f);
    AddOutput("signal", PinDataType::kFloat, 0.0f);
  }

  ~MultiplyNode() {}

  void Process(float time) override {
    float res = GetInputValue<float>(0) * 
                GetInputValue<float>(1); 
    SetOutputValue(0, res);
  }
};

struct ClampNode : public Node {
  static inline const std::string DISPLAY_NAME = "Clamp";
  static inline const NodeType TYPE = NodeType::CLAMP;

  ClampNode(const NodeParams& ctx) : Node(ctx) { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddInput("x", PinDataType::kFloat, 1.0f);
    AddInput("v_min", PinDataType::kFloat, 1.0f);
    AddInput("v_max", PinDataType::kFloat, 1.0f);
    AddOutput("signal", PinDataType::kFloat, 0.0f);
  }

  ~ClampNode() {}

  void Process(float time) override {
    float x = GetInputValue<float>(0);
    float v_min = GetInputValue<float>(1);
    float v_max = GetInputValue<float>(2);

    float res = std::clamp(x, v_min, v_max);
    SetOutputValue<float>(0, res);
  }
};

struct NegateNode : public Node {
  static inline const std::string DISPLAY_NAME = "Negate";
  static inline const NodeType TYPE = NodeType::NEGATE;

  NegateNode(const NodeParams& ctx) : Node(ctx) {
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddInput("x", PinDataType::kFloat, 0.0f);
    AddOutput("y", PinDataType::kFloat, 0.0f);
  }

  ~NegateNode() {}

  void Process(float time) override {
    SetOutputValue<float>(0, -GetInputValue<float>(0));
  }
};

const int NUM_DEBUG_VALUES = 100;

struct DebugNode : public Node {
  static inline const std::string DISPLAY_NAME = "Debug";
  static inline const NodeType TYPE = NodeType::DEBUG;

  DebugNode(const NodeParams& ctx) : Node(ctx), values(NUM_DEBUG_VALUES) { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    AddInput("x", PinDataType::kFloat, 0.0f);
    
    plot_label = GenLabel("plot", this);
    min_max_label = GenLabel("mm", this);
    slider_label = GenLabel("slider", this);
  }

  ~DebugNode() {}

  void Process(float time) override {
    if (GetActiveVoice() != 0) {
      return;
    }

    if (time < prev_time) {
      prev_time = time;
    }

    if (time - prev_time < resolution / 1000.0f) {
      return;
    }

    prev_time = time;
    float val = GetInputValue<float>(0);
    values[cur_idx % NUM_DEBUG_VALUES] = val;
    ++cur_idx;
  }
  
  void Draw() override {
    ImGui::PushItemWidth(200.0f);
    ImGui::PlotLines(plot_label.c_str(), values.data(), values.size(), cur_idx, NULL, v_min_max[0], v_min_max[1], ImVec2(0, 80));
    ImGui::InputFloat2(min_max_label.c_str(), v_min_max.data(), "%.2f", ImGuiInputTextFlags_None);
    ImGui::SliderFloat(slider_label.c_str(), &resolution, 0.0f, 1000.0f, "%.2f", ImGuiSliderFlags_None);
    ImGui::Text("%.2f", values.back());
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