#pragma once

#include <pulse/pulseaudio.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>

#include "rtaudio/RtAudio.h"

struct Synth;

class AudioOutputHandler {
   public:
    AudioOutputHandler(Synth* synth);
    ~AudioOutputHandler();

    void Start();
    void Stop();
    bool IsPlaying() const;

   private:
    Synth* m_synth;
    RtAudio dac;
};
