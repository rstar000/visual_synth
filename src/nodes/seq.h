#pragma once

#include <cmath>

#include "ImGui_Piano_imp.h"
#include "imgui.h"
#include "midi.h"
#include "node.h"
#include "node_types.h"
#include "util.h"

#include "GridUI/Widgets/Knob.hpp"

// Will enable the note every tick in the measure for a specified amount of time
struct ClockNode : public Node {
    static inline const std::string DISPLAY_NAME = "Clock";
    static inline const NodeType TYPE = NodeType::CLOCK;

    ClockNode(const NodeParams& ctx) : Node(ctx), oct(1) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        AddOutput("ch", PinDataType::kChannel, Channel{});

        bpm_slider_label = GenLabel("slider", this);
        bpm_label = GenLabel("bpm", this);
        measure_label = GenLabel("measure", this);
        note_size_label = GenLabel("size", this);
    }

    ~ClockNode() {}

    void Process(float time) override {
        Channel& value = GetOutputValue<Channel>(0);
        float quater_beat_length = 60.0f / bpm_param;
        float measure_beat_length =
            quater_beat_length * 4.0f / static_cast<float>(measure_param[1]);
        int beat_id =
            static_cast<int>(time / measure_beat_length) % measure_param[0];

        value.note = beat_id == 0 ? oct.Get(Tone::C) : oct.Get(Tone::G);
        value.begin =
            std::floor(time / measure_beat_length) * measure_beat_length;
        value.end = value.begin + measure_beat_length * note_size;
        value.velocity = time > value.end ? 0.0f : 1.0f;
    }

    void Draw() override {
        ImGui::PushItemWidth(100.0f);
        ImGui::SliderFloat(bpm_slider_label.c_str(), &bpm_param, 0.0f, 1000.0f,
                           "%.2f", ImGuiSliderFlags_None);
        ImGui::InputFloat(bpm_label.c_str(), &bpm_param, 0.0f, 0.0f, "%.2f",
                          ImGuiInputTextFlags_None);
        ImGui::InputInt2(measure_label.c_str(), measure_param.data(),
                         ImGuiInputTextFlags_None);
        ImGui::InputFloat(note_size_label.c_str(), &note_size, 0.0f, 0.0f,
                          "%.2f", ImGuiInputTextFlags_None);
        ImGui::PopItemWidth();
    }

    void Save(nlohmann::json& j) const override {
        JsonSetValue(j, "bpm", bpm_param);
        JsonSetValue(j, "measure", measure_param);
        JsonSetValue(j, "note_size", note_size);
    }

    void Load(const nlohmann::json& j) override {
        JsonGetValue(j, "bpm", bpm_param);
        JsonGetValue(j, "measure", measure_param);
        JsonGetValue(j, "note_size", note_size);
    }

   protected:
    Octave oct;
    float bpm_param = 100.0f;                   // Quater notes per minute
    std::array<int, 2> measure_param = {4, 4};  // Ex. 3 / 4
    float note_size = 0.5f;  // Fraction of the note in relation to measure.

    std::string bpm_slider_label;
    std::string bpm_label;
    std::string measure_label;
    std::string note_size_label;
};

// Unpack Channel input into different components
struct ChannelUnpackNode : public Node {
    static inline const std::string DISPLAY_NAME = "Channel unpack";
    static inline const NodeType TYPE = NodeType::CHANNEL_UNPACK;

    struct ComponentIndices {
        uint32_t channelInput;
        uint32_t freqOutput;
        uint32_t beginOutput;
        uint32_t endOutput;
        uint32_t velOutput;
        uint32_t timeOutput;
        uint32_t activeOutput;
    };

    ChannelUnpackNode(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(2, 2);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder()
                .AddColumns(2)
                .Push(0)
                .AddRows(5)
                .GetIndex(&m_indices.channelInput, 2)
                .Pop()
                .Push(1)
                .AddRows(6)
                .GetIndex(&m_indices.freqOutput, 0)
                .GetIndex(&m_indices.beginOutput, 1)
                .GetIndex(&m_indices.endOutput, 2)
                .GetIndex(&m_indices.velOutput, 3)
                .GetIndex(&m_indices.timeOutput, 4)
                .GetIndex(&m_indices.activeOutput, 5)
                .Build());

        AddInput("ch", PinDataType::kChannel, Channel{},
                 m_indices.channelInput);
        AddOutput("freq", PinDataType::kFloat, 0.0f, m_indices.freqOutput);
        AddOutput("begin", PinDataType::kFloat, 0.0f, m_indices.beginOutput);
        AddOutput("end", PinDataType::kFloat, 0.0f, m_indices.endOutput);
        AddOutput("vel", PinDataType::kFloat, 0.0f, m_indices.velOutput);
        AddOutput("time", PinDataType::kFloat, 0.0f, m_indices.timeOutput);
        AddOutput("active", PinDataType::kFloat, 0.0f, m_indices.activeOutput);
    }

    ~ChannelUnpackNode() {}

    void Process(float time) override {
        auto in = GetInputValue<Channel>(0);
        SetOutputValue<float>(0, in.note.frequency);
        SetOutputValue<float>(1, in.begin);
        SetOutputValue<float>(2, in.end);
        SetOutputValue<float>(3, in.velocity);
        SetOutputValue<float>(4, std::max(0.0f, time - in.begin));
        SetOutputValue<float>(5, in.active ? 1.0f : 0.0f);
    }

   private:
    ComponentIndices m_indices;
};

struct KeyboardData {
    std::array<std::uint8_t, 128> keys;
};

inline bool TestPianoBoardFunct(void* UserData, int Msg, int Key, float Vel) {
    KeyboardData* kbd = static_cast<KeyboardData*>(UserData);
    if (Key <= 0 || Key >= 128) return false;  // midi max keys
    if (Msg == NoteGetStatus) {
        return kbd->keys[Key];
    }

    if (Msg == NoteOn) {
        kbd->keys[Key] = 1;
    }

    if (Msg == NoteOff) {
        kbd->keys[Key] = 0;
    }
    return true;
}

struct PianoNode : public Node {
    static inline const std::string DISPLAY_NAME = "Piano";
    static inline const NodeType TYPE = NodeType::PIANO;

    PianoNode(const NodeParams& ctx) : Node(ctx) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(10, 2);
        m_layout =
            std::make_unique<GridLayout>(GridLayoutBuilder()
                                             .AddColumnsEx(2, {9.5f, 0.5f})
                                             .Build());

        AddOutput("ch", PinDataType::kChannel, Channel{}, 1u);
        std::fill(kbd.keys.begin(), kbd.keys.end(), 0);
        for (int i = 0; i < 7; ++i) {
            oct.push_back(Octave(i - 2));
        }
    }

    ~PianoNode() {}

    void Process(float time) override {
        if (GetActiveVoice() != 0) {
            return;
        }
        auto& out = GetOutputValue<Channel>(0);
        out.velocity = 0.0f;

        bool pressed = false;
        for (int i = 0; i < kbd.keys.size(); ++i) {
            if (kbd.keys[i]) {
                if (i < 3) {
                    break;
                }

                int note_idx =
                    i - kMidiOffset + 3;  // We have some invisible keys...
                if (note_idx < 0) {
                    break;
                }
                int oct_idx = note_idx / kHalfStepsPerOctave;
                if (oct_idx >= oct.size()) {
                    break;
                }

                int oct_note_idx = note_idx % kHalfStepsPerOctave;
                auto& note = oct[oct_idx].Get(static_cast<Tone>(oct_note_idx));
                out.note = note;
                pressed = true;

                // Just pressed
                if (!prev_pressed) {
                    prev_pressed = true;
                    press_begin = time;
                }
                out.begin = press_begin;
                out.end = time + 100.0f;
                out.velocity = 1.0f;
                break;
            }
        }

        if (!pressed) {
            if (prev_pressed) {
                // Just released
                // prev_note = -1;
                prev_pressed = false;
                press_end = time;
            }

            out.end = press_end;
            out.velocity = 0.0f;
        }
    }

    void Draw() override {
        m_ctx.ui->DrawComponent(m_layout->GetComponent(0), [this](ImRect dst) {
            ImGui::SetCursorScreenPos(dst.Min);
            ImGui_PianoKeyboard("##piano", dst.GetSize(), &prev_note, 21, 108,
                                TestPianoBoardFunct, &kbd, nullptr);
        });
    }

   private:
    KeyboardData kbd;
    int prev_note;
    std::vector<Octave> oct;

    bool prev_pressed = false;
    float press_begin = 0.0f;
    float press_end = 0.0f;
};

struct MidiNode : public Node {
    static inline const std::string DISPLAY_NAME = "Midi in";
    static inline const NodeType TYPE = NodeType::MIDI;

    struct VoiceState {
        bool is_active = false;
        float begin_ts = 0.0f;  // Last change
        float end_ts = 0.0f;    // Last change
    };

    MidiNode(const NodeParams& ctx)
        : Node(ctx), tracker(ctx.tracker), state(NumVoices()) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(1, 1);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder()
                .AddColumns(1).Build());

        AddOutput("ch", PinDataType::kChannel, Channel{}, 0u);
        for (int i = 0; i < 7; ++i) {
            oct.push_back(Octave(i - 2));
        }
    }

    ~MidiNode() {}

    bool GetNote(int note_idx, Note* dst) {
        if (note_idx < 3) {
            return false;
        }

        note_idx -= 24;  // We have some invisible keys...
        int oct_idx = note_idx / 12;
        if (oct_idx >= oct.size()) {
            return false;
        }

        int oct_note_idx = note_idx % 12;
        *dst = oct[oct_idx].Get(static_cast<Tone>(oct_note_idx));
        return true;
    }

    void Process(float time) override {
        int voice_idx = GetActiveVoice();
        auto& out = GetOutputValue<Channel>(0);

        if (tracker->voices[voice_idx].is_active &&
            !state[voice_idx].is_active) {
            // Note started playing
            state[voice_idx].is_active = true;
            state[voice_idx].begin_ts = time;
            state[voice_idx].end_ts = time + 100.0f;
        } else if (!tracker->voices[voice_idx].is_active &&
                   state[voice_idx].is_active) {
            // Note stopped playing
            state[voice_idx].is_active = false;
            state[voice_idx].end_ts = time;
        }

        // if (!state[voice_idx].is_active) {
        //   out.velocity = tracker->voices[voice_idx].velocity / 128.0f;
        //   out.begin = state[voice_idx].begin_ts;
        //   out.end = state[voice_idx].end_ts;
        // } else {
        // }
        out.velocity = tracker->voices[voice_idx].velocity / 128.0f;
        GetNote(tracker->voices[voice_idx].note_idx, &out.note);
        out.begin = state[voice_idx].begin_ts;
        out.end = state[voice_idx].end_ts;
        out.active = state[voice_idx].is_active;
    }

    MidiTracker* tracker;
    std::vector<VoiceState> state;
    std::vector<Octave> oct;
};

struct MidiControlNode : public Node {
    static inline const std::string DISPLAY_NAME = "Midi control";
    static inline const NodeType TYPE = NodeType::MIDI_CONTROL;

    struct ComponentIndices {
        uint32_t controlIndex;
        uint32_t controlKnob;
        uint32_t valueOutput;
    };

    MidiControlNode(const NodeParams& ctx) : Node(ctx), m_tracker(ctx.tracker) {
        type = TYPE;
        display_name = DISPLAY_NAME;

        m_shape = ImVec2(3, 1);
        m_layout = std::make_unique<GridLayout>(
            GridLayoutBuilder()
                .AddColumns(3)
                .GetIndex(&m_indices.controlIndex, 0)
                .GetIndex(&m_indices.controlKnob, 1)
                .GetIndex(&m_indices.valueOutput, 2)
                .Build());

        AddOutput("value", PinDataType::kFloat, 0.0f, m_indices.valueOutput);
    }

    ~MidiControlNode() {}

    void Process(float time) override {
        auto& out = GetOutputValue<float>(0);

        uint8_t& midiValue = m_tracker->controls[m_controlIdx];
        m_value = midiValue;
        out = static_cast<float>(midiValue) / 128.0f;
    }

    void Draw() override {
        GridUI& ui = *m_ctx.ui;

        ui.BeginComponent(m_layout->GetComponent(m_indices.controlIndex));
            DrawKnobInt(ui, "Control ID", &m_controlIdx, KnobParams<int>{
                .minValue = 0, .maxValue = 255, .defaultValue = 1, .format = "%d"
            });
        ui.EndComponent();

        ui.BeginComponent(m_layout->GetComponent(m_indices.controlKnob));
            DrawKnobInt(ui, "Value", &m_value, KnobParams<int>{
                .minValue = 0, .maxValue = 255, .defaultValue = 1, .format = "%d"
            });
        ui.EndComponent();
    }

    MidiTracker* m_tracker;
    ComponentIndices m_indices;
    int m_controlIdx;
    int m_value;
};

