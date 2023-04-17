#pragma once

#include "rtmidi/RtMidi.h"
#include <deque>
#include <memory>
#include <thread>
#include <vector>

#include "note.h"

class MidiInput {
  public:
    MidiInput(MidiTracker* tracker);
    ~MidiInput();

    const auto& GetPortNames() const { return port_names; }

    void SetActivePort(int i);

  private:
    std::unique_ptr<RtMidiIn> midiin;
    std::vector<std::string> port_names;

    MidiTracker* m_tracker;
};

class KeyboardInput {
  public:
    KeyboardInput(MidiTracker* tracker);

    void ProcessKey(int key, bool down);

    MidiTracker* m_tracker;
    std::map<int, int> key_to_note;
};