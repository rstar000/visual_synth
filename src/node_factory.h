#pragma once

#include <functional>
#include <map>

#include "node.h"
#include "node_types.h"
#include "output.h"

struct BaseFactoryFunc {
    virtual void Construct(NodeParams const& ctx, NodePtr* out) const = 0;
    virtual ~BaseFactoryFunc() = default;
};

template <typename T>
struct ConcreteFactoryFunc : BaseFactoryFunc {
    void Construct(NodeParams const& ctx, NodePtr* out) const override {
        *out = std::make_unique<T>(ctx);
    }
    ~ConcreteFactoryFunc() = default;
};

struct NodeFactory {
    NodeFactory(NodeParams const& ctx) : ctx(ctx) {
        RegisterCategories();
        RegisterNodes();
    };

    NodePtr CreateNode(NodeType const& type) const {
        SPDLOG_INFO("[NodeFactory] CreateNode: {}", display_names.at(type));
        auto& func = MapGetConstRef(factory, type);
        NodePtr ret;
        func->Construct(ctx, &ret);
        return ret;
    }

    NodePtr CreateNodeByName(std::string const& name) const {
        return CreateNode(names.GetType(name));
    }

    const auto& GetDisplayNames() const { return display_names; }
    const auto& GetNodesByCategory() const { return nodes_by_category; }
    const auto& GetCategoryNames() const { return category_names; }
    void DumpNodes(std::string const& filename) const;

   private:
    void RegisterCategories();
    void RegisterNodes();

    template <typename T>
    void RegisterNode(NodeCategory category) {
        factory[T::TYPE] = std::make_unique<ConcreteFactoryFunc<T>>();
        display_names[T::TYPE] = T::DISPLAY_NAME;
        nodes_by_category[category].push_back(T::TYPE);
    }

    std::map<NodeType, std::unique_ptr<BaseFactoryFunc>> factory;
    std::map<NodeType, std::string> display_names;
    std::map<NodeCategory, std::string> category_names;
    std::map<NodeCategory, std::vector<NodeType>> nodes_by_category;

    NodeParams ctx;
    NodeNames names;
};
