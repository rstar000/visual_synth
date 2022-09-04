#include "util.h"
#include "output.h"

int saw( void *outputBuffer, void * /*inputBuffer*/, unsigned int nBufferFrames,
         double /*streamTime*/, RtAudioStreamStatus status, void *data ) {
  SampleType* buf_out = (SampleType *) outputBuffer;
  SampleBuffer* buf_in = static_cast<SampleBuffer*>(data);

  if ( status )
    std::cout << "Stream underflow detected!" << std::endl;

  size_t available_samples = buf_in->ReadyToRead();
  size_t N = std::min(available_samples, (size_t) nBufferFrames);
  // std::cout << nBufferFrames << ' ' << N << std::endl;
  buf_in->Read(buf_out, N);
  return 0;
}


RtAudioOutputHandler::RtAudioOutputHandler(int buf_size) 
  : buf(std::make_shared<SampleBuffer>(buf_size)) {
  if (dac.getDeviceCount() < 1) {
    std::cout << "No output device" << std::endl;
    exit(1);
  }
  
  unsigned int _buf_size = static_cast<unsigned int>(buf_size);
  
  RtAudio::StreamParameters params;
  params.deviceId = dac.getDefaultOutputDevice();
  params.nChannels = 1;

  RtAudio::StreamOptions options;
  options.flags = 0;

  try {
    dac.openStream(
      /*output_params*/&params, 
      /*input_params*/NULL, 
      /*format*/RTAUDIO_SINT16, 
      /*sample_rate*/kSampleRate, 
      &_buf_size, 
      &saw, 
      buf.get(),
      &options);
    std::cout << "Actual buf size: " << buf_size << std::endl;
    dac.startStream();
  }
  catch (RtAudioError& e ) {
    e.printMessage();
    if (dac.isStreamOpen()) dac.closeStream();
  }
}

void RtAudioOutputHandler::Start() {
  if (!IsPlaying()) {
    dac.startStream();
  }
}

void RtAudioOutputHandler::Stop() {
  if (IsPlaying()) {
    dac.stopStream();
  }
}

bool RtAudioOutputHandler::IsPlaying() const {
  return dac.isStreamRunning();
}

RtAudioOutputHandler::~RtAudioOutputHandler() {
  if (dac.isStreamOpen()) dac.closeStream();
}
