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
        uint32_t signalInput, signalOutput, cutoffInput, cutoffKnob,
        resonanceInput, resonanceKnob, gainKnob, filterTypeToggle,
        doubleCheckbox;
    };

    struct IOIndices {
        uint32_t signalInput, signalOutput, cutoffInput, resonanceInput;
    };

    BiquadFilterNode(const NodeParams& ctx) : Node(ctx) {
        Setup(TYPE, DISPLAY_NAME);
        m_shape = ImVec2(4, 2);

        auto builder = GridLayoutBuilder();
        builder.AddColumnsEx(3, {0.5f, 3.0f, 0.5f})
        .Push(0)
            .AddRows(3)
            .GetIndexN({&_C.signalInput, &_C.cutoffInput, &_C.resonanceInput})
        .Pop()
            .GetIndex(&_C.signalOutput, 2)
        .Push(1)
            .AddRowsEx(2, {0.5f, 1.5f})
            .Push(0).AddColumnsEx(2, {3.0f, 1.0f})
                .GetIndex(&_C.filterTypeToggle, 0)
                .GetIndex(&_C.doubleCheckbox, 1)
            .Pop()
            .Push(1).AddColumns(3)
            .GetIndexN({ &_C.cutoffKnob, &_C.resonanceKnob, &_C.gainKnob});
        m_layout = std::make_unique<GridLayout>(builder.Build());

        _IO.signalInput = AddInput("signal", PinDataType::kFloat, 0.0f, _C.signalInput);
        _IO.cutoffInput = AddInput("cutoffShift", PinDataType::kFloat, 0.0f, _C.cutoffInput);
        _IO.signalOutput = AddOutput("signal", PinDataType::kFloat, 0.0f, _C.signalOutput);

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
        float cutoffShift = GetInputValue<float>(_IO.cutoffInput);
        float cutoffMult = std::pow(kFrequencyMultiplier, cutoffShift);
        float cutoff = m_cutoff * cutoffMult;

        filter.setBiquad(m_filterType, cutoff / m_ctx.playback->sampleRate, std::max(m_resonance, 0.1f), m_gain);
        float x = GetInputValue<float>(_IO.signalInput);
        x = filter.process(x);

        if (m_double) {
            auto& filterDouble = m_filterDouble[GetActiveVoice()];
            filterDouble.setBiquad(m_filterType, cutoff / m_ctx.playback->sampleRate, std::max(m_resonance, 0.1f), m_gain);
            x = filterDouble.process(x);
        }

        SetOutputValue(_IO.signalOutput, x);
    }

    void Draw() override {
        GridUI& ui = *m_ctx.ui;

        ui.BeginComponent(m_layout->GetComponent(_C.filterTypeToggle));
            DrawToggle(ui, "Filter type", &m_filterType, m_filterTypeToggleParams);
        ui.EndComponent();

        DrawCheckboxEx(ui, GetComponent(_C.doubleCheckbox), "Double", &m_double);

        DrawKnobEx(ui, GetComponent(_C.cutoffKnob), "Cutoff", &m_cutoff, KnobParams<float>{
            .minValue = 0.0f, .maxValue = 10000.0f, .defaultValue = 1000.0f, .format = "%.2f" 
        });

        DrawKnobEx(ui, GetComponent(_C.resonanceKnob), "Resonance", &m_resonance, KnobParams<float>{
            .minValue = 0.1f, .maxValue = 20.0f, .defaultValue = 0.7f, .format = "%.2f" 
        });

        DrawKnobEx(ui, GetComponent(_C.gainKnob), "Gain", &m_gain, KnobParams<float>{
            .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 0.0f, .format = "%.2f" 
        });
    }

private:
    ComponentIndices _C{};
    IOIndices _IO{};
    uint32_t m_filterType = 0;
    float m_cutoff = 100.0f;
    float m_resonance = 0.7f;
    float m_gain = 0.0f;

    bool m_double = false;

    std::vector<Biquad> m_filter{};
    std::vector<Biquad> m_filterDouble{};
    ToggleParams m_filterTypeToggleParams{};
};
