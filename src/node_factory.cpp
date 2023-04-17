#include "node_factory.h"

#include "nodes/common.h"
#include "nodes/env.h"
#include "nodes/osc.h"
#include "nodes/seq.h"
#include "nodes/sink.h"

template <typename T>
void RegisterDisplayName(std::map<NodeType, std::string>& m) {
    m[T::TYPE] = T::DISPLAY_NAME;
}

void NodeFactory::RegisterCategories() {
    category_names[NodeCategory::ARITHMETIC] = "Arithmetic";
    category_names[NodeCategory::OCSILLATOR] = "Oscillator";
    category_names[NodeCategory::UTILITY] = "Utility";
    category_names[NodeCategory::DEBUG] = "Debug";
    category_names[NodeCategory::SEQUENCER] = "Sequencer";
    category_names[NodeCategory::IO] = "I/O";
}

void NodeFactory::RegisterNodes() {
    RegisterNode<AudioOutputNode>(NodeCategory::IO);
    RegisterNode<PianoNode>(NodeCategory::IO);
    RegisterNode<MidiNode>(NodeCategory::IO);

    RegisterNode<SineOscillatorNode>(NodeCategory::OCSILLATOR);
    RegisterNode<SquareOscillatorNode>(NodeCategory::OCSILLATOR);
    RegisterNode<SuperOscNode>(NodeCategory::OCSILLATOR);

    RegisterNode<SliderNode>(NodeCategory::UTILITY);
    RegisterNode<ConstantNode>(NodeCategory::UTILITY);
    RegisterNode<ChannelUnpackNode>(NodeCategory::UTILITY);
    RegisterNode<ADSRNode>(NodeCategory::UTILITY);
    RegisterNode<DX7EGNode>(NodeCategory::UTILITY);

    RegisterNode<AddNode>(NodeCategory::ARITHMETIC);
    RegisterNode<MultiplyNode>(NodeCategory::ARITHMETIC);
    RegisterNode<NegateNode>(NodeCategory::ARITHMETIC);
    RegisterNode<MixNode>(NodeCategory::ARITHMETIC);
    RegisterNode<ClampNode>(NodeCategory::ARITHMETIC);

    RegisterNode<DebugNode>(NodeCategory::DEBUG);

    RegisterNode<ClockNode>(NodeCategory::SEQUENCER);
}

void NodeFactory::DumpNodes(const std::string& filename) const {
    auto j = nlohmann::json::array();
    for (auto& [nodeType, _] : factory) {
        auto node = CreateNode(nodeType);
        std::string nodeName = names.GetName(nodeType);
        
        nlohmann::json j_spec = nlohmann::json::object();
        j_spec["name"] = nodeName;
        j_spec["inputs"] = nlohmann::json::array();
        j_spec["outputs"] = nlohmann::json::array();
        for (size_t i = 0; i < node->NumInputs(); ++i) {
            auto input = node->GetInputByIndex(i);
            nlohmann::json j_in = nlohmann::json::object();
            j_in["name"] = input->name;
            j_in["index"] = i;
            j_in["type"] = input->type;
            j_spec["inputs"].push_back(j_in);
        }

        j_spec["outputs"] = nlohmann::json::array();
        for (size_t i = 0; i < node->NumOutputs(); ++i) {
            auto input = node->GetOutputByIndex(i);
            nlohmann::json j_in = nlohmann::json::object();
            j_in["name"] = input->name;
            j_in["index"] = i;
            j_in["type"] = input->type;
            j_spec["outputs"].push_back(j_in);
        }
        j.push_back(j_spec);
    }
    
    JsonSaveFile(filename, j);
}