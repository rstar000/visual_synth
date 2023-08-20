#pragma once

#include "node.h"
#include "output.h"
#include "GridUI/Widgets/Fader.hpp"
// #include "audio_thread.h"

struct AudioOutputNode : public Node {
    static inline const std::string DISPLAY_NAME = "Output";
    static inline const NodeType TYPE = NodeType::OUTPUT;

    struct ComponentIndices
    {
        uint32_t signalInput;
        uint32_t gateFader;
    };

    AudioOutputNode(const NodeParams& ctx) : Node(ctx), writer(ctx.writer) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(1, 2);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder(m_shape * GRID_STEP)
                .AddColumnsEx(3, {1, 2, 1})
                .GetIndex(&m_indices.signalInput, 0)
                .GetIndex(&m_indices.gateFader, 1)
                .Build());

        AddInput("signal", PinDataType::kFloat, 0.0f);
    }

    ~AudioOutputNode() {}

    void Process(float time) override {
        float wave = GetInputValue<float>(0) * volume;

        if (!all_voices && GetActiveVoice() != voice_idx) {
            return;
        }

        _last = wave;
        writer->Write(wave, GetActiveSample());
    }

    void Draw() override {
        float norm = (_last + 1.0f) / 2.0f;

        m_ctx.ui->DrawComponent(
            m_layout->GetComponent(m_indices.gateFader), 
            [this] (ImRect dst) {
                DrawFader("Volume", dst, volume, 0.0f, 1.0f, "%.2f");
            });
        // ImGui::PushItemWidth(100.0f);
        // ImGui::ProgressBar(norm, ImVec2(0.0f, 0.0f));
        // ImGui::SliderFloat(slider_label.c_str(), &volume, 0.0, 1.0, "%.2f",
        //                    ImGuiSliderFlags_None);
        // ImGui::Checkbox("All voices", &all_voices);
        // ImGui::InputInt(input_label.c_str(), &voice_idx, 1, 1);
        // ImGui::PopItemWidth();
    }

   private:
    float _last = 0.0f;
    int voice_idx = 0;
    float volume = 0.5f;
    bool all_voices = true;
    SampleWriter* writer;

    ComponentIndices m_indices;
};