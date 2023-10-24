#include "GUI/PatchBrowser.hpp"

#include <filesystem>

namespace {

namespace fs = std::filesystem;

const std::string JSON_EXT = ".json";

bool ListFiles(const std::string& path, std::vector<FileEntry>& result)
{
    std::error_code ec{};
    fs::directory_iterator it{path, ec};
    if (ec) {
        return false;
    }

    for (; it != fs::directory_iterator{}; ++it) {
        auto path = it->path();
        if (path.extension().string() != JSON_EXT) {
            continue;
        }
        FileEntry file;
        file.path = it->path();
        file.filename = it->path().stem();
        result.push_back(file);
    }
    return true;
}

}  // namespace

PatchBrowser::PatchBrowser(const GraphIO* io)
    : m_patchDir{io->PatchDir()}
    , m_io{io}
{
    ListFiles(m_patchDir, m_files);
    SortFiles();
}

bool PatchBrowser::Draw()
{
    ImGui::PushID("PatchBrowser");
    ImGui::BeginGroup();

    bool changed = false;

    if (ImGui::ArrowButton("Prev", ImGuiDir_Left)) {
        changed = Prev();
    }

    ImGui::SameLine();

    if (ImGui::ArrowButton("Next", ImGuiDir_Right)) {
        changed = Next();
    }

    if (ImGui::BeginListBox("##listbox", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
    {
        for (uint32_t idx = 0; idx < NumFiles(); idx++)
        {
            const bool isSelected = (m_currentIdx && *m_currentIdx == idx);
            if (ImGui::Selectable(m_files.at(idx).filename.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    SetCurrent(idx);
                }
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }

    for (uint32_t idx = 0; idx < NumFiles(); ++idx) {

    }

    ImGui::EndGroup();
    ImGui::PopID();

    return changed;
}

uint32_t PatchBrowser::NumFiles() const
{
    return static_cast<uint32_t>(m_files.size());
}

bool PatchBrowser::Next()
{
    if (!m_currentIdx) {
        return false;
    }

    if (*m_currentIdx + 1u >= NumFiles()) {
        return false;
    }

    (*m_currentIdx)++;
    LoadCurrent();
    return true;
}

void PatchBrowser::LoadCurrent() const
{
    if (!m_currentIdx) {
        return;
    }
    const auto& currentFile = m_files.at(*m_currentIdx);
    m_io->Reset();
    m_io->LoadFile(currentFile.path.string());
}

bool PatchBrowser::Prev()
{
    if (!m_currentIdx || *m_currentIdx == 0) {
        return false;
    }

    (*m_currentIdx)--;
    LoadCurrent();
    return true;
}

bool PatchBrowser::SetCurrent(uint32_t patchIdx)
{
    m_currentIdx = patchIdx;
    LoadCurrent();
    return true;
}

void PatchBrowser::SaveCurrent() const
{
    if (!m_currentIdx) {
        return;
    }
    const auto& currentFile = m_files.at(*m_currentIdx);
    m_io->SaveFile(currentFile.path.string());
}

bool PatchBrowser::CreateNew(const std::string& name)
{
    if (FindFile(name)) {
        return false;
    }
    auto newPath = m_patchDir / fs::path(name) / ".json";
    m_io->SaveFile(newPath);

    FileEntry newEntry {
        .path = newPath,
        .filename = name
    };

    m_files.push_back(newEntry);
    m_currentIdx = NumFiles() - 1;
    SortFiles();
    return true;
}

void PatchBrowser::SortFiles()
{
    std::string activeFilename;
    if (m_currentIdx) {
        activeFilename = m_files.at(*m_currentIdx).filename;
    }
    std::stable_sort(
        m_files.begin(), m_files.end(), 
        [] (const auto& a, const auto& b) {
            return a.filename < b.filename;
        });

    if (m_currentIdx) {
        uint32_t res;
        if (FindFile(activeFilename, &res)) {
            m_currentIdx = res;
        } else {
            m_currentIdx = std::nullopt;
        }
    }
}

bool PatchBrowser::FindFile(const std::string& filename, uint32_t* idxOut)
{
    auto it = std::find_if(m_files.begin(), m_files.end(), [&filename] (const auto& x) {
        return x.filename == filename;
    });

    if (it == m_files.end()) {
        return false;
    }

    if (idxOut) {
        *idxOut = it - m_files.begin();
    }

    return true;
}