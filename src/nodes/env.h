#pragma once

#include <array>
#include <cmath>

#include "imgui.h"
#include "node.h"
#include "node_types.h"
#include "util.h"

#include "GridUI/Widgets/Knob.hpp"

struct ADSRNode : public Node {
    static inline const std::string DISPLAY_NAME = "ADSR";
    static inline const NodeType TYPE = NodeType::ADSR;

    static constexpr float kMaxTime = 5.0f;

    struct ComponentIndices {
        uint32_t channelInput;
        uint32_t velocityOutput;
        uint32_t attackKnob;
        uint32_t decayKnob;
        uint32_t sustainKnob;
        uint32_t releaseKnob;
    };

    ADSRNode(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(5, 1);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder()
                .AddColumnsEx(6, {0.5f, 1, 1, 1, 1, 0.5f})
                .GetIndex(&m_indices.channelInput, 0)
                .GetIndex(&m_indices.attackKnob, 1)
                .GetIndex(&m_indices.decayKnob, 2)
                .GetIndex(&m_indices.sustainKnob, 3)
                .GetIndex(&m_indices.releaseKnob, 4)
                .GetIndex(&m_indices.velocityOutput, 5)
                .Build());

        AddInput("ch", PinDataType::kChannel, Channel{},
                 m_indices.channelInput);
        AddOutput("vel", PinDataType::kFloat, 0.0f, m_indices.velocityOutput);

        AddParam("attack", &m_attack);
        AddParam("decay", &m_decay);
        AddParam("sustain", &m_sustain);
        AddParam("release", &m_release);
    }

    ~ADSRNode() {}

    void Process(float time) override {
        auto in = GetInputValue<Channel>(0);
        float res = CalcADSR(in, time);
        SetOutputValue<float>(0, res * in.velocity);
    }

    float CalcADSR(const Channel& channel, const float time) {
        float l_begin = time - channel.begin;
        /* float l_end = time - channel.end; */
        float ad_len = m_attack + m_decay;

        // Not pressed
        if (time < channel.begin || !channel.active) {
            return 0.0f;
        } else if (time > channel.end) {
            // Release
            if (l_begin < ad_len) {
                return CalcPressedValue(channel.begin, time);
            } else {
                // Start of release
                float t_release = std::max(channel.begin + ad_len, channel.end);
                float release_frac = 0.0f;
                if (m_release > 0.01f) {
                    release_frac =
                        1.0f - std::min(((time - t_release) / m_release), 1.0f);
                }

                // After atk + decay
                return release_frac * m_sustain;
            }
        } else {
            // Attack - Decay - Sustain
            return CalcPressedValue(channel.begin, time);
        }
    }

    float CalcPressedValue(float t_begin, float t_cur) {
        float t_pressed = t_cur - t_begin;
        if (t_pressed < m_attack) {
            // Attack
            float attack_frac = t_pressed / m_attack;
            return attack_frac;
        } else if (t_pressed < m_attack + m_decay) {
            // Decay
            float decay_frac = (t_pressed - m_attack) / m_decay;
            return decay_frac * m_sustain + (1.0f - decay_frac);
        } else {
            // Sustain
            return m_sustain;
        }
    }

    void Draw() override {
        m_ctx.ui->DrawComponent(m_layout->GetComponent(m_indices.attackKnob),
                                [this](ImRect dst) {
                                    DrawKnob("Attack", dst, m_attack, 0.0f,
                                             2.0f, 0.1f, "%.2f", true);
                                });

        m_ctx.ui->DrawComponent(
            m_layout->GetComponent(m_indices.decayKnob), [this](ImRect dst) {
                DrawKnob("Decay", dst, m_decay, 0.0f, 2.0f, 0.1f, "%.2f", true);
            });

        m_ctx.ui->DrawComponent(m_layout->GetComponent(m_indices.sustainKnob),
                                [this](ImRect dst) {
                                    DrawKnob("Sustain", dst, m_sustain, 0.0f,
                                             1.0f, 0.5f, "%.2f", true);
                                });

        m_ctx.ui->DrawComponent(m_layout->GetComponent(m_indices.releaseKnob),
                                [this](ImRect dst) {
                                    DrawKnob("Release", dst, m_release, 0.0f,
                                             2.0f, 0.1f, "%.2f", true);
                                });
    }

   private:
    float m_attack = 0.1f;
    float m_decay = 1.0f;
    float m_sustain = 0.5f;
    float m_release = 0.2f;

    ComponentIndices m_indices;
};

/*
struct DX7EGNode : public Node {
    static inline const std::string DISPLAY_NAME = "DX7EG";
    static inline const NodeType TYPE = NodeType::DX7EG;

    static constexpr float kMaxValue = 100.0f;

    DX7EGNode(const NodeParams& ctx) : Node(ctx) {
        for (int i = 0; i < 4; ++i) {
            m_level[i] = std::make_unique<KnobFloat>("Level " +
std::to_string(i), 0.0f, kMaxValue); m_rate[i] =
std::make_unique<KnobFloat>("Rate " + std::to_string(i), 0.0f, kMaxValue);
        }
        type = TYPE;
        display_name = DISPLAY_NAME;

        AddInput("ch", PinDataType::kChannel, Channel{});
        AddOutput("vel", PinDataType::kFloat, 0.0f);
    }

    ~DX7EGNode() {}

    void Process(float time) override {
        auto in = GetInputValue<Channel>(0);

        float l_begin = time - in.begin;
        float l_end = time - in.end;

        // float ad_len = a_knob.value + d_knob.value;

        // if (time > in.end) {
        //     // Release
        //     if (l_begin < ad_len) {
        //         SetOutputValue<float>(0, CalcPressedValue(in.begin, time));
        //     } else {
        //         float t_release =
        //             std::max(in.begin + ad_len, in.end);  // Start of release
        //         float release_frac = (time - t_release) / r_knob.value;
        //         // After atk + decay

        //         SetOutputValue<float>(
        //             0, std::max(0.0f, (1.0f - release_frac) * s_knob.value));
        //     }
        // } else {
        //     SetOutputValue<float>(0, CalcPressedValue(in.begin, time));
        // }
    }

    void Draw() override {
        for (int i = 0; i < 4; ++i) {
            m_level[i]->Draw();
            if (i < 3) {
                ImGui::SameLine();
            }
        }

        for (int i = 0; i < 4; ++i) {
            m_rate[i]->Draw();
            ImGui::SameLine();
        }
    }

    void Save(nlohmann::json& j) const override {}

    void Load(const nlohmann::json& j) override {}

   private:
    std::array<std::unique_ptr<KnobFloat>, 4> m_level;
    std::array<std::unique_ptr<KnobFloat>, 4> m_rate;
};
*/
