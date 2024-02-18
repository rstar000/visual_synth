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

#define ADD_PARAM(name) \
    AddParam(#name, &name);
    

// Constructor argument for all nodes
struct NodeParams {
    SampleWriter* writer;
    MidiTracker* tracker;
    GridUI* ui;
    PlaybackContext* playback;
};

struct Signal {
    float left;
    float right;
};

enum class PinDataType { kInt, kFloat, kTimestamp, kChannel, kSignal };

using PinData = std::variant<int, float, std::size_t, Channel, Signal>;

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

    size_t NumInputs()  const { return inputs.size(); }
    size_t NumOutputs() const { return outputs.size(); }
    int NumVoices()     const { return m_ctx.playback->numVoices; }
    ImVec2 GetShape()   const { return m_shape; }
    NodeType GetType()  const { return m_type; }
    std::string const& GetDisplayName() const { return m_displayName; }

    void Process(int voice_idx, int sample_idx, float time) {
        m_activeVoice = voice_idx;
        m_activeSample = sample_idx;
        Process(time);
    }

    virtual void Preprocess(float time) {};
    virtual void Process(float time) = 0;
    virtual void Draw() {}
    virtual void DrawContextMenu() {}

    void Setup(NodeType type, std::string const& displayName) {
        m_type = type;
        m_displayName = displayName;
    }

    template <typename T>
    void AddParam(std::string keyName, T* valuePtr) {
        if constexpr (is_std_array<T>::value) {
            m_params.push_back(std::make_shared<ArrayParam<T>>(keyName, valuePtr));
        } else {
            m_params.push_back(std::make_shared<Param<T>>(keyName, valuePtr));
        }
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
    uint32_t AddInput(const std::string& name, PinDataType dtype,
                  PinData defaultValue, uint32_t componentIdx = 0,
                  bool mono = false) {
        size_t const newInputIdx{inputs.size()};
        int const numVoices = mono ? 1 : NumVoices();
        inputs.push_back(std::make_shared<Input>(
            name, dtype, this, defaultValue, numVoices, componentIdx));
        return static_cast<uint32_t>(newInputIdx);
    }

    uint32_t AddOutput(const std::string& name, PinDataType dtype,
                   PinData defaultValue, uint32_t componentIdx = 0,
                   bool mono = false) {
        size_t const newOutputIdx{outputs.size()};
        int const numVoices = mono ? 1 : NumVoices();
        outputs.push_back(std::make_shared<Output>(
            name, dtype, this, defaultValue, m_ctx.playback->numSamples,
            numVoices, componentIdx));
        return static_cast<uint32_t>(newOutputIdx);
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

    GridComponent const& GetComponent(uint32_t componentIdx) {
        return m_layout->GetComponent(componentIdx);
    }

    NodeParams m_ctx;

    int m_activeSample = 0;
    int m_activeVoice = 0;

    std::string m_displayName;
    NodeType m_type;

    std::vector<InputPtr> inputs;
    std::vector<OutputPtr> outputs;

    std::vector<std::shared_ptr<BaseParam>> m_params;

    std::unique_ptr<GridLayout> m_layout;
    ImVec2 m_shape;
};
