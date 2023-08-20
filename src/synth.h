#pragma once

#include <cmath>
#include <thread>

#include "GridUI/GridUI.hpp"
#include "midi.h"
#include "multigraph.h"
#include "node_factory.h"
#include "util.h"
#include "writer.h"


struct Synth {
    Synth(const uint32_t numSamples, const uint32_t numVoices) {
        m_multigraph = std::make_unique<Multigraph>();
        m_writer = std::make_unique<SampleWriter>();
        m_tracker = std::make_unique<MidiTracker>(numVoices);
        m_gridUI = std::make_unique<GridUI>();
        m_playback = std::make_unique<PlaybackContext>(
            PlaybackContext {
                .timestamp = 0.0f,
                .sampleIdx = 0,
                .numSamples = numSamples,
                .numVoices = numVoices,
                .sampleRate = kSampleRate
            }
        );
        m_factory = std::make_unique<NodeFactory>(
            NodeParams{
                .writer = m_writer.get(),
                .tracker = m_tracker.get(),
                .ui = m_gridUI.get(),
                .playback = m_playback.get()
            });
        m_io = std::make_unique<GraphIO>(m_multigraph.get(), m_factory.get());

        SPDLOG_INFO("[Synth] Initialized with numSamples = {}, numVoices = {}",
                    m_playback->numSamples, m_playback->numVoices);
    }

    const NodeFactory* GetFactory() const { return m_factory.get(); }

    Multigraph* GetGraph() const { return m_multigraph.get(); }

    const GraphIO* GetIO() const { return m_io.get(); }

    SampleWriter* GetWriter() const { return m_writer.get(); }

    MidiTracker* GetTracker() const { return m_tracker.get(); }

    PlaybackContext* GetPlayback() const { return m_playback.get(); }

    GridUI* GetGridUI() const { return m_gridUI.get(); }

    void ProcessFrame(SampleType* buffer) {
        auto access = m_multigraph->GetAccess();
        auto& nodes = access->GetSortedNodes();

        m_writer->SetBuffer(buffer);

        for (size_t frameSampleId = 0; frameSampleId < m_playback->numSamples; ++frameSampleId) {
            m_playback->timestamp = m_writer->GetTimestamp(frameSampleId);
            m_playback->sampleIdx = m_writer->GetSample(frameSampleId);
            for (auto& node : nodes) {
                for (int voiceId = 0; voiceId < node->NumVoices(); ++voiceId) {
                    node->Process(voiceId, frameSampleId, m_playback->timestamp);
                }
            }
        }
    }

   private:
    std::unique_ptr<Multigraph> m_multigraph;
    std::unique_ptr<SampleWriter> m_writer;
    std::unique_ptr<MidiTracker> m_tracker;
    std::unique_ptr<NodeFactory> m_factory;
    std::unique_ptr<GridUI> m_gridUI;
    std::unique_ptr<GraphIO> m_io;
    std::unique_ptr<PlaybackContext> m_playback;
};