#include "multigraph.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <map>
#include <stack>
#include <vector>
#include <fstream>

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
    nodes_list.reserve(m_nodes.size());
    for (auto& [id, wrapper] : m_nodes) {
        Node* node_ptr = wrapper.node.get();
        node_index[node_ptr] = index++;
        node_to_graph_index[node_ptr] = id;
        nodes_list.push_back(node_ptr);
    }

    // Fill edges
    Edges edges(m_nodes.size());
    for (auto& [_, wrapper] : m_nodes) {
        Node* node_ptr = wrapper.node.get();
        int right_idx = node_index[node_ptr];
        for (size_t input_idx = 0; input_idx < node_ptr->NumInputs();
             ++input_idx) {
            auto input = node_ptr->GetInputByIndex(input_idx);
            if (input->connection) {
                Node* left_node = input->connection->parent;
                int left_idx = node_index[left_node];
                edges[left_idx].insert(right_idx);
            }
        }
    }

    // Topological sort
    auto order = TopologicalSort(m_nodes.size(), edges);

    m_nodesOrdered.resize(m_nodes.size());
    // std::cout << "Order: [ ";
    for (int i = 0; i < nodes_list.size(); ++i) {
        m_nodesOrdered[i] = nodes_list[order[i]];
    //     std::cout << node_to_graph_index[nodes_list[order[i]]] << ", ";
    }
    // std::cout << " ]" << std::endl;
}

bool Multigraph::AddLink(pin_id_t srcPinId, pin_id_t dstPinId, link_id_t* newLinkId,
                bool commit) {
    REQ_CHECK_EX(!m_links.LinkExists(srcPinId, dstPinId), "Link exists");

    auto pin_src = m_pins.GetPinById(srcPinId);
    auto pin_dst = m_pins.GetPinById(dstPinId);

    REQ_CHECK_EX(pin_src->type == PinType::kOutput &&
                        pin_dst->type == PinType::kInput,
                    "AddLink: Pin type");

    auto node_src = GetNodeById(pin_src->node_id);
    auto node_dst = GetNodeById(pin_dst->node_id);

    auto src_out = node_src->GetOutputByIndex(pin_src->node_io_id);
    auto dst_in = node_dst->GetInputByIndex(pin_dst->node_io_id);

    REQ_CHECK_EX(src_out->type == dst_in->type, "AddLink: Type mismatch");

    // Additional requirement: input can only have one link
    if (dst_in->IsConnected()) {
        SPDLOG_ERROR("Node {} already has input {} connected", MapGetRef(m_nodes, pin_dst->node_id).attrs.display_name, pin_dst->node_io_id);
    }
    REQ_CHECK_EX(!dst_in->IsConnected(), "Already connected");
    REQ_CHECK_EX(m_links.AddLink(srcPinId, dstPinId, newLinkId, commit),
                    "Failed to add link");

    if (!commit) {
        return true;
    }

    dst_in->Connect(src_out);
    SortNodes();
    return true;
}

bool Multigraph::AddLink(node_id_t srcNodeId, int srcOutIdx, node_id_t dstNodeId,
                int dstInIdx, link_id_t* newLinkId, bool commit) {
    REQ_CHECK(srcNodeId != dstNodeId);
    auto& pins_src = MapGetRef(m_pins.node_to_pins, srcNodeId);
    auto& pins_dst = MapGetRef(m_pins.node_to_pins, dstNodeId);
    ASSERT(srcOutIdx < pins_src.outputs.size());
    ASSERT(dstInIdx < pins_dst.inputs.size());

    return AddLink(pins_src.outputs[srcOutIdx], pins_dst.inputs[dstInIdx],
                    newLinkId, commit);
}

void Multigraph::RemoveNode(node_id_t node_id) {
    ASSERT(m_nodes.contains(node_id));
    std::set<int> node_links = m_links.node_links[node_id];
    for (auto link_id : node_links) {
        RemoveLink(link_id);
    }

    m_links.node_links.erase(node_id);
    m_pins.RemoveNodePins(node_id);
    m_nodes.erase(node_id);
    SortNodes();
}

void Multigraph::RemoveLink(int link_id) {
    auto& link_pins = MapGetRef(m_links.link_id_to_pins, link_id);
    auto dst_pin = m_pins.GetPinById(link_pins.second);

    auto dst_node = GetNodeById(dst_pin->node_id);
    auto dst_input = dst_node->GetInputByIndex(dst_pin->node_io_id);

    dst_input->Disconnect();
    m_links.RemoveLink(link_id);
    SortNodes();
}

void Multigraph::RemoveLink(pin_id_t srcPinId, pin_id_t dstPinId) {
    ASSERT(LinkExists(srcPinId, dstPinId));
    link_id_t linkId = m_links.GetLinkId(srcPinId, dstPinId);
    RemoveLink(linkId);
}

int Multigraph::AddNode(NodeWrapper wrapper) {
    int new_id = id_counter++;
    ASSERT(!m_nodes.contains(new_id));
    m_pins.CreatePins(wrapper.node, new_id);
    m_nodes[new_id] = std::move(wrapper);
    SortNodes();
    return new_id;
}


// GraphIO //
GraphIO::GraphIO(Multigraph* graph, const NodeFactory* factory, const std::string& patchDir)
    : m_graph(graph), m_factory(factory), m_patchDir(patchDir) {}

void GraphIO::Serialize(nlohmann::json& j_out) const {
    auto graphLocked = m_graph->GetAccess();
    NodeNames names;
    using namespace nlohmann;

    // Save nodes
    auto& j_nodes = j_out["nodes"] = json::array();

    for (auto& [node_id, wrapper] : graphLocked->GetNodes()) {
        auto j_node = json::object();
        j_node["id"] = node_id;
        j_node["type"] = names.GetName(wrapper.node->GetType());
        j_node["params"] = json::object();
        j_node["attributes"] = json::object();
        wrapper.node->Save(j_node["params"]);
        wrapper.attrs.Save(j_node["attributes"]);
        j_nodes.push_back(j_node);
    }

    // Save links
    auto& j_links = j_out["links"] = json::array();
    auto& links = graphLocked->GetLinks();
    auto& pins = graphLocked->GetPins();

    for (auto& [link_id, link_pins] : links.link_id_to_pins) {
        auto pin_src = pins.GetPinById(link_pins.first);
        auto pin_dst = pins.GetPinById(link_pins.second);

        j_links.push_back(json::array({pin_src->node_id, pin_src->node_io_id,
                                       pin_dst->node_id, pin_dst->node_io_id}));
    }
}

void GraphIO::Deserialize(const nlohmann::json& j_in) const {
    auto graphLocked = m_graph->GetAccess();
    std::map<int, int> node_old_to_new;

    auto& j_nodes = JsonGetConstRef(j_in, "nodes");
    ASSERT(j_nodes.is_array());

    // Create nodes
    for (auto& j_node : j_nodes) {
        int old_id = JsonGetValue<int>(j_node, "id");
        std::string type_str = JsonGetValue<std::string>(j_node, "type");

        NodeWrapper wrapper;
        wrapper.node = m_factory->CreateNodeByName(type_str);
        wrapper.node->Load(JsonGetConstRef(j_node, "params"));
        wrapper.attrs.is_placed = false;
        wrapper.attrs.Load(JsonGetConstRef(j_node, "attributes"));

        int new_id = graphLocked->AddNode(std::move(wrapper));
        node_old_to_new[old_id] = new_id;
    }

    auto& j_links = JsonGetConstRef(j_in, "links");
    ASSERT(j_nodes.is_array());
    for (auto& j_link : j_links) {
        ASSERT(j_link.is_array());
        int new_link_id = 0;
        graphLocked->AddLink(
            /*src*/ MapGetRef(node_old_to_new, j_link.at(0).get<int>()),
            /*out_idx*/ j_link.at(1).get<int>(),
            /*dst*/ MapGetRef(node_old_to_new, j_link.at(2).get<int>()),
            /*in_idx*/ j_link.at(3).get<int>(), &new_link_id,
            /*commit=*/true);
    }
}

void GraphIO::SaveFile(const std::string& filename) const {
    auto json = nlohmann::json::object();
    Serialize(json);
    JsonSaveFile(filename, json);
    SPDLOG_INFO("[GraphIO] Save file: {}", filename);
}

void GraphIO::LoadFile(const std::string& filename) const {
    SPDLOG_INFO("[GraphIO] Load file: {}", filename);
    std::ifstream f(filename);
    auto json = nlohmann::json::object();
    f >> json;

    Deserialize(json);
}

void GraphIO::Reset() const {
    auto graphLocked = m_graph->GetAccess();
    std::vector<node_id_t> nodeIds;

    for (auto& [nodeId, nodeWrapper] : m_graph->GetNodes()) {
        nodeIds.push_back(nodeId);
    }

    for (auto id : nodeIds) {
        m_graph->RemoveNode(id);
    }
}
