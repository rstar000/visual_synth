#include "midi.h"

#include "SDL2/SDL.h"

constexpr uint8_t NOTE_ON_OFF = 144;
constexpr uint8_t NOTE_OFF = 128;
constexpr uint8_t CONTROL_CHANGE = 176;
constexpr uint8_t PITCH_BEND = 176;

void MidiCallback(double deltatime, std::vector<unsigned char>* message,
                void* userData) {
    unsigned int nBytes = message->size();

    MidiTracker* tracker = static_cast<MidiTracker*>(userData);
    unsigned char* msg = message->data();
    if (nBytes == 3) {
        SPDLOG_INFO("[MIDI] {} {} {}", message->at(0), message->at(1), message->at(2));
        if (msg[0] == NOTE_ON_OFF) { // Note on
            if (msg[2] != 0) {
                tracker->NoteOn(msg[1], msg[2]);
            } else {
                tracker->NoteOff(msg[1], msg[2]);
            }
        }

        if (msg[0] == CONTROL_CHANGE) {
            tracker->ControlChange(msg[1], msg[2]);
        }

        if (msg[0] == PITCH_BEND) {
            tracker->PitchBend(msg[2]);
        }
    }
}

MidiInput::MidiInput(MidiTracker* tracker) : m_tracker(tracker) {
    try {
        m_midiInput = std::make_unique<RtMidiIn>();
    } catch (RtMidiError& error) {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    ScanDevices();
}

std::span<MidiDeviceEntry const> MidiInput::GetPortNames() const
{
    return std::span{m_devices};
}

uint32_t MidiInput::GetActivePort() const
{
    return m_activeDevice;
}

void MidiInput::ScanDevices()
{
    if (m_midiInput->isPortOpen()) {
        m_midiInput->closePort();
    }

    m_devices.clear();
    uint32_t nPorts = m_midiInput->getPortCount();
    SPDLOG_INFO("[MidiInput] Num midi sources: {}", nPorts);

    for (unsigned i = 0; i <= nPorts; i++) {
        std::string portName = m_midiInput->getPortName(i);
        if (portName.empty()) {
            continue;
        }
        SPDLOG_INFO("[MidiInput]   Port {}: {}", i, portName);
        m_devices.push_back(MidiDeviceEntry {
            .deviceName = portName,
            .portId = i
        });
    }

    m_activeDevice = 0;
}

MidiInput::~MidiInput() 
{ 
    if (m_midiInput->isPortOpen()) {
        m_midiInput->closePort(); 
    }
}

void MidiInput::SetActiveDevice(uint32_t deviceId) 
{
    if (deviceId >= m_devices.size())
    {
        SPDLOG_ERROR("[MidiInput] Invalid device id: {}", deviceId);
        return;
    }
    if (m_midiInput->isPortOpen()) {
        m_midiInput->closePort();
    }
    m_midiInput->openPort(m_devices.at(deviceId).portId);
    m_midiInput->cancelCallback();
    m_midiInput->setCallback(&MidiCallback, m_tracker);
    m_midiInput->ignoreTypes(false, true, true);
    m_activeDevice = deviceId;
    SPDLOG_INFO("[MidiInput] Active device set: {} -> {}", deviceId, m_devices.at(deviceId).deviceName);
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
        int midi_key_idx = it->second + 12 * 5 + 1;
        if (down) {
            m_tracker->NoteOn(midi_key_idx, 128);
        } else {
            m_tracker->NoteOff(midi_key_idx, 0);
        }
    }
}
