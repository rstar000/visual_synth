#pragma once

#include "node.h"
#include "output.h"

#include "GridUI/Widgets/Knob.hpp"
#include "GridUI/Widgets/Toggle.hpp"
#include "GridUI/Widgets/Checkbox.hpp"

#include "Oscillators/Common.hpp"
#include "Oscillators/Waveform.hpp"


struct LFONode : public Node
{
    enum class OscType : uint32_t {
        kSine = 0,
        kSquare,
        kTriangle,
        kSaw,
    };

    static inline const std::string DISPLAY_NAME = "LFO";
    static inline const NodeType TYPE = NodeType::LFO;

    static constexpr size_t NUM_WAVES = 4;

    struct ComponentIndices
    {
        uint32_t ampInput;
        uint32_t freqInput;
        uint32_t channelInput;
        uint32_t signalOutput;
        uint32_t freqKnob;
        uint32_t ampKnob;
        uint32_t phaseKnob;
        uint32_t retriggerToggle;
        uint32_t useBPMToggle;
        uint32_t noteDropdown;
        uint32_t dottedNoteToggle;
        uint32_t waveSelect;
    };

    LFONode(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(5, 5);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder()
                .AddColumnsEx(3, {0.5, 4, 0.5})
                .Push(0)
                    .AddRows(3)
                    .GetIndex(&m_indices.freqInput, 0)
                    .GetIndex(&m_indices.ampInput, 1)
                    .GetIndex(&m_indices.channelInput, 2)
                .Pop()
                .Push(1)
                    .AddRows(4)
                    .GetIndex(&m_indices.waveSelect, 0)
                    .Push(1)
                        .AddColumns(3)
                        .GetIndex(&m_indices.freqKnob, 0)
                        .GetIndex(&m_indices.ampKnob, 1)
                        .GetIndex(&m_indices.phaseKnob, 2)
                    .Pop()
                    .Push(2)
                        .AddColumns(4)
                        .GetIndex(&m_indices.retriggerToggle, 0)
                        .GetIndex(&m_indices.useBPMToggle, 1)
                        .GetIndex(&m_indices.dottedNoteToggle, 2)
                        .GetIndex(&m_indices.noteDropdown, 3)
                    .Pop()
                .Pop()
                .GetIndex(&m_indices.signalOutput, 2)
                .Build());
        AddInput("freq", PinDataType::kFloat, 0.0f, m_indices.freqInput);
        AddInput("amp", PinDataType::kFloat, 0.0f, m_indices.ampInput);
        AddInput("channel", PinDataType::kChannel, Channel{}, m_indices.channelInput);
        AddOutput("signal", PinDataType::kFloat, 0.0f, m_indices.signalOutput);

        AddParam("freq", &m_freq);
        AddParam("amp", &m_amp);
        AddParam("phase", &m_phase);
        AddParam("retrigger", &m_retrigger);
        AddParam("useBPM", &m_useBPM);
        AddParam("dotted_note", &m_dottedNote);
        AddParam("wave_index", &m_waveIndex);

        constexpr uint32_t wavetableNumSamples = kSampleRate / 2;

        ArrayGet(m_waves, OscType::kSine) =
            std::make_unique<WavetableOscillator>(wavetableNumSamples, GenWaveSine);
        ArrayGet(m_waves, OscType::kSquare) =
            std::make_unique<WavetableOscillator>(wavetableNumSamples, GenWaveSquare);
        ArrayGet(m_waves, OscType::kTriangle) =
            std::make_unique<WavetableOscillator>(wavetableNumSamples, GenWaveTri);
        ArrayGet(m_waves, OscType::kSaw) =
            std::make_unique<WavetableOscillator>(wavetableNumSamples, GenWaveSaw);
        m_phaseAccum.resize(NumVoices());

    }

    ~LFONode() {}

    void Process(float time) override {
        float freq = m_freq;
        float amp = m_amp;

        if (inputs[0]->IsConnected()) {
            freq = GetInputValue<float>(0);
        }

        if (inputs[1]->IsConnected()) {
            amp = m_amp * GetInputValue<float>(1);
        }

        // Early return for quiet channels
        if (amp < 0.01) {
            SetOutputValue<float>(0, 0.0f);
            return;
        }

        int voiceIdx = GetActiveVoice();

        if (m_retrigger) {
            const auto& ch = GetInputValue<Channel>(2);
            if (!ch.active) {
                m_phaseAccum[voiceIdx] = 0.0f;
            }
        }

        auto& osc = m_waves[m_waveIndex];

        float wave = GetWaveValue(*osc, voiceIdx, freq, amp);

        SetOutputValue<float>(0, wave);
    }

    float GetWaveValue(WavetableOscillator& osc, uint32_t voiceIdx,
                       float freq, float amp) {
        float phaseDelta = (freq / kSampleRate) * 2.0f * M_PIf;
        float& accum = m_phaseAccum[voiceIdx];
        accum = std::fmod(accum + phaseDelta, 2.0f * M_PIf);
        return osc.GetWave(accum) * amp;
    }

    void Draw() override {
        GridUI& ui = *m_ctx.ui;
        ui.BeginComponent(m_layout->GetComponent(m_indices.freqKnob));
            DrawKnob(ui, "Freq", &m_freq, KnobParams<float>{
                .minValue = 0.0f, .maxValue = 10.0f, .defaultValue = 1.0f, .format = "%.2f" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.ampKnob));
            DrawKnob(ui, "Amp", &m_amp, KnobParams<float>{
                .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 0.5f, .format = "%.2f" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.phaseKnob));
            DrawKnob(ui, "Phase", &m_phase, KnobParams<float>{
                .minValue = 0.0f, .maxValue = 3.14f, .defaultValue = 0.5f, .format = "%.2f" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.retriggerToggle));
            DrawCheckbox(ui, "Retrigger", m_retrigger);
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.useBPMToggle));
            DrawCheckbox(ui, "Use BPM", m_useBPM);
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.dottedNoteToggle));
            DrawCheckbox(ui, "Dotted", m_dottedNote);
        ui.EndComponent();
    }

    std::array<std::unique_ptr<WavetableOscillator>, NUM_WAVES> m_waves;
    std::vector<float> m_phaseAccum;
    ComponentIndices m_indices;
    uint32_t m_waveIndex = 0u;

    float m_freq = 1.0f;
    float m_amp = 0.5f;
    float m_phase = 0.5f;

    bool m_retrigger = false;
    bool m_useBPM = false;
    bool m_dottedNote = false;
};
