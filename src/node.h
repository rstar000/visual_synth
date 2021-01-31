#pragma once
#include <vector>
#include <variant>
#include <iostream>
#include <optional>
#include <memory>
#include <string>
#include <map>

#include "note.h"
#include "util.h"

// Dot type
enum class Dt {
  kInt,
  kFloat,
  kTimestamp,
  kChannel
};

using ValueT = std::variant<
  int, 
  float,
  std::size_t,
  Channel
>;

class Node;
using NodePtr = std::shared_ptr<Node>;


struct Connection {
  Connection(const std::string& name, Dt type, Node* parent)
    : name(name), type(type), parent(parent) { }
  std::string name;
  Dt type;
  Node* parent = nullptr;
};

// Doesn't store reference to its connections
// cause there could be multiple connected inputs.
struct Output : public Connection {
  Output(const std::string& name, Dt type, Node* parent, ValueT default_value)
      : Connection(name, type, parent)
      , value(default_value) { }
  ValueT value;

  template <typename T> 
  T GetValue() const {
    const T* t_ptr = std::get_if<T>(&value);
    ASSERT(t_ptr);
    return *t_ptr;
  }

  template <typename T>
  void SetValue(T t) {
    value = t;
  }
  
  template <typename T>
  bool IsT() const {
    return std::holds_alternative<T>(value);
  }
};

struct Input : public Connection {
  Input(const std::string& name, Dt type, Node* parent, ValueT default_value)
      : Connection(name, type, parent)
      , default_value(default_value) { 
  }

  std::shared_ptr<Output> connection;
  ValueT default_value;

  template <typename T>
  T GetValue() const {
    if (connection && connection->IsT<T>()) {
      return connection->GetValue<T>();
    }
    return std::get<T>(default_value);
  }

  void Connect(std::shared_ptr<Output> output) {
    // TODO: after debugging replace asserts with warnings.
    ASSERT(parent != output->parent);
    ASSERT(type == output->type);
    connection = output;
  }

  void Disconnect() {
    connection = nullptr;
  }
  
  bool IsConnected(OutputPtr output) {
    if (!connection) {
      return false;
    }

    return connection.get() == output.get();
  }
};

using InputPtr = std::shared_ptr<Input>;
using OutputPtr = std::shared_ptr<Output>;


// Input values are required at all times
// Output values can have defaults set up.
// However, types must match.
// The Process method may use any inputs and update any outputs
class Node {
 public:
  Node() = default;

  void Update(std::size_t timestamp) {
    ASSERT(last_update <= timestamp);
    if (last_update != timestamp) {
      Process(timestamp);
    }
  }

  // JSON string with dynamic params
  void SetParams(const std::string& json_param) {
    // params = nlohmann::json::parse(json_param);
  }

  // Checks that a node has all inputs connected.
  // Subclasses may implement a stricted check
  virtual bool Validate() const {
    for (auto& input : inputs) {
      if (!input->connection) {
        return false;
      }
    }

    return true;
  }
  
  InputPtr GetInputByName(const std::string& name) {
    for (auto& input : inputs) {
      if (input->name == name) {
        return input;
      }
    }

    std::cout << "Input " << name << " not found!" << std::endl; 
    return nullptr;
  }

  OutputPtr GetOutputByName(const std::string& name) {
    for (auto& output : outputs) {
      if (output->name == name) {
        return output;
      }
    }
    
    std::cout << "Output " << name << " not found!" << std::endl; 
    return nullptr;
  }
  
  InputPtr GetInputByIndex(size_t index) {
    ASSERT(i < inputs.size());
    return inputs[i];
  }

  OutputPtr GetOutputByIndex(size_t index) {
    ASSERT(i < outputs.size());
    return outputs[i];
  }
  
  size_t NumInputs() const {
    return inputs.size();
  }

  size_t NumOutputs() const {
    return outputs.size();
  }

  virtual void Process(float time) = 0;
 protected:

  std::string name;
  // nlohmann::json params;
  std::size_t last_update = -1;

  std::vector<InputPtr> inputs;
  std::vector<OutputPtr> outputs;
};