
#pragma once
#include <iostream>

#include <map>

#include "tracker.h"
#include "output.h"
#include "node_graph.h"
#include "waveform.h"
#include "gui.h"

enum class NodeType {
  kKeyboardTracker,
  kTrackerUnpack,
  kSine,
  kOutput,
  kADTR,
  kMix
};

struct NodePins {
  std::vector<int> input_ids;
  std::vector<int> output_ids;
  
  std::map<int, int> input_pin_to_input_idx;
  std::map<int, int> output_pin_to_output_idx;
}

struct Bridge {
  std::shared_ptr<Tracker> tracker;  // Needed for input node
  std::shared_ptr<SampleWriter> writer;  // Needed for output node
  std::shared_ptr<NodeGraph> node_graph;

  NodePtr CreateNode(NodeType node_type) {
    
  }
  
  NodePins CreatePins(NodePtr node, int node_id) {
    NodePins pins;
    for (int i = 0; i < node->NumInputs(); ++i) {
      pins.input_ids.push_back(++pin_counter);
      pins.input_pin_to_input_idx[pin_counter] = i;
      pin_id_to_node_id[pin_counter] = node_id;
    }

    for (int i = 0; i < node->NumOutputs(); ++i) {
      pins.output_ids.push_back(++pin_counter);
      pins.output_pin_to_output_idx[pin_counter] = i;
      pin_id_to_node_id[pin_counter] = node_id;
    }
    return pins;
  }

  int AddNode(NodePtr new_node) {
    node_graph->AddNode(new_node);
    int node_id = ++node_counter;
    
    node_id_to_ptr[node_id] = new_node;
    node_id_to_pins[node_id] = CreatePins(new_node);
    return node_id;
  }  
  
  void RemoveNode(int node_id) {
    auto node = node_id_to_ptr[node_id];
    node_id_to_pins.erase(node_id);
    node_id_to_ptr.erase(node_id);
    
    node_graph->RemoveNode(node);
  }
  
  bool IsLinkValid(int pin_from, int pin_to) {
    // TODO
    return true;
  }
  
  int AddLink(int pin_from, int pin_to) {
    int node_id_from = pin_id_to_node_id[pin_from];
    int node_id_to = pin_id_to_node_id[pin_to];
    
    auto node_from = node_id_to_ptr[node_id_from];
    auto node_to = node_id_to_ptr[node_id_to];
    
    auto& pins_from = node_id_to_pins[node_id_from];
    auto& pins_to = node_id_to_pins[node_id_to];
    
    node_graph->AddConnection(
      node_from, 
      pins_from.output_pin_to_output_idx[pin_from], 
      node_to, 
      pins_to.input_pin_to_input_idx[pin_to]);
    
    int link_id = ++link_counter;
    link_id_to_pins[link_id] = std::make_pair(pin_from, pin_to);
  }
  
    
  std::map<int, NodePins> node_id_to_pins;
  std::map<int, NodePtr> node_id_to_ptr;
  std::map<int, int> pin_id_to_node_id;
  
  std::map<int, std::pair<int, int>> link_id_to_pins;
  
  int node_counter = 0;
  int pin_counter = 0;
  int link_counter = 0;
};

