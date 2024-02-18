#pragma once

#include "GridUI/GridLayout.hpp"
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
        uint32_t ampInput, freqInput, channelInput, signalOutput, freqKnob, 
        ampKnob, phaseKnob, retriggerToggle, useBPMToggle, noteDropdown, 
        dottedNoteToggle, waveSelect;
    };

    struct IOIndices {
        uint32_t ampInput, freqInput, channelInput, signalOutput;
    };

    LFONode(const NodeParams& ctx) : Node(ctx) {
        Setup(TYPE, DISPLAY_NAME);

        m_shape = ImVec2(5, 5);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder()
                .AddColumnsEx(3, {0.5, 4, 0.5})
                .Push(0)
                    .AddRowsEx(2, {1, 3})
                    .Push(0)
                    .AddRows(3)
                    .GetIndexN({&_C.freqInput, &_C.ampInput, &_C.channelInput})
                    .Pop()
                .Pop()
                .Push(1)
                    .AddRows(4)
                    .GetIndex(&_C.waveSelect, 0)
                    .Push(1)
                        .AddColumns(3)
                        .GetIndexN({&_C.freqKnob, &_C.ampKnob, &_C.phaseKnob})
                    .Pop()
                    .Push(2)
                        .AddColumns(4)
                        .GetIndexN({&_C.retriggerToggle, &_C.useBPMToggle, &_C.dottedNoteToggle, &_C.noteDropdown})
                    .Pop()
                .Pop()
                .GetIndex(&_C.signalOutput, 2)
                .Build());

        _IO.freqInput = AddInput("freq", PinDataType::kFloat, 0.0f, _C.freqInput);
        _IO.ampInput = AddInput("amp", PinDataType::kFloat, 0.0f, _C.ampInput);
        _IO.channelInput = AddInput("channel", PinDataType::kChannel, Channel{}, _C.channelInput);
        _IO.signalOutput = AddOutput("signal", PinDataType::kFloat, 0.0f, _C.signalOutput);

        ADD_PARAM(m_freq);
        ADD_PARAM(m_amp);
        ADD_PARAM(m_phase);
        ADD_PARAM(m_retrigger);
        ADD_PARAM(m_useBPM);
        ADD_PARAM(m_dottedNote);
        ADD_PARAM(m_waveIndex);

        constexpr uint32_t wavetableNumSamples = kSampleRate / 2;

        ArrayGet(m_waves, OscType::kSine) = std::make_unique<WavetableOscillator>(wavetableNumSamples, GenWaveSine);
        ArrayGet(m_waves, OscType::kSquare) = std::make_unique<WavetableOscillator>(wavetableNumSamples, GenWaveSquare);
        ArrayGet(m_waves, OscType::kTriangle) = std::make_unique<WavetableOscillator>(wavetableNumSamples, GenWaveTri);
        ArrayGet(m_waves, OscType::kSaw) = std::make_unique<WavetableOscillator>(wavetableNumSamples, GenWaveSaw);
        m_phaseAccum.resize(NumVoices());
    }

    ~LFONode() {}

    void Process(float time) override {
        float freq = m_freq;
        float amp = m_amp;

        if (inputs[_IO.freqInput]->IsConnected()) {
            freq = GetInputValue<float>(_IO.freqInput);
        }

        if (inputs[_IO.ampInput]->IsConnected()) {
            amp = m_amp * GetInputValue<float>(_IO.ampInput);
        }

        // Early return for quiet channels
        if (amp < 0.01f) {
            SetOutputValue<float>(_IO.signalOutput, 0.0f);
            return;
        }

        int voiceIdx = GetActiveVoice();

        if (m_retrigger) {
            Channel const& channel = GetInputValue<Channel>(_IO.channelInput);
            if (!channel.active) {
                m_phaseAccum[voiceIdx] = 0.0f;
            }
        }

        float wave = GetWaveValue(*m_waves[m_waveIndex], voiceIdx, freq, amp);

        SetOutputValue<float>(_IO.signalOutput, wave);
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
        DrawKnobEx(ui, GetComponent(_C.freqKnob), "Freq", &m_freq, KnobParams<float>{
            .minValue = 0.0f, .maxValue = 10.0f, .defaultValue = 1.0f, .format = "%.2f" 
        });

        DrawKnobEx(ui, GetComponent(_C.ampKnob), "Amp", &m_amp, KnobParams<float>{
            .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 0.5f, .format = "%.2f" 
        });

        DrawKnobEx(ui, GetComponent(_C.phaseKnob), "Phase", &m_phase, KnobParams<float>{
            .minValue = 0.0f, .maxValue = 3.14f, .defaultValue = 0.5f, .format = "%.2f" 
        });

        DrawCheckboxEx(ui, GetComponent(_C.retriggerToggle), "Retrigger", &m_retrigger);
        DrawCheckboxEx(ui, GetComponent(_C.useBPMToggle), "Use BPM", &m_useBPM);
        DrawCheckboxEx(ui, GetComponent(_C.dottedNoteToggle), "Dotted", &m_dottedNote);
    }

    ComponentIndices _C{};
    IOIndices _IO{};

    std::array<std::unique_ptr<WavetableOscillator>, NUM_WAVES> m_waves;
    std::vector<float> m_phaseAccum;
    uint32_t m_waveIndex = 0u;

    float m_freq = 1.0f;
    float m_amp = 0.5f;
    float m_phase = 0.5f;

    bool m_retrigger = false;
    bool m_useBPM = false;
    bool m_dottedNote = false;
};
