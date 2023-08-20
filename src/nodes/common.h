#pragma once

#include <array>
#include <cmath>
#include <deque>

#include "imgui.h"
#include "node.h"
#include "node_types.h"
#include "GridUI/Widgets/Knob.hpp"

struct SliderNode : public Node {
    static inline const std::string DISPLAY_NAME = "Slider";
    static inline const NodeType TYPE = NodeType::SLIDER;

    SliderNode(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        AddOutput("signal", PinDataType::kFloat, 0.0f);
    }

    ~SliderNode() {}

    void Process(float time) override { SetOutputValue<float>(0, signal); }

    void Draw() override {
        ImGui::PushItemWidth(100.0f);
        ImGui::SliderFloat("Slider", &signal, v_min_max[0],
                           v_min_max[1], "%.2f", ImGuiSliderFlags_None);
        ImGui::InputFloat2("Input", v_min_max.data(), "%.2f",
                           ImGuiInputTextFlags_None);
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
};

struct ConstantNode : public Node {
    static inline const std::string DISPLAY_NAME = "Constant";
    static inline const NodeType TYPE = NodeType::CONSTANT;

    ConstantNode(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(1, 1);
        m_layout =
            std::make_unique<GridLayout>(
                GridLayoutBuilder(m_shape * GRID_STEP)
                    .AddColumnsEx(2, {2, 1})
                    .Build());

        AddOutput("signal", PinDataType::kFloat, 0.0f, 1);
        AddParam("signal", &signal);
    }

    ~ConstantNode() {}

    void Process(float time) override { SetOutputValue<float>(0, signal); }

    void Draw() override {
        m_ctx.ui->DrawComponent(m_layout->GetComponent(0), [this](ImRect dst) {
            float offset = ImGui::GetStyle().FramePadding.y + ImGui::GetTextLineHeight() * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(dst.Min.x, dst.GetCenter().y - offset));
            ImGui::PushItemWidth(dst.GetWidth());
            ImGui::InputFloat("##value", &signal, 0.0f, 0.0f, "%.2f",
                              ImGuiInputTextFlags_None);
            ImGui::PopItemWidth();
        });
    }

   protected:
    float signal;
};

struct MixNode : public Node {
    static inline const std::string DISPLAY_NAME = "Mix";
    static inline const NodeType TYPE = NodeType::MIX;

    struct ComponentIndices 
    {
        uint32_t inputA;
        uint32_t inputB;
        uint32_t inputAlpha;
        uint32_t outputSignal;
        uint32_t mixKnob;
    };

    MixNode(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(2, 1);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder(m_shape * GRID_STEP)
                .AddColumnsEx(3, {0.5f, 1.0f, 0.5f})
                .Push(0).AddRows(3)
                    .GetIndex(&m_indices.inputA, 0)
                    .GetIndex(&m_indices.inputB, 1)
                    .GetIndex(&m_indices.inputAlpha, 2)
                .Pop()
                .GetIndex(&m_indices.mixKnob, 1)
                .GetIndex(&m_indices.outputSignal, 2)
                .Build());

        AddInput("input_a", PinDataType::kFloat, 0.0f, m_indices.inputA);
        AddInput("input_b", PinDataType::kFloat, 0.0f, m_indices.inputB);
        AddInput("alpha", PinDataType::kFloat, 0.0f, m_indices.inputAlpha);
        AddOutput("signal", PinDataType::kFloat, 0.0f, m_indices.outputSignal);
        AddParam("alpha", &m_alpha);
    }

    ~MixNode() {}

    void Process(float time) override {
        float alpha = GetInputValue<float>(2);
        if (!inputs[2]->IsConnected()) {
            alpha = m_alpha;
        }

        float res = GetInputValue<float>(0) * (1.0f - alpha) +
                    GetInputValue<float>(1) * alpha;
        SetOutputValue<float>(0, res);
    }

    void Draw() override {
        m_ctx.ui->DrawComponent(m_layout->GetComponent(m_indices.mixKnob), [this](ImRect dst) {
            DrawKnob("Alpha", dst, signal, 0.0f, 1.0f, 0.5f, "%.2f", true);
        });
    }

   protected:
    float m_alpha;
    float signal;
    ComponentIndices m_indices;
};

struct AddNode : public Node {
    static inline const std::string DISPLAY_NAME = "Add";
    static inline const NodeType TYPE = NodeType::ADD;

    AddNode(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(1, 1);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder(m_shape * GRID_STEP)
                .AddColumns(2)
                .Push(0).AddRows(3)
                .Build());

        AddInput("a", PinDataType::kFloat, 0.0f, 0);
        AddInput("b", PinDataType::kFloat, 0.0f, 1);
        AddInput("c", PinDataType::kFloat, 0.0f, 2);
        AddOutput("signal", PinDataType::kFloat, 0.0f, 3);
    }

    ~AddNode() {}

    void Process(float time) override {
        float res = GetInputValue<float>(0) + GetInputValue<float>(1) +
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

        m_shape = ImVec2(1, 1);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder(m_shape * GRID_STEP)
                .AddColumns(2)
                .Push(0).AddRows(2)
                .Build());

        AddInput("a", PinDataType::kFloat, 0.0f, 0);
        AddInput("b", PinDataType::kFloat, 0.0f, 1);
        AddOutput("signal", PinDataType::kFloat, 0.0f, 2);
    }

    ~MultiplyNode() {}

    void Process(float time) override {
        float res = GetInputValue<float>(0) * GetInputValue<float>(1);
        SetOutputValue(0, res);
    }
};

struct ClampNode : public Node {
    static inline const std::string DISPLAY_NAME = "Clamp";
    static inline const NodeType TYPE = NodeType::CLAMP;

    ClampNode(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(1, 1);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder(m_shape * GRID_STEP)
                .AddColumns(2)
                .Push(0).AddRows(3)
                .Build());

        AddInput("x", PinDataType::kFloat, 1.0f, 0);
        AddInput("v_min", PinDataType::kFloat, 1.0f, 1);
        AddInput("v_max", PinDataType::kFloat, 1.0f, 2);
        AddOutput("signal", PinDataType::kFloat, 0.0f, 3);
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

        m_shape = ImVec2(1, 1);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder(m_shape * GRID_STEP)
                .AddColumns(2)
                .Build());

        AddInput("x", PinDataType::kFloat, 0.0f, 0);
        AddOutput("y", PinDataType::kFloat, 0.0f, 1);
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

    struct ComponentIndices
    {
        uint32_t inputX;
        uint32_t knobVMin;
        uint32_t knobVMax;
        uint32_t knobResolution;
        uint32_t display;
    };

    DebugNode(const NodeParams& ctx) : Node(ctx), values(NUM_DEBUG_VALUES) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(5, 4);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder(m_shape * GRID_STEP)
                .AddColumnsEx(2, {1, 4})
                .Push(0).AddRows(4)
                    .GetIndex(&m_indices.inputX, 0)
                    .GetIndex(&m_indices.knobVMin, 1)
                    .GetIndex(&m_indices.knobVMax, 2)
                    .GetIndex(&m_indices.knobResolution, 3)
                .Pop()
                .GetIndex(&m_indices.display, 1)
                .Build());

        AddInput("x", PinDataType::kFloat, 0.0f, m_indices.inputX);

        AddParam("v_min", &m_limits[0]);
        AddParam("v_max", &m_limits[1]);
        AddParam("resolution", &m_resolution);
    }

    ~DebugNode() {}

    void Process(float time) override {
        if (GetActiveVoice() != 0) {
            return;
        }

        if (time < prev_time) {
            prev_time = time;
        }

        if (time - prev_time < m_resolution / 1000.0f) {
            return;
        }

        prev_time = time;
        float val = GetInputValue<float>(0);
        values[cur_idx % NUM_DEBUG_VALUES] = val;
        ++cur_idx;
    }

    void Draw() override {
        m_ctx.ui->DrawComponent(m_layout->GetComponent(m_indices.knobVMin), [this](ImRect dst) {
            DrawKnob("Min", dst, m_limits[0], -10.0f, 10.0f, -1.0f, "%.2f", true);
        });

        m_ctx.ui->DrawComponent(m_layout->GetComponent(m_indices.knobVMax), [this](ImRect dst) {
            DrawKnob("Max", dst, m_limits[1], -10.0f, 10.0f, 1.0f, "%.2f", true);
        });

        m_ctx.ui->DrawComponent(m_layout->GetComponent(m_indices.knobResolution), [this](ImRect dst) {
            DrawKnob("Resolution", dst, m_resolution, 0.0f, 100.0f, 1.0f, "%.2f", true);
        });

        m_ctx.ui->DrawComponent(m_layout->GetComponent(m_indices.display), [this](ImRect dst) {
            ImGui::SetCursorScreenPos(dst.Min);
            ImGui::PlotLines("##Plot", values.data(), values.size(),
                            cur_idx, NULL, m_limits[0], m_limits[1],
                            dst.GetSize());
        });
    }

   private:
    int cur_idx = 0;
    std::vector<float> values;

    std::array<float, 2> m_limits = {-1.0f, 1.0f};
    float m_resolution = 1.0f;

    float prev_time = 0.0f;

    ComponentIndices m_indices;
};