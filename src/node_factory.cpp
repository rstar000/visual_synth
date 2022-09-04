#include "node_factory.h"

#include "nodes/osc.h"
#include "nodes/sink.h"
#include "nodes/common.h"
#include "nodes/seq.h"

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
  RegisterContextNode<AudioOutputNode>(NodeCategory::IO, [this] () -> NodePtr {
    return std::make_shared<AudioOutputNode>(ctx.output);
  });
  
  RegisterSimpleNode<SineOscillatorNode>(NodeCategory::OCSILLATOR);
  RegisterSimpleNode<SquareOscillatorNode>(NodeCategory::OCSILLATOR);

  RegisterSimpleNode<SliderNode>(NodeCategory::UTILITY);
  RegisterSimpleNode<ConstantNode>(NodeCategory::UTILITY);
  RegisterSimpleNode<ChannelUnpackNode>(NodeCategory::UTILITY);

  RegisterSimpleNode<AddNode>(NodeCategory::ARITHMETIC);
  RegisterSimpleNode<MultiplyNode>(NodeCategory::ARITHMETIC);
  RegisterSimpleNode<NegateNode>(NodeCategory::ARITHMETIC);
  RegisterSimpleNode<MixNode>(NodeCategory::ARITHMETIC);
  RegisterSimpleNode<ClampNode>(NodeCategory::ARITHMETIC);

  RegisterSimpleNode<DebugNode>(NodeCategory::DEBUG);

  RegisterSimpleNode<ClockNode>(NodeCategory::SEQUENCER);
}