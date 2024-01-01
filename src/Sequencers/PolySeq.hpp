#pragma once

#include "GridUI/Colors.hpp"
#include "note.h"
#include "util.h"

struct SequencerStep {
    bool enable{false};
    bool syncDuration{true};
    Note note{};
    // If syncDuration == true
    //   fraction of current beat length
    // else
    //   time in seconds
    float duration{1.0f};
    float velocity{0.5f};
};

class PolySequencer {
public:
    static constexpr uint32_t MAX_SEQUENCER_STEPS = 64;

    PolySequencer(uint32_t numChannels);

    void DrawEditor();

    uint32_t NumSteps() const {
        return m_numSteps;
    }

    uint32_t NumChannels() const {
        return m_numChannels;
    }

    uint32_t GetStep() const {
        return m_numChannels;
    }

private:
    uint32_t m_numSteps{};
    uint32_t m_numChannels{};
    std::vector<SequencerStep> m_steps{};
    NoteLookup m_lookup{};
    ColorScheme m_colors;
    std::array<uint8_t, 256> m_noteIdx;
};
