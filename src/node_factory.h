#pragma once

#include <functional>
#include <map>

#include "node.h"
#include "node_types.h"
#include "output.h"


enum class NodeCategory {
  OCSILLATOR,
  SEQUENCER,
  IO,
  UTILITY,
  ARITHMETIC,
  DEBUG
};

struct NodeFactory {
  NodeFactory(const Context& ctx) : ctx(ctx) { 
    RegisterCategories();
    RegisterNodes();
  };

  NodePtr CreateNode(const NodeType& type) const {
    auto& func = MapGetConstRef(factory, type);
    return func();
  }
  
  void RegisterCategories();
  void RegisterNodes();

  const auto& GetDisplayNames() const {
    return display_names;
  }
  
  const auto& GetNodesByCategory() const {
    return nodes_by_category;
  }
  
  const auto& GetCategoryNames() const {
    return category_names;
  }
  
 private:
  template <typename T>
  void RegisterNode(NodeCategory category) {
    factory[T::TYPE] = [this] () -> NodePtr {
      return std::make_shared<T>(ctx);
    };
    display_names[T::TYPE] = T::DISPLAY_NAME;
    nodes_by_category[category].push_back(T::TYPE);
  }


  std::map<NodeType, std::function<NodePtr()>> factory;
  std::map<NodeType, std::string> display_names;
  std::map<NodeCategory, std::string> category_names;
  std::map<NodeCategory, std::vector<NodeType>> nodes_by_category;

  Context ctx;
  NodeNames names;
};