#pragma once

#include <deque>
#include <memory>
#include <thread>
#include <vector>
#include <span>

#include "util.h"
#include "note.h"
#include "rtmidi/RtMidi.h"

struct MidiDeviceEntry {
    std::string deviceName;
    uint32_t portId;
};

class MidiInput {
   public:
    MidiInput(MidiTracker* tracker);
    ~MidiInput();

    std::span<MidiDeviceEntry const> GetPortNames() const;

    uint32_t GetActivePort() const;
    void SetActiveDevice(uint32_t deviceId);
    void ScanDevices();

   private:
    uint32_t m_activeDevice = 0;
   
    std::unique_ptr<RtMidiIn> m_midiInput;
    std::vector<MidiDeviceEntry> m_devices;

    MidiTracker* m_tracker;
};

class KeyboardInput {
   public:
    KeyboardInput(MidiTracker* tracker);

    void ProcessKey(int key, bool down);

    MidiTracker* m_tracker;
    std::map<int, int> key_to_note;
};