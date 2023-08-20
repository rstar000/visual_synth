#pragma once

#include <cmath>
#include <array>

#include "imgui.h"
#include "node.h"
#include "node_types.h"
#include "util.h"

#include "GridUI/Widgets/Fader.hpp"

// Simple sequencer with single note
struct MonoSequencer : public Node {
    static inline const std::string DISPLAY_NAME = "Mono sequencer";
    static inline const NodeType TYPE = NodeType::MONO_SEQUENCER;

    static constexpr int NUM_BEATS = 16;

    struct ComponentIndices
    {
        std::array<uint32_t, NUM_BEATS> volumeFaders;
        std::array<uint32_t, NUM_BEATS> durationFaders;
        uint32_t bpmInput;
        uint32_t channelOutput;
    };

    MonoSequencer(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(9, 2);

        auto builder = GridLayoutBuilder(m_shape * GRID_STEP);
        builder.AddColumnsEx(3, {0.5f, 8.0f, 0.5f});
        builder.GetIndex(&m_indices.bpmInput, 0);
        builder.GetIndex(&m_indices.channelOutput, 2);
        builder.Push(1).MakeRectGrid(NUM_BEATS, 2);

        auto GetIndexXY = [&] (uint32_t x, uint32_t y, uint32_t* idx) {
            builder.Push(y);
            builder.GetIndex(idx, x);
            builder.Pop();
        };

        for (uint32_t x = 0; x < NUM_BEATS; ++x) {
            GetIndexXY(x, 0, &m_indices.volumeFaders[x]);
            GetIndexXY(x, 1, &m_indices.durationFaders[x]);
        }

        m_layout = std::make_unique<GridLayout>(builder.Build());

        AddInput("bpm", PinDataType::kFloat, 100.0f, m_indices.bpmInput);
        AddOutput("ch", PinDataType::kChannel, Channel{}, m_indices.channelOutput);
        
        for (uint32_t beatIdx = 0; beatIdx < NUM_BEATS; ++beatIdx) {
            AddParam(string_format("beat_%u_volume", beatIdx), &m_volume.at(beatIdx));
            AddParam(string_format("beat_%u_duration", beatIdx), &m_duration.at(beatIdx));
        }

        std::fill(m_volume.begin(), m_volume.end(), 1.0f);
        std::fill(m_duration.begin(), m_duration.end(), 0.05f);
    }

    ~MonoSequencer() {}

    void Process(float time) override {
        float bpm = GetInputValue<float>(0);
        float bps = bpm / 60.0f;
        uint32_t numVoices = NumVoices();
        uint32_t activeVoice = GetActiveVoice();

        uint32_t beatIdx = static_cast<uint32_t>(bpm * (time / 60.0f)) % NUM_BEATS;
        uint32_t targetVoice = beatIdx % numVoices;
        if (targetVoice != activeVoice) {
            return;
        }

        m_beatIdx = beatIdx;

        float beatInt = 0.0f;
        float beatFrac = std::modf(time * bps, &beatInt);

        float t_begin = time - beatFrac / bps;
        float t_end = t_begin + m_duration[beatIdx];

        Channel ch{
            .note = Note(4, 0),
            .begin = t_begin,
            .end = t_end,
            .velocity = m_volume[beatIdx]
        };

        SetOutputValue<Channel>(0, ch);
    }

    void Draw() override {
        for (uint32_t x = 0; x < NUM_BEATS; ++x) {
            m_ctx.ui->DrawComponent(m_layout->GetComponent(m_indices.volumeFaders[x]), [this, &x] (ImRect dst) {
                ImGui::PushID(x);
                if (m_beatIdx == x) {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(200, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor(255, 0, 0));
                    DrawFaderRect("##volume", dst, m_volume[x], 0.0f, 1.0f, "%.2f");
                    ImGui::PopStyleColor(2);
                } else {
                    DrawFaderRect("##volume", dst, m_volume[x], 0.0f, 1.0f, "%.2f");
                }
                ImGui::PopID();
            });

            m_ctx.ui->DrawComponent(m_layout->GetComponent(m_indices.durationFaders[x]), [this, &x] (ImRect dst) {
                ImGui::PushID(x);
                DrawFaderRect("##duration", dst, m_duration[x], 0.0f, 1.0f, "%.2f");
                ImGui::PopID();
            });
        }
    }

private:
    ComponentIndices m_indices;
    uint32_t m_beatIdx = 0;
    std::array<float, NUM_BEATS> m_volume;
    std::array<float, NUM_BEATS> m_duration;
};