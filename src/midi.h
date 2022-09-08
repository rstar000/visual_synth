#pragma once

#include <thread>
#include <memory>
#include <vector>
#include <deque>
#include "rtmidi/RtMidi.h"

#include "note.h"


class MidiInput {
 public:
  MidiInput(std::shared_ptr<MidiTracker> tracker);
  ~MidiInput();
  
  const auto& GetPortNames() const {
    return port_names;
  }
  
  void SetActivePort(int i);

 private:
  std::shared_ptr<RtMidiIn> midiin;
  std::vector<std::string> port_names;
  
  std::shared_ptr<MidiTracker> tracker;
};

class KeyboardInput {
 public:
  KeyboardInput(std::shared_ptr<MidiTracker> tracker);

  void ProcessKey(int key, bool down);

  
  std::shared_ptr<MidiTracker> tracker;
  std::map<int, int> key_to_note;
};