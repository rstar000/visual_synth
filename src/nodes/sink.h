#pragma once

#include "node.h"
#include "audio_thread.h"


struct AudioOutputNode : public Node {
  static inline const std::string DISPLAY_NAME = "Audio Output";
  static inline const NodeType TYPE = NodeType::OUTPUT;

  AudioOutputNode(std::shared_ptr<AudioOutput> output) : output(output) { 
    type = TYPE;
    display_name = DISPLAY_NAME;

    auto signal = std::make_shared<Input>("signal", PinDataType::kFloat, this, 0.0f);
    inputs = {signal};
  }

  ~AudioOutputNode() {}

  void Process(float time) override {
    float wave = inputs[0]->GetValue<float>();
    output->wave = wave;
  }
  
  void Draw() override {
    float norm = (output->wave + 1.0f) / 2.0f;
    ImGui::PushItemWidth(100.0f);
    ImGui::ProgressBar(norm, ImVec2(0.0f, 0.0f));
    ImGui::PopItemWidth();
  }

 private:
  std::shared_ptr<AudioOutput> output;
};