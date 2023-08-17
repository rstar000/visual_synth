#include <cmath>
#include <thread>

#include "gui.h"
#include "midi.h"
#include "multigraph.h"
#include "node_factory.h"
#include "output.h"
#include "synth.h"
#include "util.h"

#include "cxxopts.hpp"

struct ProgramArguments {
    std::string filename;
    int numVoices;
    int numSamples;
};

void ParseArguments(int argc, char** const argv, ProgramArguments* dstArgs) {
    cxxopts::Options options("VS2", "Modular synthesizer | standalone executable.");
    options.add_options()
        ("i,input", "Input filename (optional)", cxxopts::value<std::string>())
        ("ns,num_samples", "Buffer size in samples", cxxopts::value<int>()->default_value("500"))
        ("nv,num_voices", "Number of polyphonic voices", cxxopts::value<int>()->default_value("10"));
    
    auto result = options.parse(argc, argv);
    
    if (result.count("input")) {
        dstArgs->filename = result["input"].as<std::string>();
    }
    
    dstArgs->numVoices = result["num_voices"].as<int>();
    dstArgs->numSamples = result["num_samples"].as<int>();
}

int main(int argc, char** argv) {
    ProgramArguments args;
    ParseArguments(argc, argv, &args);

    auto synth = std::make_unique<Synth>(args.numSamples, args.numVoices);
    
    if (!args.filename.empty()) {
        synth->GetIO()->LoadFile(args.filename);
    }
    
    synth->GetFactory()->DumpNodes("nodes.json");

    auto midi_input = std::make_unique<MidiInput>(synth->GetTracker());
    auto rt_output = std::make_shared<AudioOutputHandler>(synth.get());
    auto gui = Gui(synth.get());

    // audio_thread->Start();
    gui.Spin();

    return 0;
}