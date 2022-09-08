#include "output.h"
#include "util.h"
#include "multigraph.h"


// Actual sound generation happens here, in RtAudio thread
int callback(void* outputBuffer, void * /*inputBuffer*/, unsigned int nBufferFrames,
         double /*streamTime*/, RtAudioStreamStatus status, void *data ) {

  // TimeIt time_it([&] (const auto& x) { 
  //   std::cout << "callback  " << x << std::endl;
  //   std::cout << "waterline " << (nBufferFrames / static_cast<float>(kSampleRate)) * 1000 << std::endl;
  // });

  auto buf_out = static_cast<SampleType*>(outputBuffer);
  auto ctx = static_cast<AudioOutputHandler::Params*>(data);
  
  ASSERT(ctx->buf_size <= nBufferFrames);

  if ( status )
    std::cout << "Stream underflow detected!" << std::endl;

  std::memset(buf_out, 0, nBufferFrames * sizeof(SampleType));

  {
    // Graph mutex locked
    auto access = ctx->graph->GetAccess();
    auto nodes = access->GetSortedNodes();
    
    ctx->writer->SetBuffer(buf_out);
    
    for (auto node : nodes) {
      for (size_t s = 0; s < nBufferFrames; ++s) {
        float timestamp = ctx->writer->GetTimestamp(s);
        for (int v = 0; v < node->NumVoices(); ++v) {
          node->Process(v, s, timestamp);
        }
      }
    }
  }

  ctx->writer->Flush(nBufferFrames);
  
  return 0;
}


AudioOutputHandler::AudioOutputHandler(Params ctx)
    : ctx(ctx)
    , dac(RtAudio::Api::LINUX_ALSA) {
  if (dac.getDeviceCount() < 1) {
    std::cout << "No output device" << std::endl;
    exit(1);
  }
  
  std::cout << "API: " << dac.getCurrentApi() << std::endl;
  
  unsigned int _buf_size = static_cast<unsigned int>(ctx.buf_size);
  
  auto num_devices = dac.getDeviceCount();
  for (unsigned int i = 0; i < num_devices; ++i) {
    auto dev_info = dac.getDeviceInfo(i);
    std::cout << "Name: " << dev_info.name << std::endl;
    std::cout << "Out chls: " << dev_info.outputChannels << std::endl;
    std::cout << "Formats: " << dev_info.nativeFormats << std::endl;
    std::cout << "SR: " << dev_info.preferredSampleRate << std::endl;
  }
  
  RtAudio::StreamParameters params;
  params.deviceId = dac.getDefaultOutputDevice();
  params.nChannels = 1;
  params.firstChannel = 0;

  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_NONINTERLEAVED;

  try {
    dac.openStream(
      /*output_params*/&params, 
      /*input_params*/NULL, 
      /*format*/RTAUDIO_SINT16, 
      /*sample_rate*/kSampleRate,
      /*buf_size*/&_buf_size, 
      /*callback*/&callback, 
      /*userdata*/&this->ctx,
      /*options*/&options);
    std::cout << "Actual buf size: " << _buf_size << std::endl;
    dac.startStream();
  } catch (RtAudioError& e ) {
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

bool AudioOutputHandler::IsPlaying() const {
  return dac.isStreamRunning();
}

