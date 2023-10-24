#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "GridUI/GridLayout.hpp"
#include "GridUI/GridUI.hpp"
#include "Param.hpp"
#include "json.hpp"
#include "midi.h"
#include "node_types.h"
#include "note.h"
#include "output.h"
#include "util.h"
#include "writer.h"

// Constructor argument for all nodes
struct NodeParams {
    SampleWriter* writer;
    MidiTracker* tracker;
    GridUI* ui;
    PlaybackContext* playback;
};

enum class PinDataType { kInt, kFloat, kTimestamp, kChannel };

using PinData = std::variant<int, float, std::size_t, Channel>;

class Node;
// Nodes are owned by the graph
using NodePtr = std::unique_ptr<Node>;

struct Connection {
    Connection(const std::string& name, PinDataType type, Node* parent,
               uint32_t componentIdx)
        : name(name), type(type), parent(parent), componentIdx(componentIdx) {}
    std::string name;
    PinDataType type;
    Node* parent = nullptr;
    uint32_t componentIdx;
};

// Doesn't store reference to its connections
// because there could be multiple connected inputs.
struct Output : public Connection {
    Output(const std::string& name, PinDataType type, Node* parent,
           PinData default_value, int num_samples, int num_voices,
           uint32_t componentIdx)
        : Connection(name, type, parent, componentIdx),
          num_samples(num_samples),
          num_voices(num_voices),
          value(num_voices * num_samples, default_value) {}

    const int num_samples;
    const int num_voices;
    std::vector<PinData> value;

    template <typename T>
    const T& GetValue(int voice_idx, int sample_idx) const {
        return std::get<T>(value[GetIndex(voice_idx, sample_idx)]);
    }

    template <typename T>
    T& GetValue(int voice_idx, int sample_idx) {
        return std::get<T>(value[GetIndex(voice_idx, sample_idx)]);
    }

    template <typename T>
    void SetValue(int voice_idx, int sample_idx, const T& new_value) {
        value[GetIndex(voice_idx, sample_idx)] = new_value;
    }

   private:
    int GetIndex(int voice_idx, int sample_idx) const {
        return num_samples * voice_idx + sample_idx;
    }
};

struct Input : public Connection {
    Input(const std::string& name, PinDataType type, Node* parent,
          PinData default_value, int num_voices, uint32_t componentIdx)
        : Connection(name, type, parent, componentIdx),
          num_voices(num_voices),
          default_value(default_value) {}

    const int num_voices;
    std::shared_ptr<Output> connection;
    PinData default_value;

    template <typename T>
    const T& GetValue(int voice_idx, int sample_idx) const {
        if (connection) {
            return connection->GetValue<T>(voice_idx, sample_idx);
        }
        return std::get<T>(default_value);
    }

    bool Connect(std::shared_ptr<Output> output) {
        if (parent == output->parent || type != output->type ||
            num_voices != output->num_voices) {
            return false;
        }
        connection = output;
        return true;
    }

    void Disconnect() { connection = nullptr; }

    bool IsConnected(std::shared_ptr<Output> output) const {
        if (!connection) {
            return false;
        }

        return connection.get() == output.get();
    }

    bool IsConnected() const { return bool(connection); }
};

using InputPtr = std::shared_ptr<Input>;
using OutputPtr = std::shared_ptr<Output>;

class Node {
   public:
    Node(const NodeParams& ctx) : m_ctx{ctx} { m_shape = ImVec2(2, 2); }

    virtual ~Node() = default;

    InputPtr GetInputByIndex(size_t index) {
        ASSERT(index < inputs.size());
        return inputs[index];
    }

    OutputPtr GetOutputByIndex(size_t index) {
        ASSERT(index < outputs.size());
        return outputs[index];
    }

    size_t NumInputs() const { return inputs.size(); }

    size_t NumOutputs() const { return outputs.size(); }

    int NumVoices() const { return m_ctx.playback->numVoices; }

    ImVec2 GetShape() const { return m_shape; }

    const std::string& GetDisplayName() const { return display_name; }

    NodeType GetType() const { return type; }

    void Process(int voice_idx, int sample_idx, float time) {
        m_activeVoice = voice_idx;
        m_activeSample = sample_idx;
        Process(time);
    }

    virtual void Preprocess(float time) {};
    virtual void Process(float time) = 0;
    virtual void Draw() {}

    template <typename T>
    void AddParam(std::string keyName, T* valuePtr) {
        m_params.push_back(std::make_shared<Param<T>>(keyName, valuePtr));
    }

    virtual void Load(const nlohmann::json& j) {
        for (auto& param : m_params) {
            param->Load(j);
        }
    }

    virtual void Save(nlohmann::json& j) const {
        for (auto& param : m_params) {
            param->Save(j);
        }
    }

    const GridLayout& GetLayout() const {
        ASSERT(bool(m_layout));
        return *m_layout;
    }

   protected:
    void AddInput(const std::string& name, PinDataType dtype,
                  PinData default_value, uint32_t componentIdx = 0,
                  bool mono = false) {
        int _num_voices = mono ? 1 : NumVoices();
        inputs.push_back(std::make_shared<Input>(
            name, dtype, this, default_value, _num_voices, componentIdx));
    }

    void AddOutput(const std::string& name, PinDataType dtype,
                   PinData default_value, uint32_t componentIdx = 0,
                   bool mono = false) {
        int _num_voices = mono ? 1 : NumVoices();
        outputs.push_back(std::make_shared<Output>(
            name, dtype, this, default_value, m_ctx.playback->numSamples,
            _num_voices, componentIdx));
    }

    int GetActiveSample() const { return m_activeSample; }

    int GetActiveVoice() const { return m_activeVoice; }

    template <typename T>
    const T& GetInputValue(int input_idx) {
        return inputs[input_idx]->GetValue<T>(m_activeVoice, m_activeSample);
    }

    template <typename T>
    const T& GetInputValueEx(int inputIdx, int voiceIdx) {
        return inputs[inputIdx]->GetValue<T>(voiceIdx, m_activeSample);
    }

    template <typename T>
    void SetOutputValue(int output_idx, const T& new_value) {
        return outputs[output_idx]->SetValue<T>(m_activeVoice, m_activeSample,
                                                new_value);
    }

    template <typename T>
    void SetOutputValueEx(int outputIdx, int voiceIdx, const T& newValue) {
        return outputs[outputIdx]->SetValue<T>(voiceIdx, m_activeSample,
                                                newValue);
    }

    template <typename T>
    T& GetOutputValue(int output_idx) {
        return outputs[output_idx]->GetValue<T>(m_activeVoice, m_activeSample);
    }

    NodeParams m_ctx;

    int m_activeSample = 0;
    int m_activeVoice = 0;

    std::string display_name;
    NodeType type;
    std::size_t last_update = -1;

    std::vector<InputPtr> inputs;
    std::vector<OutputPtr> outputs;

    std::vector<std::shared_ptr<BaseParam>> m_params;

    std::unique_ptr<GridLayout> m_layout;
    ImVec2 m_shape;
};
