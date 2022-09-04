#pragma once

#include <vector>
#include <variant>
#include <iostream>
#include <optional>
#include <memory>
#include <string>
#include <map>

#include "note.h"
#include "node_types.h"
#include "util.h"

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
  Output(const std::string& name, PinDataType type, Node* parent, PinData default_value)
      : Connection(name, type, parent)
      , value(default_value) { }
  PinData value;

  template <typename T> 
  T GetValue() const {
    const T* t_ptr = std::get_if<T>(&value);
    ASSERT(t_ptr);
    return *t_ptr;
  }

  template <typename T> 
  T& GetValue() {
    T* t_ptr = std::get_if<T>(&value);
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
  Input(const std::string& name, PinDataType type, Node* parent, PinData default_value)
      : Connection(name, type, parent)
      , default_value(default_value) { 
  }

  std::shared_ptr<Output> connection;
  PinData default_value;

  template <typename T>
  T GetValue() const {
    if (connection && connection->IsT<T>()) {
      return connection->GetValue<T>();
    }
    return std::get<T>(default_value);
  }

  bool Connect(std::shared_ptr<Output> output) {
    // TODO: after debugging replace asserts with warnings.
    if (parent == output->parent || type != output->type) {
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
  Node() = default;
  virtual ~Node() = default;

  void Update(std::size_t timestamp) {
    ASSERT(last_update <= timestamp);
    if (last_update != timestamp) {
      Process(timestamp);
    }
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
  
  const std::string& GetDisplayName() const {
    return display_name;
  }
  
  NodeType GetType() const {
    return type;
  }

  virtual void Process(float time) = 0;
  virtual void Draw() {}
  
  virtual void Load(const nlohmann::json& j) {};
  virtual void Save(nlohmann::json& j) const {};

 protected:
  std::string display_name;
  NodeType type;
  std::size_t last_update = -1;

  std::vector<InputPtr> inputs;
  std::vector<OutputPtr> outputs;
};