#pragma once
#include "imgui.h"
#include "json.hpp"

struct Widget {
  virtual void Save(nlohmann::json& j) const {}

  virtual void Load(const nlohmann::json& j) {}
  
  virtual void Draw() {}
};

struct Slider : public Widget {
  Slider(const std::string& name, int v_min, int v_max) 
      : name(name)
      , v_min(v_min)
      , v_max(v_max) 
      , value((v_min + v_max) / 2.0f)
  {
    s_label = GenLabel(name, &s_label);
  }

  void Draw() {
    ImGui::SliderFloat(s_label.c_str(), &value, 0.0f, 1000.0f, "%.2f", ImGuiSliderFlags_None);
  }

  void Save(nlohmann::json& j) const override {
    JsonSetValue(j, "value", value);
  }

  void Load(const nlohmann::json& j) override {
    JsonGetValue(j, "value", value);
  }

  float v_min;
  float v_max; 

  float value;  

 private: 
  std::string name;
  std::string s_label;
};