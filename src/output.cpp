#include "output.h"

#include "multigraph.h"
#include "synth.h"
#include "util.h"

// Actual sound generation happens here, in RtAudio thread
int callback(void* outputBuffer, void* /*inputBuffer*/,
             unsigned int nBufferFrames, double /*streamTime*/,
             RtAudioStreamStatus status, void* data) {
    // TimeIt time_it([&] (const auto& x) {
    //   std::cout << "callback  " << x << std::endl;
    //   std::cout << "waterline " << (nBufferFrames /
    //   static_cast<float>(kSampleRate)) * 1000 << std::endl;
    // });

    SampleType* buf_out = static_cast<SampleType*>(outputBuffer);
    Synth* synth = static_cast<Synth*>(data);

    // ASSERT(ctx->buf_size <= nBufferFrames);

    if (status) {
        SPDLOG_ERROR("Stream underflow detected!");
    }

    std::memset(buf_out, 0, nBufferFrames * sizeof(SampleType));

    synth->ProcessFrame(buf_out);
    synth->GetWriter()->Flush(nBufferFrames);

    return 0;
}

AudioOutputHandler::AudioOutputHandler(Synth* synth)
    : m_synth(synth), dac(RtAudio::Api::LINUX_ALSA) {
    if (dac.getDeviceCount() < 1) {
        SPDLOG_ERROR("No output device");
        exit(1);
    }

    SPDLOG_INFO("[AudioOutputHandler] API: {}", dac.getCurrentApi());

    unsigned int _buf_size = static_cast<unsigned int>(m_synth->GetPlayback()->numSamples);

    auto num_devices = dac.getDeviceCount();
    for (unsigned int i = 0; i < num_devices; ++i) {
        auto dev_info = dac.getDeviceInfo(i);
        SPDLOG_INFO("[AudioOutputHandler] Device {}:", i);
        SPDLOG_INFO("[AudioOutputHandler]   Name: {}", dev_info.name);
        SPDLOG_INFO("[AudioOutputHandler]   Channels: {}", dev_info.outputChannels);
        SPDLOG_INFO("[AudioOutputHandler]   Formats: {}", dev_info.nativeFormats);
        SPDLOG_INFO("[AudioOutputHandler]   SR: {}", dev_info.preferredSampleRate);
    }

    RtAudio::StreamParameters params;
    params.deviceId = dac.getDefaultOutputDevice();
    params.nChannels = 1;
    params.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_NONINTERLEAVED;

    try {
        dac.openStream(
            /*output_params*/ &params,
            /*input_params*/ NULL,
            /*format*/ RTAUDIO_SINT16,
            /*sample_rate*/ kSampleRate,
            /*buf_size*/ &_buf_size,
            /*callback*/ &callback,
            /*userdata*/ m_synth,
            /*options*/ &options);
        SPDLOG_INFO("[AudioOutputHandler] Actual buf size: {}", _buf_size);
        dac.startStream();
    } catch (RtAudioError& e) {
        e.printMessage();
        if (dac.isStreamOpen()) dac.closeStream();
    }
}

AudioOutputHandler::~AudioOutputHandler() {
    if (dac.isStreamOpen()) dac.closeStream();
}

void AudioOutputHandler::Start() {
    if (!IsPlaying()) {
        dac.startStream();
    }
}

void AudioOutputHandler::Stop() {
    if (IsPlaying()) {
        dac.stopStream();
    }
}

bool AudioOutputHandler::IsPlaying() const { return dac.isStreamRunning(); }
