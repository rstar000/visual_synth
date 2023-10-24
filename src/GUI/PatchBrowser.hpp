#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>
#include <imgui_internal.h>

#include "util.h"
#include "json.hpp"
#include "multigraph.h"
#include "portable-file-dialogs.h"

#include <optional>
#include <vector>
#include <string>
#include <filesystem>

struct FileEntry
{
    std::filesystem::path path;
    std::string filename;
};


struct PatchBrowser
{
    PatchBrowser(const GraphIO* io);
    bool Next();
    bool Prev();
    bool SetCurrent(uint32_t patchIdx);
    bool CreateNew(const std::string& name);

    bool Draw();
    uint32_t NumFiles() const;

private:
    void LoadCurrent() const;
    void SaveCurrent() const;
    void SortFiles();
    void Reload();
    bool FindFile(const std::string& filename, uint32_t* idxOut = nullptr);

    std::filesystem::path m_patchDir;
    std::vector<FileEntry> m_files;
    std::optional<uint32_t> m_currentIdx;

    const GraphIO* m_io;
};
