#pragma once

#include <vector>
#include <map>
#include <set>
#include <mutex>

#include "node.h"
#include "node_factory.h"
#include "util.h"

#include "json.hpp"

template <typename T>
struct Access {
  T* obj;
  std::lock_guard<std::mutex> lock;
  
  T* operator-> () {
    return obj;
  }
};

enum class PinType { kInput, kOutput };

// Crutch to have "unique" ids for nodes, links and pins
const int kPinIdOffset = 100000;
const int kLinkIdOffset = 200000;

using node_id_t = int;
using link_id_t = int;
using pin_id_t = int;


struct PinInfo {
  PinType type;
  node_id_t node_id;
  int node_io_id;  // Sequential input or output id
};

struct NodePins {
  std::vector<pin_id_t> inputs;
  std::vector<pin_id_t> outputs;
};

// External attributes not related to node functionality
struct NodeAttributes {
  float pos_x = 0.0f, pos_y = 0.0f;
  bool is_placed = false;
  
  void Save(nlohmann::json& j) const {
    JsonSetValue(j, "x", pos_x);
    JsonSetValue(j, "y", pos_y);
  }

  void Load(const nlohmann::json& j) {
    JsonGetValue(j, "x", pos_x);
    JsonGetValue(j, "y", pos_y);
  }
};

struct NodeWrapper {
  NodePtr node;
  std::shared_ptr<NodeAttributes> attrs;
};

using Nodes = std::map<node_id_t, NodeWrapper>;

struct Pins {
  void CreatePins(const NodePtr& node, node_id_t node_id) {
    ASSERT(!node_to_pins.contains(node_id));
    auto& node_pins = node_to_pins[node_id];

    for (int i = 0; i < node->NumInputs(); ++i) {
      int pin_id = pin_counter++;
      MapInsert(pin_info, pin_id, PinInfo{
        .type = PinType::kInput,
        .node_id = node_id,
        .node_io_id = i});
      node_pins.inputs.push_back(pin_id);
    }

    for (int i = 0; i < node->NumOutputs(); ++i) {
      int pin_id = pin_counter++;
      MapInsert(pin_info, pin_id, PinInfo{
        .type = PinType::kOutput,
        .node_id = node_id,
        .node_io_id = i});
      node_pins.outputs.push_back(pin_id);
    }
  }
  
  void RemoveNodePins(node_id_t node_id) {
    auto& node_pins = MapGetRef(node_to_pins, node_id);
    for (auto pin_id : node_pins.inputs) {
      MapErase(pin_info, pin_id);
    }

    for (auto pin_id : node_pins.outputs) {
      MapErase(pin_info, pin_id);
    }
    
    MapErase(node_to_pins, node_id);
  }
  
  const PinInfo* GetPinById(pin_id_t pin_id) const {
    return &MapGetConstRef(pin_info, pin_id);
  }
  
  node_id_t GetNodeFromPin(pin_id_t pin_id) const {
    auto& info = MapGetConstRef(pin_info, pin_id);
    return info.node_id;
  }

  std::map<pin_id_t, PinInfo> pin_info;  
  std::map<node_id_t, NodePins> node_to_pins;
  
 private:
  pin_id_t pin_counter = kPinIdOffset;
};

struct Links {
  Links(const Pins* pins) : pins(pins) { }
  
  bool CanAddLink(pin_id_t pin_src, pin_id_t pin_dst) {
    return AddLink(pin_src, pin_dst, nullptr, /*commit=*/false);
  }

  bool AddLink(pin_id_t pin_src, pin_id_t pin_dst, link_id_t* new_link_id, bool commit=true) {
    auto pin_pair = std::make_pair(pin_src, pin_dst);
    REQ_CHECK_EX(!pins_to_link_id.contains(pin_pair), "Pin pair already exists");

    int node_src = pins->GetNodeFromPin(pin_src);
    int node_dst = pins->GetNodeFromPin(pin_dst);

    REQ_CHECK(node_src != node_dst);
    if (!commit) {
      return true;
    }

    // Commit
    link_id_t id = link_counter++;
    pins_to_link_id[pin_pair] = id;
    link_id_to_pins[id] = pin_pair;

    node_links[node_src].insert(id);
    node_links[node_dst].insert(id);

    *new_link_id = id;
    return true;
  }
  
  void RemoveLink(link_id_t link_id) {
    auto link_pins = MapGetRef(link_id_to_pins, link_id);

    MapErase(pins_to_link_id, link_pins);
    MapErase(link_id_to_pins, link_id);
    
    // Find node ids for src and dst pins
    node_id_t node_src = pins->GetNodeFromPin(link_pins.first);
    node_id_t node_dst = pins->GetNodeFromPin(link_pins.second);
    
    SetErase(node_links[node_src], link_id);
    SetErase(node_links[node_dst], link_id);
  }
  
  bool LinkExists(int pin_src, int pin_dst) const {
    auto pin_pair = std::make_pair(pin_src, pin_dst);
    return pins_to_link_id.contains(pin_pair);
  }

  std::map<link_id_t, std::pair<pin_id_t, pin_id_t>> link_id_to_pins;  
  std::map<std::pair<pin_id_t, pin_id_t>, link_id_t> pins_to_link_id;
  std::map<node_id_t, std::set<link_id_t>> node_links;  // Any links associated with node
  
 private:
  link_id_t link_counter = kLinkIdOffset;
  const Pins* pins;
};


class Multigraph {
 public:
  Multigraph() : links(&pins) { }
  
  int AddNode(NodeWrapper wrapper) {
    int new_id = id_counter++;
    ASSERT(!nodes.contains(new_id));
    nodes[new_id] = wrapper;
    pins.CreatePins(wrapper.node, new_id);
    SortNodes();
    return new_id;
  }
  
  bool CanAddLink(int pin_id_src, int pin_id_dst) {
    return AddLink(pin_id_src, pin_id_dst, nullptr, false);
  }
  
  bool AddLink(int pin_id_src, int pin_id_dst, int* new_link_id, bool commit=true) {
    REQ_CHECK_EX(!links.LinkExists(pin_id_src, pin_id_dst), "Link exists");

    auto pin_src = pins.GetPinById(pin_id_src);
    auto pin_dst = pins.GetPinById(pin_id_dst);
    
    REQ_CHECK_EX(pin_src->type == PinType::kOutput && pin_dst->type == PinType::kInput, "AddLink: Pin type");
    
    auto node_src = GetNodeById(pin_src->node_id);
    auto node_dst = GetNodeById(pin_dst->node_id);
    
    auto src_out = node_src->GetOutputByIndex(pin_src->node_io_id);
    auto dst_in  = node_dst->GetInputByIndex(pin_dst->node_io_id);
    
    REQ_CHECK_EX(src_out->type == dst_in->type, "AddLink: Type mismatch");
    
    // Additional requirement: input can only have one link
    REQ_CHECK_EX(!dst_in->IsConnected(), "Already connected");
    REQ_CHECK_EX(links.AddLink(pin_id_src, pin_id_dst, new_link_id, commit), "Failed to add link");
    
    if (!commit) {
      return true;
    }

    dst_in->Connect(src_out);
    SortNodes();
    return true;
  }
  
  bool AddLink(node_id_t node_id_src, int out_idx_src, node_id_t node_id_dst, int in_idx_dst, int* new_link_id, bool commit) {
    REQ_CHECK(node_id_src != node_id_dst);
    auto& pins_src = MapGetRef(pins.node_to_pins, node_id_src);
    auto& pins_dst = MapGetRef(pins.node_to_pins, node_id_dst);
    ASSERT(out_idx_src < pins_src.outputs.size());
    ASSERT(in_idx_dst < pins_dst.inputs.size());
    
    return AddLink(pins_src.outputs[out_idx_src], pins_dst.inputs[in_idx_dst], new_link_id, commit);
  }
  
  void RemoveNode(int node_id) {
    ASSERT(nodes.contains(node_id));
    std::set<int> node_links = links.node_links[node_id];
    for (auto link_id : node_links) {
      RemoveLink(link_id);
    }

    links.node_links.erase(node_id);
    pins.RemoveNodePins(node_id);
    nodes.erase(node_id);
    SortNodes();
  }
  
  void RemoveLink(int link_id) {
    auto& link_pins = MapGetRef(links.link_id_to_pins, link_id);
    auto dst_pin = pins.GetPinById(link_pins.second);

    auto dst_node = GetNodeById(dst_pin->node_id);
    auto dst_input = dst_node->GetInputByIndex(dst_pin->node_io_id);

    dst_input->Disconnect();
    links.RemoveLink(link_id);
    SortNodes();
  }

  NodePtr& GetNodeById(int node_id) {
    return MapGetRef(nodes, node_id).node;
  }
  
  const Nodes& GetNodes() const { return nodes; }

  const Pins& GetPins() const { return pins; }

  const Links& GetLinks() const { return links; }
  
  auto& GetSortedNodes() { return nodes_ordered; }
  
  // Used for concurrent ops between GUI and audio thread
  Access<Multigraph> GetAccess() {
    return {this, std::lock_guard(_mtx)};
  }

 private:
  void SortNodes();

  std::vector<Node*> nodes_ordered;  // Ordered for processing

  Nodes nodes;  // Node id to node
  Pins pins;
  Links links;

  int id_counter = 1;
  
  mutable std::mutex _mtx;
};


void SaveGraph(const Multigraph& g, nlohmann::json& j);
void LoadGraph(Multigraph& g, const nlohmann::json& j, const NodeFactory& factory);