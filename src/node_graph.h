#pragma once

#include <vector>
#include "node.h"
#include "tracker.h"
#include "output.h"

using Edge = std::pair<std::size_t, std::size_t>;

class NodeGraph {
 public:
  NodeGraph() {}

  void AddNode(NodePtr node) {
    nodes_.push_back(std::move(node));
  }
  
  void RemoveNode(NodePtr node) {
    // TODO
  }

  float Run(float time) {
    for (auto& node : nodes_) {
      node->Process(time);
    }
    
    return output_;
  }

  void AddConnection(NodePtr from, size_t output_idx, NodePtr to, size_t input_idx) {
    auto output = from->GetOutputByIndex(output_idx);
    auto input = to->GetInputByIndex(input_idx);
    input->Connect(output);
    
    SortNodes();
  }

  void RemoveConnection(NodePtr from, size_t output_idx, NodePtr to, size_t input_idx) {
    auto output = from->GetOutputByIndex(output_idx);
    auto input = to->GetInputByIndex(input_idx);
    if (input->IsConnected(output)) {
      input->Disconnect();
    } else {
      std::cout << output->name << " and " << input->name << " not connected!";
    }
  }
  
  void SetOutput(float output) {
    output_ = output;
  }

 private:
  void SortNodes() {

  }

  std::vector<std::shared_ptr<Node>> nodes_;
  std::vector<Edge> edges_;
  float output_;
};

class OutputNode : public Node {
 public:
  OutputNode(std::shared_ptr<NodeGraph> graph) 
      : graph_(graph) {
    auto signal = std::make_shared<Input>("signal", Dt::kFloat, this, 0.0f);
    inputs = {signal};
  }

 protected:
  void Process(float time) override {
    float signal = inputs[0]->GetValue<float>();
    graph_->SetOutput(signal);
  }
  
  std::shared_ptr<NodeGraph> graph_;
};