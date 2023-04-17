#include "midi.h"

#include "SDL2/SDL.h"

void MidiCallback(double deltatime, std::vector<unsigned char>* message,
                void* userData) {
    unsigned int nBytes = message->size();
    // for ( unsigned int i=0; i<nBytes; i++ )
    //   std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
    // if ( nBytes > 0 )
    //   std::cout << "stamp = " << deltatime << std::endl;

    MidiTracker* tracker = static_cast<MidiTracker*>(userData);
    if (nBytes == 3) {
        if (message->at(0) == 144) { // Note on
            tracker->NoteOn(message->at(1), message->at(2));
        }

        if (message->at(0) == 128) { // Note off
            tracker->NoteOff(message->at(1), message->at(2));
        }
    }
}

MidiInput::MidiInput(MidiTracker* tracker) : m_tracker(tracker) {
    try {
        midiin = std::make_unique<RtMidiIn>();
    } catch (RtMidiError& error) {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    // Check available ports vs. specified.
    unsigned int nPorts = midiin->getPortCount();
    SPDLOG_INFO("[MidiInput] Num midi sources: {}", nPorts);

    for (unsigned i = 0; i < nPorts; i++) {
        std::string portName = midiin->getPortName(i);
        SPDLOG_INFO("[MidiInput]   Port {}: {}", i, portName);
        port_names.push_back(portName);
    }

    SetActivePort(port_names.size() - 1);
}

MidiInput::~MidiInput() { midiin->closePort(); }

void MidiInput::SetActivePort(int i) {
    midiin->openPort(i);
    midiin->setCallback(&MidiCallback, m_tracker);
    midiin->ignoreTypes(false, false, false);
}

KeyboardInput::KeyboardInput(MidiTracker* tracker) : m_tracker(tracker) {

    const static std::vector<int> keys = {SDLK_z, SDLK_s, SDLK_x, SDLK_d,
                                          SDLK_c, SDLK_v, SDLK_g, SDLK_b,
                                          SDLK_h, SDLK_n, SDLK_j, SDLK_m};

    for (int i = 0; i < keys.size(); ++i) {
        key_to_note[keys[i]] = i;
    }
}

void KeyboardInput::ProcessKey(int key, bool down) {
    if (auto it = key_to_note.find(key); it != key_to_note.end()) {
        int midi_key_idx = it->second + 12 * 5;
        if (down) {
            m_tracker->NoteOn(midi_key_idx, 128);
        } else {
            m_tracker->NoteOff(midi_key_idx, 0);
        }
    }
}
