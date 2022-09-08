#include "multigraph.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>
#include <stack>
#include <deque>

using Edges = std::vector<std::set<int>>;

std::vector<int> TopologicalSort(int num_vertices, Edges edges) {
  std::vector<std::uint8_t> visited(num_vertices, 0);
  Edges incoming(num_vertices);
  for (int n = 0; n < num_vertices; ++n) {
    for (int m : edges[n]) {
      incoming[m].insert(n);
    }
  }
  
  std::deque<int> no_incoming;
  std::vector<int> order;
  for (int m = 0; m < num_vertices; ++m) {
    if (incoming[m].empty()) {
      no_incoming.push_back(m);
    }
  }
  
  while (!no_incoming.empty()) {
    int n = no_incoming.front();
    no_incoming.pop_front();
    order.push_back(n);
    
    auto edges_n = edges[n];
    for (int m : edges_n) {
      // Erase n->m
      edges[n].erase(m);
      incoming[m].erase(n);

      if (incoming[m].empty()) {
        no_incoming.push_back(m);
      }
    }
  }
  
  return order;
}


void Multigraph::SortNodes() {
  // Map nodes to simple 1->N index
  std::map<Node*, int> node_index;
  std::map<Node*, int> node_to_graph_index;
  std::vector<Node*> nodes_list;
  int index = 0;
  nodes_list.reserve(nodes.size());
  for (auto& [id, wrapper] : nodes) {
    Node* node_ptr = wrapper.node.get();
    node_index[node_ptr] = index++;
    node_to_graph_index[node_ptr] = id;
    nodes_list.push_back(node_ptr);
  }
    
  // Fill edges
  Edges edges(nodes.size());
  for (auto& [_, wrapper] : nodes) {
    Node* node_ptr = wrapper.node.get();
    int right_idx = node_index[node_ptr];
    for (size_t input_idx = 0; input_idx < node_ptr->NumInputs(); ++input_idx) {
      auto input = node_ptr->GetInputByIndex(input_idx);
      if (input->connection) {
        Node* left_node = input->connection->parent;
        int left_idx = node_index[left_node];
        edges[left_idx].insert(right_idx);
      }
    }
  }
  
  // Topological sort
  auto order = TopologicalSort(nodes.size(), edges);
  
  nodes_ordered.resize(nodes.size());
  std::cout << "Order: [ ";
  for (int i = 0; i < nodes_list.size(); ++i) {
    nodes_ordered[i] = nodes_list[order[i]];
    std::cout << node_to_graph_index[nodes_list[order[i]]] << ", ";
  }
  std::cout << " ]" << std::endl;
}

void SaveGraph(const Multigraph& g, nlohmann::json& j) {
  NodeNames names;
  using namespace nlohmann;
  auto& j_nodes = j["nodes"] = json::array();
  auto& j_links = j["links"] = json::array();
  
  auto& pins = g.GetPins();
  auto& links = g.GetLinks();

  for (auto& [node_id, wrapper] : g.GetNodes()) {
    auto j_node = json::object();
    j_node["id"] = node_id;
    j_node["type"] = names.GetName(wrapper.node->GetType());
    j_node["params"] = json::object();
    j_node["attributes"] = json::object();
    wrapper.node->Save(j_node["params"]);
    wrapper.attrs->Save(j_node["attributes"]);
    j_nodes.push_back(j_node);
  }
  
  for (auto& [link_id, link_pins] : links.link_id_to_pins) {
    auto pin_src = pins.GetPinById(link_pins.first);
    auto pin_dst = pins.GetPinById(link_pins.second);
    
    j_links.push_back(json::array({
      pin_src->node_id, pin_src->node_io_id, 
      pin_dst->node_id, pin_dst->node_io_id}));
  }
}

void LoadGraph(Multigraph& g, const nlohmann::json& j, const NodeFactory& factory) {
  NodeNames names;

  std::map<int, int> node_old_to_new;

  auto& j_nodes = JsonGetConstRef(j, "nodes");
  ASSERT(j_nodes.is_array());

  // Create nodes
  for (auto& j_node : j_nodes) {
    int old_id = JsonGetValue<int>(j_node, "id");
    std::string type_str = JsonGetValue<std::string>(j_node, "type");
    
    NodeWrapper wrapper;
    wrapper.node = factory.CreateNode(names.GetType(type_str));
    wrapper.node->Load(JsonGetConstRef(j_node, "params"));
    wrapper.attrs = std::make_shared<NodeAttributes>();
    wrapper.attrs->is_placed = false;
    wrapper.attrs->Load(JsonGetConstRef(j_node, "attributes"));

    int new_id = g.AddNode(wrapper);
    node_old_to_new[old_id] = new_id;
  }
  
  auto& j_links = JsonGetConstRef(j, "links");
  ASSERT(j_nodes.is_array());
  for (auto& j_link : j_links) {
    ASSERT(j_link.is_array());
    int new_link_id = 0;
    g.AddLink(
      /*src*/MapGetRef(node_old_to_new, j_link.at(0).get<int>()),
      /*out_idx*/j_link.at(1).get<int>(),
      /*dst*/MapGetRef(node_old_to_new, j_link.at(2).get<int>()),
      /*in_idx*/j_link.at(3).get<int>(),
      &new_link_id,
      /*commit=*/true
    );
  }
}