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
  RegisterNode<AudioOutputNode>(NodeCategory::IO);
  RegisterNode<PianoNode>(NodeCategory::IO);
  RegisterNode<MidiNode>(NodeCategory::IO);
  RegisterNode<SineOscillatorNode>(NodeCategory::OCSILLATOR);
  RegisterNode<SquareOscillatorNode>(NodeCategory::OCSILLATOR);

  RegisterNode<SliderNode>(NodeCategory::UTILITY);
  RegisterNode<ConstantNode>(NodeCategory::UTILITY);
  RegisterNode<ChannelUnpackNode>(NodeCategory::UTILITY);

  RegisterNode<AddNode>(NodeCategory::ARITHMETIC);
  RegisterNode<MultiplyNode>(NodeCategory::ARITHMETIC);
  RegisterNode<NegateNode>(NodeCategory::ARITHMETIC);
  RegisterNode<MixNode>(NodeCategory::ARITHMETIC);
  RegisterNode<ClampNode>(NodeCategory::ARITHMETIC);

  RegisterNode<DebugNode>(NodeCategory::DEBUG);

  RegisterNode<ClockNode>(NodeCategory::SEQUENCER);
}