#pragma once

#include <cmath>
#include <new>

#include "GridUI/Widgets/Knob.hpp"
#include "GridUI/Widgets/Toggle.hpp"
#include "Oscillators/Common.hpp"
#include "Oscillators/Waveform.hpp"
#include "Param.hpp"
#include "imgui.h"
#include "node.h"
#include "node_types.h"

struct SineOscillatorNode : public Node {
    static inline const std::string DISPLAY_NAME = "Sine wave";
    static inline const NodeType TYPE = NodeType::SINE_OSC;

    struct ComponentIndices {
        uint32_t freqInput;
        uint32_t ampInput;
        uint32_t phaseInput;
        uint32_t signalOutput;
        uint32_t freqKnob;
        uint32_t ampKnob;
    };

    SineOscillatorNode(const NodeParams& ctx) : Node(ctx) {
        Setup(TYPE, DISPLAY_NAME);
        m_shape = ImVec2(3, 1);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder()
                .AddColumnsEx(4, {1, 2, 2, 1})
                .Push(0)
                .AddRows(3)
                .GetIndex(&m_indices.freqInput, 0)
                .GetIndex(&m_indices.ampInput, 1)
                .GetIndex(&m_indices.phaseInput, 2)
                .Pop()
                .GetIndex(&m_indices.freqKnob, 1)
                .GetIndex(&m_indices.ampKnob, 2)
                .GetIndex(&m_indices.signalOutput, 3)
                .Build());

        AddInput("freq", PinDataType::kFloat, 440.0f, m_indices.freqInput);
        AddInput("amp", PinDataType::kFloat, 0.5f, m_indices.ampInput);
        AddInput("phase", PinDataType::kFloat, 0.0f, m_indices.phaseInput);
        AddOutput("signal", PinDataType::kFloat, 0.0f, m_indices.signalOutput);

        AddParam<float>("freq", &m_freq);
        AddParam<float>("amp", &m_amp);
    }

    ~SineOscillatorNode() {}

    void Process(float time) override {
        float freq = m_freq;
        float amp = m_amp;
        if (inputs[0]->IsConnected()) {
            freq = GetInputValue<float>(0);
        }

        if (inputs[1]->IsConnected()) {
            amp = GetInputValue<float>(1);
        }
        float phase = GetInputValue<float>(2);
        float wave = amp * sin(time * 2.0f * M_PIf * freq + phase);
        SetOutputValue<float>(0, wave);
    }

    void Draw() override {
        GridUI& ui = *m_ctx.ui;
        ui.BeginComponent(m_layout->GetComponent(m_indices.freqKnob));
            DrawKnob(ui, "Freq", &m_freq, KnobParams<float>{
                .minValue = 1.0f, .maxValue = 1000.0f, .defaultValue = 440.0f, .format = "%.2f" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.ampKnob));
            DrawKnob(ui, "Amp", &m_amp, KnobParams<float>{
                .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 0.5f, .format = "%.2f" 
            });
        ui.EndComponent();
    }

   private:
    float m_freq = 440.0f;
    float m_amp = 0.5f;

    ComponentIndices m_indices;
};

struct SquareOscillatorNode : public Node {
    static inline const std::string DISPLAY_NAME = "Square wave";
    static inline const NodeType TYPE = NodeType::SQUARE_OSC;

    SquareOscillatorNode(const NodeParams& ctx) : Node(ctx) {
        Setup(TYPE, DISPLAY_NAME);
        AddInput("freq", PinDataType::kFloat, 440.0f);
        AddInput("amp", PinDataType::kFloat, 0.5f);
        AddInput("phase", PinDataType::kFloat, 0.0f);
        AddOutput("signal", PinDataType::kFloat, 0.0f);
    }

    ~SquareOscillatorNode() {}

    void Process(float time) override {
        if (inputs[0]->IsConnected()) {
            freq = GetInputValue<float>(0);
        }

        if (inputs[1]->IsConnected()) {
            amp = GetInputValue<float>(1);
        }

        float t = std::fmod(time, 1.0f / freq) * freq;
        float wave = amp * (t > 0.5f ? 1.0f : -1.0f);
        SetOutputValue<float>(0, wave);
    }

   private:
    float freq = 440.0f;
    float amp = 0.5f;
};

struct SuperOscNode : public Node {
    enum class OscType : uint32_t {
        kSine = 0,
        kSquare,
        kTriangle,
        kSaw,
        kNoise
    };
    static inline const std::string DISPLAY_NAME = "HyperOsc";
    static inline const NodeType TYPE = NodeType::SUPER_OSC;
    static constexpr size_t NUM_WAVES = 5;
    static constexpr size_t MAX_UNISON_COUNT = 10;
    static constexpr size_t NUM_CENTS_DETUNE = 100;

    struct ComponentIndices {
        uint32_t freqInput;
        uint32_t ampInput;
        uint32_t phaseInput;
        uint32_t timeInput;
        uint32_t signalOutput;

        uint32_t octaveKnob;
        uint32_t semitoneKnob;
        uint32_t centsKnob;

        uint32_t ampKnob;
        uint32_t unisonWidthKnob;
        uint32_t unisonCountKnob;
        uint32_t waveSelectMenu;
    };

    SuperOscNode(const NodeParams& ctx) : Node(ctx) {
        Setup(TYPE, DISPLAY_NAME);
        m_shape = ImVec2(4, 4);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder()
                .AddColumnsEx(3, {0.5, 3, 0.5})
                .Push(0)
                .AddRows(4)
                    .GetIndex(&m_indices.freqInput, 0)
                    .GetIndex(&m_indices.ampInput, 1)
                    .GetIndex(&m_indices.phaseInput, 2)
                    .GetIndex(&m_indices.timeInput, 3)
                .Pop()
                .Push(1)
                .AddRowsEx(3, {1, 1, 1})
                    .GetIndex(&m_indices.waveSelectMenu, 0)
                .Push(1)
                .AddColumns(3)
                    .GetIndex(&m_indices.octaveKnob, 0)
                    .GetIndex(&m_indices.semitoneKnob, 1)
                    .GetIndex(&m_indices.centsKnob, 2)
                .Pop()
                .Push(2)
                .AddColumns(3)
                    .GetIndex(&m_indices.ampKnob, 0)
                    .GetIndex(&m_indices.unisonWidthKnob, 1)
                    .GetIndex(&m_indices.unisonCountKnob, 2)
                .Pop()
                .Pop()
                    .GetIndex(&m_indices.signalOutput, 2)
                .Build());

        AddInput("freq", PinDataType::kFloat, 440.0f, m_indices.freqInput);
        AddInput("amp", PinDataType::kFloat, 0.5f, m_indices.ampInput);
        AddInput("phase", PinDataType::kFloat, 0.0f, m_indices.phaseInput);
        AddInput("time", PinDataType::kFloat, 0.0f, m_indices.timeInput);
        AddOutput("signal", PinDataType::kFloat, 0.0f, m_indices.signalOutput);

        AddParam("octave", &m_octave);
        AddParam("semitone", &m_semitone);
        AddParam("cents", &m_cents);
        AddParam("amp", &m_amp);
        AddParam("unisonWidth", &m_unisonWidth);
        AddParam("unisonCount", &m_unisonCount);
        AddParam("waveIndex", &m_waveIndex);

        constexpr uint32_t wavetableNumSamples = kSampleRate / 2;

        ArrayGet(m_waves, OscType::kSine) =
            std::make_unique<WavetableOscillator>(wavetableNumSamples,
                                                  GenWaveSine);
        ArrayGet(m_waves, OscType::kSquare) =
            std::make_unique<WavetableOscillator>(wavetableNumSamples,
                                                  GenWaveSquare);
        ArrayGet(m_waves, OscType::kTriangle) =
            std::make_unique<WavetableOscillator>(wavetableNumSamples,
                                                  GenWaveTri);
        ArrayGet(m_waves, OscType::kSaw) =
            std::make_unique<WavetableOscillator>(wavetableNumSamples,
                                                  GenWaveSaw);
        ArrayGet(m_waves, OscType::kNoise) =
            std::make_unique<WavetableOscillator>(wavetableNumSamples,
                                                  GenWaveNoise);

        m_phaseAccum.resize(NumVoices() * MAX_UNISON_COUNT);

        m_centsDetune.resize(NUM_CENTS_DETUNE);
        for (int i = 0; i < NUM_CENTS_DETUNE; i++) {
            m_centsDetune.at(i) = std::pow(2.0f, i / 1200.0f);
        }

        m_waveSelectToggleParams = {
            .numOptions = 5,
            .tooltips = {"Sine", "Square", "Triangle", "Saw", "Noise"},
            .height = 20.0f,
            .sideMargin = 5.0f
        };
    }

    ~SuperOscNode() {}

    void Process(float time) override {
        /*
          phase_delta = N * f / Fs
          where:

          phase_delta is the number of LUT samples to increment
          freq is the desired output frequency
          Fs is the sample rate
          N is the size of the LUT
        */

        float freq = 440.0f;
        float amp = 0.0f;
        float phaseMod = 0.0f;
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

        if (inputs[2]->IsConnected()) {
            phaseMod = GetInputValue<float>(2);
        }

        auto& osc = m_waves[m_waveIndex];

        float wavesSum = 0.0f;
        int voiceIdx = GetActiveVoice();

        float freqPitchShift = m_pitchShift * freq;
        wavesSum += GetWaveValue(*osc, voiceIdx, 0, freqPitchShift, amp, phaseMod);

        // Add unison
        for (int k = 1; k < m_unisonCount; ++k) {
            float detuneAmount =
                m_unisonWidth * (NUM_CENTS_DETUNE / 2.0f) * float(k) / m_unisonCount;
            if (k % 2) {
                detuneAmount *= -1.0f;
            }

            float freqDetuned = GetDetunedFreq(freqPitchShift, detuneAmount);
            wavesSum += GetWaveValue(*osc, voiceIdx, k, freqDetuned, amp, phaseMod);
        }

        SetOutputValue<float>(0, wavesSum);
    }

    float GetDetunedFreq(float baseFreq, int detuneSec) {
        if (detuneSec == 0) {
            return baseFreq;
        } 

        if (detuneSec > 0) {
            float detuneMod = m_centsDetune[detuneSec];
            return baseFreq * detuneMod;
        } else {
            float detuneMod = m_centsDetune[-detuneSec];
            return baseFreq / detuneMod;
        }
    }

    float ComputePitchShiftMultiplier() {
        float octaveFrac = 1.0f;
        float semitoneFrac = 1.0f;
        if (m_octave > 0) {
            octaveFrac = 1 << m_octave;
        } else if (m_octave < 0) {
            octaveFrac = 1.0f / (1 << (-m_octave));
        }

        if (m_semitone != 0) {
            semitoneFrac = std::pow(kFrequencyMultiplier, m_semitone);
        }
        float pitchShiftedFreq = octaveFrac * semitoneFrac;
        if (m_cents != 0) {
            pitchShiftedFreq = GetDetunedFreq(pitchShiftedFreq, m_cents);
        }

        return pitchShiftedFreq;
    }

    float GetWaveValue(WavetableOscillator& osc, uint32_t voiceIdx,
                       uint32_t unisonIdx, float freq, float amp, float phaseMod) {
        float phaseDelta = (freq / kSampleRate) * 2.0f * M_PIf;
        float& accum = m_phaseAccum[unisonIdx * NumVoices() + voiceIdx];
        accum = std::fmod(accum + phaseDelta, 2.0f * M_PIf);
        return osc.GetWave(accum + phaseMod) * amp;
    }

    void Draw() override {
        GridUI& ui = *m_ctx.ui;
        ui.BeginComponent(m_layout->GetComponent(m_indices.octaveKnob));
            DrawKnobInt(ui, "Freq", &m_octave, KnobParams<int>{
                .minValue = -4, .maxValue = 4, .defaultValue = 0, .format = "%d" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.semitoneKnob));
            DrawKnobInt(ui, "Semitone", &m_semitone, KnobParams<int>{
                .minValue = -12, .maxValue = 12, .defaultValue = 0, .format = "%d" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.centsKnob));
            DrawKnobInt(ui, "Finetune", &m_cents, KnobParams<int>{
                .minValue = -(int)NUM_CENTS_DETUNE + 1, .maxValue = NUM_CENTS_DETUNE - 1, .defaultValue = 0, .format = "%d" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.ampKnob));
            DrawKnob(ui, "Amp", &m_amp, KnobParams<float>{
                .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 0.5f, .format = "%.2f" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.unisonCountKnob));
            DrawKnobInt(ui, "Unison", &m_unisonCount, KnobParams<int>{
                .minValue = 0, .maxValue = 9, .defaultValue = 0, .format = "%d" 
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.unisonWidthKnob));
            DrawKnob(ui, "Width", &m_unisonWidth, KnobParams<float>{
                .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 0.0f, .format = "%.2f"});
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.waveSelectMenu));
            DrawToggle(ui, "Wave select", &m_waveIndex, m_waveSelectToggleParams);
        ui.EndComponent();
        m_pitchShift = ComputePitchShiftMultiplier();
    }

   private:
    ToggleParams m_waveSelectToggleParams{};

    int m_octave = 0;
    int m_semitone = 0;
    int m_cents = 0;
    float m_amp = 0.5f;

    float m_unisonWidth = 0.0f;
    int m_unisonCount = 0;

    uint32_t m_waveIndex = 0u;

    float m_pitchShift = 1.0f;

    std::vector<float> m_phaseAccum;
    std::vector<float> m_centsDetune;

    ComponentIndices m_indices;

    std::array<std::unique_ptr<WavetableOscillator>, NUM_WAVES> m_waves;
};
