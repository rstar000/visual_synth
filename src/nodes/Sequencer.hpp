#pragma once

#include <bits/types/time_t.h>
#include <cmath>
#include <array>

#include "GridUI/Colors.hpp"
#include "imgui.h"
#include "node.h"
#include "node_types.h"
#include "util.h"

#include "GridUI/Widgets/Fader.hpp"
#include "GridUI/Widgets/Checkbox.hpp"
#include "GridUI/Widgets/Knob.hpp"

#include "Sequencers/PolySeq.hpp"

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
        Setup(TYPE, DISPLAY_NAME);

        m_shape = ImVec2(9, 2);

        auto builder = GridLayoutBuilder();
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
        float bpm = inputs[0]->IsConnected() ? GetInputValueEx<float>(0, 0) : m_ctx.playback->bpm;
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
        GridUI const& ui = *m_ctx.ui;
        for (uint32_t x = 0; x < NUM_BEATS; ++x) {
            m_ctx.ui->BeginComponent(m_layout->GetComponent(m_indices.volumeFaders[x]));
                DrawFaderRect(ui, "##volume", &m_volume[x], 
                    FaderRectParams{
                        .minValue = 0.0f,
                        .maxValue = 1.0f,
                        .format = "%.2f",
                        .speed = 0.0f,
                        .highlighted = (x == m_beatIdx)
                }); 
            m_ctx.ui->EndComponent();

            m_ctx.ui->BeginComponent(m_layout->GetComponent(m_indices.durationFaders[x]));
                DrawFaderRect(ui, "##duration", &m_duration[x], 
                    FaderRectParams{
                        .minValue = 0.0f,
                        .maxValue = 1.0f,
                        .format = "%.2f",
                        .speed = 0.0f,
                        .highlighted = (x == m_beatIdx)
                });
            m_ctx.ui->EndComponent();
        }
    }

private:
    ComponentIndices m_indices;
    uint32_t m_beatIdx = 0;
    std::array<float, NUM_BEATS> m_volume;
    std::array<float, NUM_BEATS> m_duration;
};

struct HoldNotesNode : public Node {
    static inline const std::string DISPLAY_NAME = "Hold notes";
    static inline const NodeType TYPE = NodeType::HOLD_NOTES;

    static constexpr int NUM_NOTES = 8;

    struct ComponentIndices {
        std::array<uint32_t, NUM_NOTES> semitoneKnob;
        std::array<uint32_t, NUM_NOTES> noteEnable;
        uint32_t channelOutput;
    };

    HoldNotesNode(const NodeParams& ctx) : Node(ctx) {
        Setup(TYPE, DISPLAY_NAME);
        m_shape = ImVec2(9, 2);

        auto builder = GridLayoutBuilder();
        builder.AddColumnsEx(3, {0.5f, 8.0f, 0.5f});
        builder.GetIndex(&m_indices.channelOutput, 2);
        builder.Push(1).MakeRectGrid(NUM_NOTES, 2);

        auto GetIndexXY = [&] (uint32_t x, uint32_t y, uint32_t* idx) {
            builder.Push(y);
            builder.GetIndex(idx, x);
            builder.Pop();
        };

        for (uint32_t x = 0; x < NUM_NOTES; ++x) {
            GetIndexXY(x, 0, &m_indices.semitoneKnob[x]);
            GetIndexXY(x, 1, &m_indices.noteEnable[x]);
        }

        m_layout = std::make_unique<GridLayout>(builder.Build());

        AddOutput("ch", PinDataType::kChannel, Channel{}, m_indices.channelOutput);

        for (uint32_t noteIdx = 0; noteIdx < NUM_NOTES; ++noteIdx) {
            AddParam(string_format("note_%u_semitone", noteIdx), &m_semitone.at(noteIdx));
            AddParam(string_format("note_%u_enable", noteIdx), &m_enable.at(noteIdx));
        }

        std::fill(m_semitone.begin(), m_semitone.end(), 0);
        std::fill(m_enable.begin(), m_enable.end(), false);
    }

    ~HoldNotesNode() {}

    void Process(float time) override {
        uint32_t activeVoice = GetActiveVoice();

        if (activeVoice >= NUM_NOTES) {
            return;
        }

        if (m_enable[activeVoice]) {
            Channel ch{
                .note = m_note[activeVoice],
                .begin = 0.0f,
                .end = time + 100.0f,
                .velocity = 1.0f,
                .active = true
            };

            SetOutputValue<Channel>(0, ch);
        } else {
            Channel ch{
                .note = m_note[activeVoice],
                .begin = time + 100.0f,
                .end = time + 200.0f,
                .velocity = 0.0f,
                .active = false
            };

            SetOutputValue<Channel>(0, ch);
        }
    }

    void Draw() override {
        GridUI& ui = *m_ctx.ui;
        int const maxNoteIdx = static_cast<int>(m_noteLookup.Size()) - 1;
        for (uint32_t x = 0; x < NUM_NOTES; ++x) {
            ui.BeginComponent(m_layout->GetComponent(m_indices.semitoneKnob[x]));
                const char* activeNoteName = m_noteLookup.Get(m_semitone[x]).name.c_str();
                DrawKnobInt(ui, "Semitone", &m_semitone[x], KnobParams<int>{
                    .minValue = 0, .maxValue = maxNoteIdx, .defaultValue = 0, .format = activeNoteName, .colorIndex = 0
                });
            ui.EndComponent();

            ui.BeginComponent(m_layout->GetComponent(m_indices.noteEnable[x]));
                DrawCheckbox(*m_ctx.ui, "", &m_enable[x]);
            ui.EndComponent();

            m_note[x] = m_noteLookup.Get(m_semitone[x]).note;
        }
    }

    ComponentIndices m_indices{};
    std::array<int32_t, NUM_NOTES> m_semitone;
    std::array<Note, NUM_NOTES> m_note;
    std::array<bool, NUM_NOTES> m_enable;
    NoteLookup m_noteLookup{};
};

struct Arpeggiator : public Node {
    static inline const std::string DISPLAY_NAME = "Arpeggiator";
    static inline const NodeType TYPE = NodeType::ARPEGGIATOR;

    static constexpr uint32_t NUM_BEATS = 16;
    static constexpr uint32_t MAX_NUM_NOTES = 8;

    struct ComponentIndices
    {
        std::array<uint32_t, NUM_BEATS> volumeFaders;
        std::array<uint32_t, NUM_BEATS> durationFaders;
        uint32_t channelInput;
        uint32_t bpmInput;
        uint32_t channelOutput;
    };

    Arpeggiator(const NodeParams& ctx) : Node(ctx) {
        Setup(TYPE, DISPLAY_NAME);

        m_shape = ImVec2(9, 2);

        auto builder = GridLayoutBuilder();
        builder.AddColumnsEx(3, {0.5f, 8.0f, 0.5f});
        builder.Push(0).AddRows(2);
            builder.GetIndex(&m_indices.bpmInput, 0);
            builder.GetIndex(&m_indices.channelInput, 1);
        builder.Pop();
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

        AddInput("ch", PinDataType::kChannel, Channel{}, m_indices.channelInput);
        AddInput("bpm", PinDataType::kFloat, 100.0f, m_indices.bpmInput);
        AddOutput("ch", PinDataType::kChannel, Channel{}, m_indices.channelOutput);

        for (uint32_t beatIdx = 0; beatIdx < NUM_BEATS; ++beatIdx) {
            AddParam(string_format("beat_%u_volume", beatIdx), &m_volume.at(beatIdx));
            AddParam(string_format("beat_%u_duration", beatIdx), &m_duration.at(beatIdx));
        }

        std::fill(m_volume.begin(), m_volume.end(), 1.0f);
        std::fill(m_duration.begin(), m_duration.end(), 0.05f);
    }

    ~Arpeggiator() {}

    void UpdateInputNotes(float time) {
        m_numActiveNotes = 0;
        m_beginTime = time + 100.0f;
        for (uint32_t voiceIdx = 0; voiceIdx < NumVoices(); ++voiceIdx) {
            Channel const& v = GetInputValueEx<Channel>(0, voiceIdx);
            if (v.begin < time && v.end > time) {
                m_sortedNotes[m_numActiveNotes] = v.note;
                ++m_numActiveNotes;
                m_beginTime = std::min(m_beginTime, v.begin);
            }

            if (m_numActiveNotes >= MAX_NUM_NOTES) {
                break;
            }
        }

        std::sort(
            m_sortedNotes.begin(),
            m_sortedNotes.begin() + m_numActiveNotes,
            [] (const Note& a, const Note& b) {
                return a.frequency < b.frequency;
            }
        );
    }

    void Preprocess(float time) override {
        float bpm = inputs[1]->IsConnected() ? GetInputValueEx<float>(1, 0) : m_ctx.playback->bpm;

        float bps = bpm / 60.0f;

        size_t sampleIdx = m_ctx.playback->sampleIdx;
        // Next beat triggered
        uint32_t numOutputChannels = std::min(NUM_BEATS, (uint32_t)NumVoices());
        if (sampleIdx > m_updateTimestamp) {
            UpdateInputNotes(time);
            m_updateTimestamp = sampleIdx + std::floor(static_cast<float>(m_ctx.playback->sampleRate) / bps);
            m_currentVoice = (m_currentVoice + 1) % numOutputChannels;

            m_beatIdx = (m_beatIdx + 1) % NUM_BEATS;

            Channel& curChannel = m_channelState[m_currentVoice];
            if (m_numActiveNotes > 0) {
                curChannel.note = m_sortedNotes[m_beatIdx % m_numActiveNotes];
                curChannel.begin = time;
                curChannel.end = time + m_duration[m_beatIdx];
                curChannel.velocity = m_volume[m_beatIdx];
                curChannel.active = true;
            } else {
                curChannel.begin = time + 100.0f;
                curChannel.end = time + 200.0f;
                curChannel.velocity = 0.0f;
                curChannel.active = false;
            }
        }
    }

    void Process(float time) override {
        uint32_t numVoices = NumVoices();
        uint32_t activeVoice = GetActiveVoice();

        uint32_t numOutputChannels = std::min(NUM_BEATS, numVoices);

        if (activeVoice >= numOutputChannels) {
            Channel ch{
                .note = Note(0, 0),
                .begin = time + 100.0f,
                .end = time + 200.0f,
                .velocity = 0.0f,
                .active = false
            };

            SetOutputValue<Channel>(0, ch);
        } else {
            SetOutputValue<Channel>(0, m_channelState[activeVoice]);
        }
    }

    void Draw() override {
        GridUI& ui = *m_ctx.ui;
        for (uint32_t x = 0; x < NUM_BEATS; ++x) {
            m_ctx.ui->BeginComponent(m_layout->GetComponent(m_indices.volumeFaders[x]));
                DrawFaderRect(ui, "##volume", &m_volume[x], 
                    FaderRectParams{
                        .minValue = 0.0f,
                        .maxValue = 1.0f,
                        .format = "%.2f",
                        .speed = 0.0f,
                        .highlighted = (x == m_beatIdx)
                }); 
            m_ctx.ui->EndComponent();

            m_ctx.ui->BeginComponent(m_layout->GetComponent(m_indices.durationFaders[x]));
                DrawFaderRect(ui, "##duration", &m_duration[x], 
                    FaderRectParams{
                        .minValue = 0.0f,
                        .maxValue = 1.0f,
                        .format = "%.2f",
                        .speed = 0.0f,
                        .highlighted = (x == m_beatIdx)
                });
            m_ctx.ui->EndComponent();
        }
    }

private:
    ComponentIndices m_indices;
    uint32_t m_beatIdx = 0;
    Channel m_activeBeat{};
    std::array<float, NUM_BEATS> m_volume;
    std::array<float, NUM_BEATS> m_duration;

    // Beat fields
    std::array<Note, MAX_NUM_NOTES> m_sortedNotes;
    std::array<Channel, NUM_BEATS> m_channelState;
    uint32_t m_numActiveNotes = 0;
    float m_beginTime = 0.0f;

    // Output voice fields
    uint32_t m_currentVoice = 0;
    size_t m_updateTimestamp = 0;
};


struct PolySequencerNode : public Node {
    static inline const std::string DISPLAY_NAME = "PolySequencer";
    static inline const NodeType TYPE = NodeType::ARPEGGIATOR;

    static constexpr uint32_t NUM_BEATS = 32;
    static constexpr uint32_t MAX_NUM_CHANNELS = 4;
    static constexpr uint32_t MAX_NUM_NOTES = 8;

    struct ComponentIndices
    {
        uint32_t bpmInput;
        uint32_t channelOutput;
        uint32_t stepSizeToggle;
        uint32_t numStepsKnob;
        uint32_t indicatorText;
        std::array<uint32_t, NUM_BEATS * MAX_NUM_CHANNELS> beats;
    };

    PolySequencerNode(const NodeParams& ctx) 
        : Node(ctx)
        , m_seq(MAX_NUM_CHANNELS)
    {
        Setup(TYPE, DISPLAY_NAME);
        m_shape = ImVec2(9, 4);

        auto builder = GridLayoutBuilder();
        builder.AddColumnsEx(3, {0.5f, 8.0f, 0.5f});
        builder.Push(0).AddRows(2);
            builder.GetIndex(&m_indices.bpmInput, 0);
        builder.Pop();
            builder.GetIndex(&m_indices.channelOutput, 2);
        builder.Push(1).AddRowsEx(2, {1, 3})
            .Push(0).AddColumnsEx(3, {1, 1, 6})
                .GetIndex(&m_indices.numStepsKnob, 0)
                .GetIndex(&m_indices.stepSizeToggle, 1)
                .GetIndex(&m_indices.indicatorText, 2)
            .Pop()
            .Push(1)
            .MakeRectGrid(NUM_BEATS, MAX_NUM_CHANNELS);

        for (uint32_t x = 0; x < NUM_BEATS; ++x) {
            for (uint32_t y = 0; y < MAX_NUM_CHANNELS; ++y) {
                builder.GetIndexXY(x, y, &m_indices.beats[NUM_BEATS * y + x]);
            }
        }

        m_layout = std::make_unique<GridLayout>(builder.Build());

        AddInput("bpm", PinDataType::kFloat, 100.0f, m_indices.bpmInput);
        AddOutput("ch", PinDataType::kChannel, Channel{}, m_indices.channelOutput);
    }

    ~PolySequencerNode() {}

    void Preprocess(float time) override {
        float bpm = GetInputValueEx<float>(1, 0);
        float bps = bpm / 60.0f;

        size_t sampleIdx = m_ctx.playback->sampleIdx;
        uint32_t beatIdx = static_cast<uint32_t>(bps * time);

        if (beatIdx != m_beatIdx) {
            float t_begin = time;
            float t_end = time;

            // Update
        }
    }

    void Process(float time) override {
        /* uint32_t numVoices = NumVoices(); */
        /* uint32_t activeVoice = GetActiveVoice(); */
        /**/
        /* uint32_t numOutputChannels = std::min(NUM_BEATS, numVoices); */
        /**/
        /* if (activeVoice >= numOutputChannels) { */
        /*     Channel ch{ */
        /*         .note = Note(0, 0), */
        /*         .begin = time + 100.0f, */
        /*         .end = time + 200.0f, */
        /*         .velocity = 0.0f, */
        /*         .active = false */
        /*     }; */
        /**/
        /*     SetOutputValue<Channel>(0, ch); */
        /* } else { */
        /*     SetOutputValue<Channel>(0, m_channelState[activeVoice]); */
        /* } */
    }

    void Draw() override {
        GridUI& ui = *m_ctx.ui;
        for (uint32_t x = 0; x < NUM_BEATS; ++x) {
        }
    }

private:
    ComponentIndices m_indices;
    PolySequencer m_seq;
    uint32_t m_beatIdx = 0;
    // Beat fields
    uint32_t m_currentVoice = 0;
    size_t m_updateTimestamp = 0;
};
