#include "output.h"
#include "multigraph.h"
#include "gui.h"
#include "node_factory.h"
#include "midi.h"

#include <thread>
#include <cmath>

int main() {
  int buf_size = 100;
  int num_voices = 10;

  auto graph = std::make_shared<Multigraph>();
  auto writer = std::make_shared<SampleWriter>();
  auto midi_tracker = std::make_shared<MidiTracker>(num_voices);
  auto midi = std::make_shared<MidiInput>(midi_tracker);

  auto rt_output = std::make_shared<AudioOutputHandler>(AudioOutputHandler::Params{
      .graph = graph,
      .writer = writer,
      .buf_size = buf_size
  });

  auto factory = std::make_shared<NodeFactory>(NodeParams{
      .writer = writer,
      .tracker = midi_tracker,
      .num_samples = buf_size, 
      .num_voices = num_voices
  });
  
  auto gui = Gui(Gui::Params{
      .graph = graph, 
      .factory = factory, 
      .midi_tracker = midi_tracker
  });
  
  // audio_thread->Start();
  gui.Spin();

  return 0;
}