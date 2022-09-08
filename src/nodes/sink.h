#pragma once

#include "node.h"
#include "output.h"
// #include "audio_thread.h"


struct AudioOutputNode : public Node {
  static inline const std::string DISPLAY_NAME = "Audio Output";
  static inline const NodeType TYPE = NodeType::OUTPUT;

  AudioOutputNode(const NodeParams& ctx) 
      : Node(ctx)
      , writer(ctx.writer) { 
    type = TYPE;
    display_name = DISPLAY_NAME;
    
    AddInput("signal", PinDataType::kFloat, 0.0f);
    
    input_label = GenLabel("voice", this);
  }

  ~AudioOutputNode() {}

  void Process(float time) override {
    float wave = GetInputValue<float>(0);

    if (!all_voices && GetActiveVoice() != voice_idx) {
      return;
    }

    _last = wave;
    writer->Write(wave, GetActiveSample());
  }
  
  void Draw() override {
    float norm = (_last + 1.0f) / 2.0f;
    ImGui::PushItemWidth(100.0f);
    ImGui::ProgressBar(norm, ImVec2(0.0f, 0.0f));
    ImGui::Checkbox("All voices", &all_voices);
    ImGui::InputInt(input_label.c_str(), &voice_idx, 1, 1);
    ImGui::PopItemWidth();
  }

 private:
  float _last = 0.0f;
  int voice_idx = 0;
  bool all_voices = true;
  std::shared_ptr<SampleWriter> writer;
  
  std::string input_label;
};