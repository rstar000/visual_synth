#pragma once

#include <array>
#include <cmath>

#include "imgui.h"
#include "node.h"
#include "node_types.h"
#include "ui/knob.h"
#include "util.h"

struct ADSRNode : public Node {
    static inline const std::string DISPLAY_NAME = "ADSR";
    static inline const NodeType TYPE = NodeType::ADSR;

    static constexpr float kMaxTime = 5.0f;

    ADSRNode(const NodeParams& ctx)
        : Node(ctx),
          a_knob("Attack", 0.0f, kMaxTime),
          d_knob("Decay", 0.0f, kMaxTime),
          s_knob("Sustain", 0.0f, 1.0f),
          r_knob("Release", 0.0f, kMaxTime) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        AddInput("ch", PinDataType::kChannel, Channel{});
        AddOutput("vel", PinDataType::kFloat, 0.0f);
    }

    ~ADSRNode() {}

    void Process(float time) override {
        auto in = GetInputValue<Channel>(0);

        float l_begin = time - in.begin;
        float l_end = time - in.end;

        float ad_len = a_knob.value + d_knob.value;

        if (time > in.end) {
            // Release
            if (l_begin < ad_len) {
                SetOutputValue<float>(0, CalcPressedValue(in.begin, time));
            } else {
                float t_release =
                    std::max(in.begin + ad_len, in.end);  // Start of release
                float release_frac = (time - t_release) / r_knob.value;
                // After atk + decay

                SetOutputValue<float>(
                    0, std::max(0.0f, (1.0f - release_frac) * s_knob.value));
            }
        } else {
            SetOutputValue<float>(0, CalcPressedValue(in.begin, time));
            // if (t_pressed < a_knob.value) {
            //   // Attack
            //   float attack_frac = t_pressed / a_knob.value;
            //   SetOutputValue<float>(0, attack_frac);
            // } else if (t_pressed < a_knob.value + d_knob.value) {
            //   // Decay
            //   float decay_frac = (t_pressed - a_knob.value) / d_knob.value;
            //   SetOutputValue<float>(0, decay_frac * s_knob.value + (1.0f -
            //   decay_frac));
            // } else {
            //   // Sustain
            //   SetOutputValue<float>(0, s_knob.value);
            // }
        }
    }

    float CalcPressedValue(float t_begin, float t_cur) {
        float t_pressed = t_cur - t_begin;
        if (t_pressed < a_knob.value) {
            // Attack
            float attack_frac = t_pressed / a_knob.value;
            return attack_frac;
        } else if (t_pressed < a_knob.value + d_knob.value) {
            // Decay
            float decay_frac = (t_pressed - a_knob.value) / d_knob.value;
            return decay_frac * s_knob.value + (1.0f - decay_frac);
        } else {
            // Sustain
            return s_knob.value;
        }
    }

    void Draw() override {
        a_knob.Draw();
        ImGui::SameLine();

        d_knob.Draw();
        ImGui::SameLine();

        s_knob.Draw();
        ImGui::SameLine();

        r_knob.Draw();
        ImGui::SameLine();
    }

    void Save(nlohmann::json& j) const override {
        a_knob.Save(j["attack"]);
        d_knob.Save(j["decay"]);
        s_knob.Save(j["sustain"]);
        r_knob.Save(j["release"]);
    }

    void Load(const nlohmann::json& j) override {
        if (j.find("attack") != j.end()) {
            a_knob.Load(j["attack"]);
            d_knob.Load(j["decay"]);
            s_knob.Load(j["sustain"]);
            r_knob.Load(j["release"]);
        }
    }

   private:
    KnobFloat a_knob;
    KnobFloat d_knob;
    KnobFloat s_knob;
    KnobFloat r_knob;
};

struct DX7EGNode : public Node {
    static inline const std::string DISPLAY_NAME = "DX7EG";
    static inline const NodeType TYPE = NodeType::DX7EG;

    static constexpr float kMaxValue = 100.0f;

    DX7EGNode(const NodeParams& ctx) : Node(ctx) {
        for (int i = 0; i < 4; ++i) {
            level[i] = std::make_unique<KnobFloat>("Level " + std::to_string(i), 0.0f, kMaxValue);
            rate[i] = std::make_unique<KnobFloat>("Rate " + std::to_string(i), 0.0f, kMaxValue);
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
        for (auto& knob : level) {
            level[i]->Draw();
            if (i < 3) {
                ImGui::SameLine();
            }
        }

        for (int i = 0; i < 4; ++i) {
            rate[i]->Draw();
            ImGui::SameLine();
        }
    }

    void Save(nlohmann::json& j) const override {}

    void Load(const nlohmann::json& j) override {}

   private:
    std::array<std::unique_ptr<KnobFloat>, 4> m_level;
    std::array<std::unique_ptr<KnobFloat>, 4> m_rate;
};
