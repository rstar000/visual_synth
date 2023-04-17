#pragma once

#include <cmath>
#include <thread>

#include "midi.h"
#include "multigraph.h"
#include "node_factory.h"
#include "util.h"
#include "writer.h"

struct Synth {
    Synth(int numSamples, int numVoices) {
        m_numSamples = numSamples;
        m_numVoices = numVoices;
        m_multigraph = std::make_unique<Multigraph>();
        m_writer = std::make_unique<SampleWriter>();
        m_tracker = std::make_unique<MidiTracker>(m_numVoices);
        m_factory = std::make_unique<NodeFactory>(
            NodeParams{.writer = m_writer.get(),
                       .tracker = m_tracker.get(),
                       .num_samples = m_numSamples,
                       .num_voices = m_numVoices});
        m_io = std::make_unique<GraphIO>(m_multigraph.get(), m_factory.get());

        SPDLOG_INFO("[Synth] Initialized with numSamples = {}, numVoices = {}",
                     m_numSamples, m_numVoices);
    }

    const NodeFactory* GetFactory() const { return m_factory.get(); }

    Multigraph* GetGraph() const { return m_multigraph.get(); }

    const GraphIO* GetIO() const { return m_io.get(); }

    SampleWriter* GetWriter() const { return m_writer.get(); }

    MidiTracker* GetTracker() const { return m_tracker.get(); }

    int numSamples() const { return m_numSamples; }

    int numVoices() const { return m_numVoices; }

    void ProcessFrame(SampleType* buffer) {
        auto access = m_multigraph->GetAccess();
        auto& nodes = access->GetSortedNodes();

        m_writer->SetBuffer(buffer);

        for (size_t sampleId = 0; sampleId < m_numSamples; ++sampleId) {
            for (auto& node : nodes) {
                float timestamp = m_writer->GetTimestamp(sampleId);
                for (int voiceId = 0; voiceId < node->NumVoices(); ++voiceId) {
                    node->Process(voiceId, sampleId, timestamp);
                }
            }
        }
    }

   private:
    std::unique_ptr<Multigraph> m_multigraph;
    std::unique_ptr<SampleWriter> m_writer;
    std::unique_ptr<MidiTracker> m_tracker;
    std::unique_ptr<NodeFactory> m_factory;
    std::unique_ptr<GraphIO> m_io;

    int m_numSamples;
    int m_numVoices;
};