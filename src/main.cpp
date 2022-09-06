#include "output.h"
#include "multigraph.h"
#include "gui.h"
#include "node_factory.h"
#include "audio_thread.h"

#include <thread>
#include <cmath>

int main() {
  int buf_size = 2000;
  int num_voices = 6;

  auto graph = std::make_shared<Multigraph>();
  auto rt_output = std::make_shared<RtAudioOutputHandler>(buf_size);
  auto writer = std::make_shared<SampleWriter>(rt_output->GetBuffer());

  Context context{
      .writer = writer,
      .num_samples = buf_size, 
      .num_voices = num_voices
  };

  auto audio_thread = std::make_shared<AudioThread>(graph, context);
  auto factory = std::make_shared<NodeFactory>(context);
  
  auto gui = Gui(graph, factory, audio_thread);
  
  audio_thread->Start();
  gui.Spin();

  return 0;
}