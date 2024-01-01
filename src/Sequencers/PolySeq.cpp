#include "PolySeq.hpp"
#include "imgui.h"

#include "GridUI/Widgets/Fader.hpp"
#include "ImGui_Piano_imp.h"


PolySequencer::PolySequencer(uint32_t numChannels)
    : m_numSteps(MAX_SEQUENCER_STEPS)
    , m_numChannels{numChannels}
    , m_steps(numChannels * MAX_SEQUENCER_STEPS)
    , m_colors(ColorScheme::GenerateDefault())
{
    m_noteIdx.fill(0u);
}

inline bool PianoCallback(void* UserData, int Msg, int Key, float Vel) {
    int* keyOut = static_cast<int*>(UserData);
    if (Key <= 0 || Key >= 128) return false;  // midi max keys
    if (Msg == NoteGetStatus) {
        return *keyOut == (Key - 24);
    }

    if (Msg == NoteOn) {
        *keyOut = (Key - 24);
    }

    if (Msg == NoteOff) {
        // *keyOut = Key - 24;
    }
    return true;
}

void PolySequencer::DrawEditor()
{
    uint8_t noteIdxMin = 0u;
    uint8_t noteIdxMax = static_cast<uint8_t>(m_lookup.Size() - 1);

    ImGuiSliderFlags dragFlags = 0;
    dragFlags |= ImGuiSliderFlags_Vertical;
    float const TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    float const min_row_height = (float)(int)(TEXT_BASE_HEIGHT);

    ImGui::BeginGroup();

        static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("##SequenceEditor", 6, flags))
        {
            ImGui::TableSetupColumn("Seq",           ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);
            ImGui::TableSetupColumn("Enable",         ImGuiTableColumnFlags_WidthFixed, 0.0f, 1);
            ImGui::TableSetupColumn("Note",     ImGuiTableColumnFlags_WidthFixed, 50.0f, 2);
            ImGui::TableSetupColumn("Velocity",     ImGuiTableColumnFlags_None, 0.0f, 3);
            ImGui::TableSetupColumn("Duration",  ImGuiTableColumnFlags_None, 0.0f, 4);
            ImGui::TableSetupColumn("Rel",  ImGuiTableColumnFlags_None, 0.0f, 5);

            ImGui::TableNextRow(ImGuiTableRowFlags_Headers, min_row_height);

            for (int row = 0; row < m_numSteps; row++)
            {
                ImGui::PushID(row);
                ImGui::TableNextRow(ImGuiTableRowFlags_None);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d", row);

                ImGui::TableNextColumn();
                ImGui::Checkbox("##Enable", &m_steps[row].enable);
                ImGui::TableNextColumn();
                const auto& note = m_lookup.Get(m_noteIdx[row]);
                const char* activeNoteName = note.name.c_str();
                ImGui::DragScalar("##Semitone", ImGuiDataType_U8, &m_noteIdx[row], 0.02f, &noteIdxMin, &noteIdxMax, activeNoteName, dragFlags);
                m_steps[row].note = note.note;
                if (ImGui::BeginPopupContextItem()) {
                    ImGui::Text("Note %d key", row);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("x")) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::Separator();

                    if (ImGui::SmallButton("-")) {
                        if (m_noteIdx[row] >= 12) {
                            m_noteIdx[row] -= 12;
                        }
                    }

                    ImGui::SameLine();

                    if (ImGui::SmallButton("+")) {
                        if (m_noteIdx[row] + 12 < m_lookup.Size()) {
                            m_noteIdx[row] += 12;
                        }
                    }

                    ImGui::SameLine();

                    ImGui::Text("Octave: %d", m_noteIdx[row] / 12);

                    static int prevNote = 0;
                    int curNote = m_noteIdx[row] % 12;
                    int curNoteOld = curNote;
                    ImGui_PianoKeyboard("##piano", ImVec2{200, 150}, &prevNote, 24, 35,
                                        PianoCallback, &curNote, nullptr);

                    if (curNoteOld != curNote) {
                        m_noteIdx[row] = m_noteIdx[row] - curNoteOld + curNote;
                    }
                    ImGui::EndPopup();
                }

                ImGui::TableNextColumn();

                ImVec2 size{20.0f, min_row_height};
                ImGui::VFaderRectEx(size, m_colors, "Volume", &m_steps[row].velocity, FaderRectParams{
                    .minValue = 0.0f, .maxValue = 1.0f, 
                    .format = "%.2f",
                    .speed = 0.0f, 
                    .highlighted = false
                    });

                ImGui::TableNextColumn();

                ImGui::VFaderRectEx(size, m_colors, "Duration", &m_steps[row].duration, FaderRectParams{
                    .minValue = 0.0f, .maxValue = 1.0f, 
                    .format = "%.2f",
                    .speed = 0.0f, 
                    .highlighted = false
                    });
                if (ImGui::BeginPopupContextItem())
                {
                    ImGui::Text("Note %d duration", row);
                    ImGui::Separator();
                    if (ImGui::MenuItem("1/2", NULL)) {
                        m_steps[row].duration = 0.50f;
                    }
                    if (ImGui::MenuItem("1/4", NULL)) {
                        m_steps[row].duration = 0.25f;
                    }
                    if (ImGui::MenuItem("1/8", NULL)) {
                        m_steps[row].duration = 0.125f;
                    }
                    if (ImGui::MenuItem("1/16", NULL)) {
                        m_steps[row].duration = 0.0625f;
                    }
                    ImGui::EndPopup();
                }

                ImGui::TableNextColumn();
                ImGui::Checkbox("Rel", &m_steps[row].syncDuration);

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

    ImGui::EndGroup();
}
