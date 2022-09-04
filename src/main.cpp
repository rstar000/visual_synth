#include "output.h"
#include "multigraph.h"
#include "gui.h"
#include "node_factory.h"
#include "audio_thread.h"

#include <thread>
#include <cmath>

int main() {
  size_t buf_size = 4000;

  auto graph = std::make_shared<Multigraph>();
  auto rt_output = std::make_shared<RtAudioOutputHandler>(buf_size);
  auto audio_thread = std::make_shared<AudioThread>(rt_output, graph);
  auto factory = std::make_shared<NodeFactory>(Context{audio_thread->GetOutput()});
  
  auto gui = Gui(graph, factory, audio_thread);
  
  audio_thread->Start();
  gui.Spin();

  return 0;
}