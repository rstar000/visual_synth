#include <cmath>
#include <thread>
#include <limits>

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
    std::string patchDir;
    uint32_t numVoices;
    uint32_t numSamples;
};

void ParseArguments(int argc, char** const argv, ProgramArguments* dstArgs) {
    cxxopts::Options options("VS2", "Modular synthesizer | standalone executable.");
    options.add_options()
        ("i,input", "Input filename (optional)", cxxopts::value<std::string>())
        ("ns,num_samples", "Buffer size in samples", cxxopts::value<int>()->default_value("500"))
        ("nv,num_voices", "Number of polyphonic voices", cxxopts::value<int>()->default_value("10"))
        ("patch_dir", "Patch directory", cxxopts::value<std::string>());
    
    auto result = options.parse(argc, argv);
    
    if (result.count("input")) {
        dstArgs->filename = result["input"].as<std::string>();
    }

    if (result.count("patch_dir")) {
        dstArgs->patchDir = result["patch_dir"].as<std::string>();
    }
    
    dstArgs->numVoices = result["num_voices"].as<int>();
    dstArgs->numSamples = result["num_samples"].as<int>();
}

int main(int argc, char** argv) {
    ProgramArguments args;
    ParseArguments(argc, argv, &args);

    SynthInitParams params {
        .numSamples = args.numSamples,
        .numVoices = args.numVoices,
        .patchDir = args.patchDir
    };

    auto synth = std::make_unique<Synth>(params);
 
    if (!args.filename.empty()) {
        synth->GetIO()->LoadFile(args.filename);
    }

    synth->GetFactory()->DumpNodes("nodes.json");

    auto midi_input = std::make_unique<MidiInput>(synth->GetTracker());
    auto rt_output = std::make_shared<AudioOutputHandler>(synth.get());
    auto gui = Gui(*synth, *midi_input);

    // audio_thread->Start();
    gui.Spin();

    return 0;
}
