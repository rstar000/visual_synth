#pragma once

#include "Filters/BiQuad.hpp"
#include "GridUI/GridLayout.hpp"
#include "GridUI/Widgets/Toggle.hpp"
#include "GridUI/Widgets/Knob.hpp"
#include "GridUI/Widgets/Checkbox.hpp"
#include "node.h"
#include "note.h"

struct BiquadFilterNode : public Node {
    static inline const std::string DISPLAY_NAME = "Biquad filter";
    static inline const NodeType TYPE = NodeType::BIQUAD_FILTER;

    static constexpr uint32_t NUM_BEATS = 16;
    static constexpr uint32_t MAX_NUM_NOTES = 8;

    struct ComponentIndices
    {
        uint32_t signalInput;
        uint32_t signalOutput;
        uint32_t cutoffInput;
        uint32_t cutoffKnob;
        uint32_t resonanceInput;
        uint32_t resonanceKnob;
        uint32_t gainKnob;
        uint32_t filterTypeToggle;
        uint32_t doubleCheckbox;
    };

    BiquadFilterNode(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(4, 2);

        auto builder = GridLayoutBuilder();
        builder.AddColumnsEx(3, {0.5f, 3.0f, 0.5f});
        builder.Push(0).AddRows(3);
            builder.GetIndex(&m_indices.signalInput, 0);
            builder.GetIndex(&m_indices.cutoffInput, 1);
            builder.GetIndex(&m_indices.resonanceInput, 2);
        builder.Pop();
            builder.GetIndex(&m_indices.signalOutput, 2);
        builder.Push(1)
            .AddRowsEx(2, {0.5f, 1.5f})
            .Push(0).AddColumnsEx(2, {3.0f, 1.0f})
                .GetIndex(&m_indices.filterTypeToggle, 0)
                .GetIndex(&m_indices.doubleCheckbox, 1)
            .Pop()
            .Push(1).AddColumns(3)
                .GetIndex(&m_indices.cutoffKnob, 0)
                .GetIndex(&m_indices.resonanceKnob, 1)
                .GetIndex(&m_indices.gainKnob, 2);
        m_layout = std::make_unique<GridLayout>(builder.Build());

        AddInput("signal", PinDataType::kFloat, 0.0f, m_indices.signalInput);
        AddInput("cutoffShift", PinDataType::kFloat, 0.0f, m_indices.cutoffInput);
        AddOutput("signal", PinDataType::kFloat, 0.0f, m_indices.signalOutput);

        AddParam("filter_type", &m_filterType);
        AddParam("cutoff", &m_cutoff);
        AddParam("resonance", &m_resonance);
        AddParam("gain", &m_gain);

        m_filterTypeToggleParams = {
            .numOptions = 5,
            .tooltips = {"LowPass", "HighPass", "BandPass", "Notch", "Peak"},
            .height = 20.0f,
            .sideMargin = 5.0f
        };
        m_filter.resize(NumVoices());
        m_filterDouble.resize(NumVoices());
    }

    ~BiquadFilterNode() {}

    void Preprocess(float time) override {}

    void Process(float time) override {
        auto& filter = m_filter[GetActiveVoice()];
        float cutoffShift = GetInputValue<float>(1);
        float cutoffMult = std::pow(kFrequencyMultiplier, cutoffShift);
        float cutoff = m_cutoff * cutoffMult;

        filter.setBiquad(m_filterType, cutoff / m_ctx.playback->sampleRate, std::max(m_resonance, 0.1f), m_gain);
        float x = GetInputValue<float>(0);
        x = filter.process(x);

        if (m_double) {
            auto& filterDouble = m_filterDouble[GetActiveVoice()];
            filterDouble.setBiquad(m_filterType, cutoff / m_ctx.playback->sampleRate, std::max(m_resonance, 0.1f), m_gain);
            x = filterDouble.process(x);
        }

        SetOutputValue(0, x);
    }

    void Draw() override {
        GridUI& ui = *m_ctx.ui;

        ui.BeginComponent(m_layout->GetComponent(m_indices.filterTypeToggle));
            DrawToggle(ui, "Filter type", &m_filterType, m_filterTypeToggleParams);
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.doubleCheckbox));
            DrawCheckbox(ui, "Double", m_double);
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.cutoffKnob));
            DrawKnob(ui, "Cutoff", &m_cutoff, KnobParams<float>{
                .minValue = 0.0f, .maxValue = 10000.0f, .defaultValue = 1000.0f, .format = "%.2f" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.resonanceKnob));
            DrawKnob(ui, "Resonance", &m_resonance, KnobParams<float>{
                .minValue = 0.1f, .maxValue = 20.0f, .defaultValue = 0.7f, .format = "%.2f" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.gainKnob));
            DrawKnob(ui, "Gain", &m_gain, KnobParams<float>{
                .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 0.0f, .format = "%.2f" 
            });
        ui.EndComponent();
    }

private:
    ComponentIndices m_indices{};
    uint32_t m_filterType = 0;
    float m_cutoff = 100.0f;
    float m_resonance = 0.7f;
    float m_gain = 0.0f;

    bool m_double = false;

    std::vector<Biquad> m_filter{};
    std::vector<Biquad> m_filterDouble{};
    ToggleParams m_filterTypeToggleParams{};
};
