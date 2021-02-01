#include "node_graph.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>
#include <stack>

using Edges = std::vector<std::vector<int>>;
std::vector<int> TopologicalSort(int num_vertices, const Edges& edges) {
  std::vector<std::uint8_t> visited(num_vertices, 0);
  
  // false == regular node, true == finished processing children for this node.
  std::stack<std::pair<int, bool>> nodes;
  std::vector<int> order;
  order.reserve(num_vertices);

  for (int i = 0; i < num_vertices; ++i) {
    if (visited[i]) {
      continue;
    }

    nodes.push(std::make_pair(i, false));

    while (!nodes.empty()) {
      auto [s, flag] = nodes.top();
      nodes.pop();
      
      if (flag) {
        order.push_back(s);
        continue;
      }

      if (!visited[s]) {
        visited[s] = true;
      }
      
      nodes.push(std::make_pair(s, true));
      for (const auto& adjacent : edges[s]) {
        if (!visited[adjacent]) {
          nodes.push(std::make_pair(adjacent, false));
        }
      }
    }
  }
  
  std::reverse(order.begin(), order.end());
  return order;
}

void NodeGraph::SortNodes() {
  std::map<Node*, int> node_index;
  Edges edges(nodes_.size());
  
  int index = 0;

  for (auto& node_ptr : nodes_) {
    node_index[node_ptr.get()] = index++;
  }
    
  // Fill edges
  for (auto& node_ptr : nodes_) {
    int right_idx = node_index[node_ptr.get()];
    for (size_t input_idx = 0; input_idx < node_ptr->NumInputs(); ++input_idx) {
      auto input = node_ptr->GetInputByIndex(input_idx);
      if (input->connection) {
        Node* left_node = input->connection->parent;
        int left_idx = node_index[left_node];
        edges[left_idx].push_back(right_idx);
      }
    }
  }
  
  // Topological sort
  auto order = TopologicalSort(nodes_.size(), edges);
  
  std::vector<NodePtr> reordered;
  reordered.reserve(nodes_.size());
  
  for (int i : order) {
    reordered.push_back(nodes_[i]);
  }
  
  nodes_ = reordered;
}