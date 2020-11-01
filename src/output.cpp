#include "util.h"
#include "output.h"

// TODO: lots of refactoring here.

PulseAudioOutputHandler::PulseAudioOutputHandler(
    const std::shared_ptr<SampleBuffer>& buffer) {

  mainloop = pa_threaded_mainloop_new();
  ASSERT(mainloop);
  mainloop_api = pa_threaded_mainloop_get_api(mainloop);
  context = pa_context_new(mainloop_api, "Synth");
  ASSERT(context);

  // Set a callback so we can wait for the context to be ready
  pa_context_set_state_callback(context, &context_state_cb, mainloop);

  // Lock the mainloop so that it does not run and crash before the context is ready
  pa_threaded_mainloop_lock(mainloop);

  // Start the mainloop
  ASSERT_EQUAL(pa_threaded_mainloop_start(mainloop), 0);
  ASSERT_EQUAL(pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL), 0);

  // Wait for the context to be ready
  while (true) {
    pa_context_state_t context_state = pa_context_get_state(context);
    ASSERT(PA_CONTEXT_IS_GOOD(context_state));
    if (context_state == PA_CONTEXT_READY) break;
    pa_threaded_mainloop_wait(mainloop);
  }

  // Create a playback stream
  pa_sample_spec sample_spec;
  sample_spec.format = kSampleFormat;
  sample_spec.rate = kSampleRate;
  sample_spec.channels = kNumChannels;

  pa_channel_map map;
  pa_channel_map_init_stereo(&map);

  stream = pa_stream_new(context, "Synth", &sample_spec, &map);
  pa_stream_set_state_callback(stream, stream_state_cb, mainloop);
  SetBuffer(buffer);

  size_t samples_in_buffer = buffer->Size();
  pa_buffer_attr buffer_attr;
  buffer_attr.maxlength = static_cast<uint32_t>(
      samples_in_buffer * kNumChannels * sizeof(SampleType));
  buffer_attr.tlength = std::numeric_limits<uint32_t>::max();
  buffer_attr.prebuf = std::numeric_limits<uint32_t>::max();
  buffer_attr.minreq = std::numeric_limits<uint32_t>::max();

  pa_stream_flags_t stream_flags;
  stream_flags = static_cast<pa_stream_flags_t>(
      PA_STREAM_START_CORKED | PA_STREAM_INTERPOLATE_TIMING |
      PA_STREAM_NOT_MONOTONIC | PA_STREAM_AUTO_TIMING_UPDATE |
      PA_STREAM_ADJUST_LATENCY);

  // Connect stream to the default audio output sink
  ASSERT_EQUAL(
    pa_stream_connect_playback(
      /*stream = */ stream,
      /*dev = */    nullptr,
      /*attr = */   &buffer_attr,
      /*flags = */  stream_flags,
      /*volume = */ nullptr,
      /*sync = */   nullptr),
    0);

  // Wait for the stream to be ready
  while (true) {
    pa_stream_state_t stream_state = pa_stream_get_state(stream);
    ASSERT(PA_STREAM_IS_GOOD(stream_state));
    if (stream_state == PA_STREAM_READY) break;
    pa_threaded_mainloop_wait(mainloop);
  }

  pa_threaded_mainloop_unlock(mainloop);
}

PulseAudioOutputHandler::~PulseAudioOutputHandler() {
  Stop();
  pa_threaded_mainloop_unlock(mainloop);
  pa_threaded_mainloop_stop(mainloop);
  pa_threaded_mainloop_free(mainloop);
}

void PulseAudioOutputHandler::SetBuffer(const std::shared_ptr<SampleBuffer>& buffer) {
  pa_stream_set_write_callback(stream, stream_write_cb, buffer.get());
}

void PulseAudioOutputHandler::Stop() {
  pa_stream_cork(stream, 1, stream_success_cb, mainloop);
}

void PulseAudioOutputHandler::Start() {
  pa_stream_cork(stream, 0, stream_success_cb, mainloop);
}

void context_state_cb(pa_context* context, void* mainloop) {
  pa_threaded_mainloop_signal(static_cast<pa_threaded_mainloop*>(mainloop), 0);
}

void stream_state_cb(pa_stream *s, void *mainloop) {
  pa_threaded_mainloop_signal(static_cast<pa_threaded_mainloop*>(mainloop), 0);
}

void stream_success_cb(pa_stream *stream, int success, void *userdata) {
  return;
}

void stream_write_cb(pa_stream *stream, size_t requested_bytes, void *userdata) {
  // Static storage for samples fetched from buffer
  static std::vector<SampleType> samples;
  size_t requested_samples = requested_bytes / (sizeof(SampleType) * kNumChannels);

  if (samples.size() < requested_samples) {
    samples.resize(requested_samples);
  }

  auto buffer = static_cast<SampleBuffer*>(userdata);
  std::size_t available_samples = buffer ? buffer->ReadyToRead() : requested_samples;
  std::size_t samples_to_store = std::min(requested_samples, available_samples);

  if (buffer) {
    buffer->Read(samples, samples_to_store);
  } else {
    std::fill_n(samples.begin(), samples_to_store, 0);
  }

  if (requested_samples > available_samples) {
    std::cerr << "Warning! Sound output is starving!" << std::endl;
    std::fill(samples.begin() + samples_to_store, samples.end(), 0);
  }

  int16_t* stream_data = NULL;

  // TODO: make this part not hacky
  size_t nbytes = requested_bytes;
  size_t bytes_written = 0;
  size_t stream_sample_idx = 0;  // Num samples written to both channels
  pa_stream_begin_write(stream, reinterpret_cast<void**>(&stream_data), &nbytes);
  for (std::size_t buf_sample_idx = 0; buf_sample_idx < samples.size(); ++buf_sample_idx) {
    if (bytes_written + sizeof(SampleType) >= nbytes) {
      break;
    }
    stream_data[stream_sample_idx] = samples[buf_sample_idx];
    stream_data[stream_sample_idx + 1] = samples[buf_sample_idx];
    stream_sample_idx += 2;
    bytes_written = stream_sample_idx * sizeof(SampleType);
  }
  pa_stream_write(stream, stream_data, nbytes, NULL, 0LL, PA_SEEK_RELATIVE);
}
