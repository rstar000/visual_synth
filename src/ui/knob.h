#pragma once

#include "imgui.h"
#include "json.hpp"
#include "imgui_knobs.h"

#include "widgets.h"

struct KnobFloat : public Widget {
  KnobFloat(const std::string& vis_name, float v_min, float v_max)
      : v_min(v_min)
      , v_max(v_max)
      , value((v_min + v_max) / 2.0f) {
    s_label = GenLabel("knob", this, vis_name);
  }
  
  void Draw() {
    ImGuiKnobs::Knob(
      s_label.c_str(), &value, v_min, v_max, 0, "%.2f", 
      ImGuiKnobVariant_Wiper, 0);
  }

  void Save(nlohmann::json& j) const override {
    JsonSetValue(j, "value", value);
  }

  void Load(const nlohmann::json& j) override {
    JsonGetValue(j, "value", value, 0.0f);
  }
  
  float GetValue() const {
    return value;
  }

  const float v_min;
  const float v_max;
  float value;
 private: 
  std::string s_label;
};
