#pragma once

#include "IPlug_include_in_plug_hdr.h"

#include "output.h"
#include "multigraph.h"
#include "gui.h"
#include "node_factory.h"
#include "audio_thread.h"

class SynthPlugin final : public iplug::Plugin {
{
public:
  SynthPlugin(const InstanceInfo& info);

  bool SerializeState(IByteChunk &chunk) const override;
  int UnserializeState(const IByteChunk &chunk, int startPos) override;
//  bool CompareState(const uint8_t* pIncomingState, int startPos) const override;
  
  void OnIdle() override;
  void OnUIOpen() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
private:
  void UpdateUIControls();
  
  std::atomic<int> mStepPos;
  int mPrevPos = -1;
  double mSteps[kNumSteps] = {};
  
  std::shared_ptr<Multigraph> graph;
  std::shared_ptr<SampleBuffer> buffer;
  std::shared_ptr<NodeFactory> factory;
  std::shared_ptr<Gui> gui;
  std::shared_ptr<SampleWriter> writer;
};