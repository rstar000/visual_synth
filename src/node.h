#pragma once

#include <vector>
#include <variant>
#include <iostream>
#include <optional>
#include <memory>
#include <string>
#include <map>

#include "node_types.h"
#include "util.h"

#include "note.h"
#include "output.h"
#include "midi.h"

#include "json.hpp"

enum class PinDataType {
  kInt,
  kFloat,
  kTimestamp,
  kChannel
};

using PinData = std::variant<
  int, 
  float,
  std::size_t,
  Channel
>;

class Node;
using NodePtr = std::shared_ptr<Node>;


// Constructor argument for all nodes
struct NodeParams {
  std::shared_ptr<SampleWriter> writer;
  std::shared_ptr<MidiTracker> tracker;
  int num_samples;
  int num_voices;
};

struct Connection {
  Connection(const std::string& name, PinDataType type, Node* parent)
    : name(name), type(type), parent(parent) { }
  std::string name;
  PinDataType type;
  Node* parent = nullptr;
};

// Doesn't store reference to its connections
// cause there could be multiple connected inputs.
struct Output : public Connection {
  Output(
    const std::string& name, 
    PinDataType type, 
    Node* parent, 
    PinData default_value,
    int num_samples,
    int num_voices)
      : Connection(name, type, parent)
      , num_samples(num_samples)
      , num_voices(num_voices)
      , value(num_voices * num_samples, default_value) { }
  
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
  Input(
    const std::string& name, 
    PinDataType type, 
    Node* parent, 
    PinData default_value,
    int num_voices)
      : Connection(name, type, parent)
      , num_voices(num_voices)
      , default_value(default_value) { 
  }

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
    if (parent == output->parent || type != output->type || num_voices != output->num_voices) {
      return false;
    }
    connection = output;
    return true;
  }

  void Disconnect() {
    connection = nullptr;
  }
  
  bool IsConnected(std::shared_ptr<Output> output) const {
    if (!connection) {
      return false;
    }

    return connection.get() == output.get();
  }
  
  bool IsConnected() const {
    return bool(connection);
  }
};

using InputPtr = std::shared_ptr<Input>;
using OutputPtr = std::shared_ptr<Output>;


class Node {
 public:
  Node(const NodeParams& ctx) 
    : num_samples(ctx.num_samples)
    , num_voices(ctx.num_voices)
  { }

  virtual ~Node() = default;
  
  InputPtr GetInputByIndex(size_t index) {
    ASSERT(index < inputs.size());
    return inputs[index];
  }

  OutputPtr GetOutputByIndex(size_t index) {
    ASSERT(index < outputs.size());
    return outputs[index];
  }
  
  size_t NumInputs() const {
    return inputs.size();
  }

  size_t NumOutputs() const {
    return outputs.size();
  }
  
  int NumVoices() const {
    return num_voices;
  }
  
  const std::string& GetDisplayName() const {
    return display_name;
  }
  
  NodeType GetType() const {
    return type;
  }

  void Process(int voice_idx, int sample_idx, float time) {
    _active_voice = voice_idx;
    _active_sample = sample_idx;
    Process(time);
  }
  
  virtual void Process(float time) = 0;
  virtual void Draw() {}
  
  virtual void Load(const nlohmann::json& j) {};
  virtual void Save(nlohmann::json& j) const {};

 protected:
  void AddInput(const std::string& name, PinDataType dtype, PinData default_value, bool mono = false) {
    int _num_voices = mono ? 1 : num_voices;
    inputs.push_back(std::make_shared<Input>(name, dtype, this, default_value, _num_voices));
  }

  void AddOutput(const std::string& name, PinDataType dtype, PinData default_value, bool mono = false) {
    int _num_voices = mono ? 1 : num_voices;
    outputs.push_back(std::make_shared<Output>(name, dtype, this, default_value, num_samples, _num_voices));
  }

  int GetActiveSample() const {
    return _active_sample;
  }

  int GetActiveVoice() const {
    return _active_voice;
  }
  
  template <typename T>
  const T& GetInputValue(int input_idx) {
    return inputs[input_idx]->GetValue<T>(_active_voice, _active_sample);
  }

  template <typename T>
  void SetOutputValue(int output_idx, const T& new_value) {
    return outputs[output_idx]->SetValue<T>(_active_voice, _active_sample, new_value);
  }
  
  template <typename T>
  T& GetOutputValue(int output_idx) {
    return outputs[output_idx]->GetValue<T>(_active_voice, _active_sample);
  }

  const int num_samples;
  const int num_voices;
  
  int _active_sample = 0;
  int _active_voice = 0;

  std::string display_name;
  NodeType type;
  std::size_t last_update = -1;

  std::vector<InputPtr> inputs;
  std::vector<OutputPtr> outputs;
};