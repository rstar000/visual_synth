
#pragma once

#include <iostream>
#include <memory>
#include <map>
#include <optional>

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

struct Bridge;

using BridgePtr = std::shared_ptr<Bridge>;

struct NodePins {
  std::vector<int> input_ids;
  std::vector<int> output_ids;
  
  std::map<int, int> input_pin_to_input_idx;
  std::map<int, int> output_pin_to_output_idx;
};

struct Position {
  float x, y;
};

struct NodeAttributes {
  NodePtr ptr;
  NodePins pins;
  std::optional<Position> position;
};

struct Bridge {
  std::shared_ptr<Tracker> tracker;  // Needed for input node
  std::shared_ptr<NodeGraph> node_graph;

  NodePtr CreateNode(NodeType node_type);
  
  NodePins CreatePins(NodePtr node, int node_id) {
    NodePins pins;
    for (int i = 0; i < node->NumInputs(); ++i) {
      pins.input_ids.push_back(++counter);
      pins.input_pin_to_input_idx[counter] = i;
      pin_id_to_node_id[counter] = node_id;
    }

    for (int i = 0; i < node->NumOutputs(); ++i) {
      pins.output_ids.push_back(++counter);
      pins.output_pin_to_output_idx[counter] = i;
      pin_id_to_node_id[counter] = node_id;
    }
    return pins;
  }

  int AddNode(NodePtr new_node) {
    node_graph->AddNode(new_node);
    int node_id = ++counter;
    
    auto& attrs = node_attrs[node_id];
    attrs.ptr = new_node;
    attrs.pins = CreatePins(new_node, node_id);
    return node_id;
  }  
  
  void RemoveNode(int node_id) {
    auto ptr = node_attrs[node_id].ptr;
    node_attrs.erase(node_id);
    node_graph->RemoveNode(ptr);
  }
  
  bool CanAddLink(int pin_from, int pin_to) {
    // TODO
    return true;
  }
  
  int AddLink(int pin_from, int pin_to) {
    int node_id_from = pin_id_to_node_id[pin_from];
    int node_id_to = pin_id_to_node_id[pin_to];
    
    auto& attrs_from = node_attrs[node_id_from];
    auto& attrs_to = node_attrs[node_id_to];

    node_graph->AddConnection(
      attrs_from.ptr, 
      attrs_from.pins.output_pin_to_output_idx[pin_from], 
      attrs_to.ptr, 
      attrs_to.pins.input_pin_to_input_idx[pin_to]);
    
    int link_id = ++counter;
    link_id_to_pins[link_id] = std::make_pair(pin_from, pin_to);
    return link_id;
  }
  
  void RemoveLink(int link_id) {
    auto [pin_from, pin_to] = link_id_to_pins[link_id];
    int node_id_to = pin_id_to_node_id[pin_to];
    auto& attrs_to = node_attrs[node_id_to];
    auto node_to = attrs_to.ptr;
    int input_idx = attrs_to.pins.input_pin_to_input_idx[pin_to];
    node_to->GetInputByIndex(input_idx)->Disconnect();
    link_id_to_pins.erase(link_id);
  }
    
  std::map<int, NodeAttributes> node_attrs;
  std::map<int, int> pin_id_to_node_id;
  std::map<int, std::pair<int, int>> link_id_to_pins;
  
  int counter = 1;
};