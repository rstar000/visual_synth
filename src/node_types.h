#pragma once

#include <map>
#include <string>
#include "util.h"

#define X_NODE_NAMES \
       X(SINE_OSC) \
       X(SQUARE_OSC) \
       X(SUPER_OSC) \
       X(KEYBOARD) \
       X(OUTPUT) \
       X(SLIDER) \
       X(CONSTANT) \
       X(ADD) \
       X(MULTIPLY) \
       X(CLAMP) \
       X(NEGATE) \
       X(RANGE_SHIFT) \
       X(DEBUG) \
       X(CLOCK) \
       X(CHANNEL_UNPACK) \
       X(PIANO) \
       X(MIDI) \
       X(MIDI_CONTROL) \
       X(HOLD_NOTES) \
       X(ARPEGGIATOR) \
       X(ADSR) \
       X(DX7EG) \
       X(MONO_SEQUENCER) \
       X(MIX) \
       X(BIQUAD_FILTER) \
       X(LFO) \

enum class NodeType {
#define X(v)       v,
    X_NODE_NAMES
#undef X
};

enum class NodeCategory {
    OCSILLATOR,
    SEQUENCER,
    IO,
    UTILITY,
    ARITHMETIC,
    DEBUG,
    FILTER,
    FX
};

struct NodeNames {
    NodeNames() {
        #define X(v) type_to_name[NodeType::v] = #v; \
                    name_to_type[#v] = NodeType::v;
        X_NODE_NAMES
        #undef X
    }

    NodeType GetType(const std::string& s) const {
        return MapGetConstRef(name_to_type, s);
    }

    std::string GetName(const NodeType& t) const {
        return MapGetConstRef(type_to_name, t);
    }

   private:
    std::map<NodeType, std::string> type_to_name;
    std::map<std::string, NodeType> name_to_type;
};
