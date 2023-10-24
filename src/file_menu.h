#pragma once

#include <fstream>
#include <optional>
#include <string>

#include "json.hpp"
#include "multigraph.h"
#include "portable-file-dialogs.h"

struct FileMenu {
    FileMenu(const GraphIO* io) : m_io(io) {}

    void SaveAs() {
        std::string destination = pfd::save_file("Select a file").result();
        if (destination.empty()) {
            SPDLOG_ERROR("[FileMenu] File not selected");
            return;
        }

        SPDLOG_INFO("[FileMenu] SaveAs: {}", destination);
        m_io->SaveFile(destination);
        filename = destination;
    }

    void Save() {
        if (!filename) {
            SaveAs();
        } else {
            m_io->SaveFile(*filename);
        }
    }

    void Load() { Import(); }

    auto& GetFilename() const { return filename; }

    void Import() {
        auto selection = pfd::open_file("Select a file").result();
        if (selection.empty()) {
            SPDLOG_ERROR("[FileMenu] File not selected");
            return;
        }

        SPDLOG_INFO("[FileMenu] Import: {}", selection[0]);
        m_io->LoadFile(selection[0]);
    }
    
    void Reset() {
        SPDLOG_INFO("[FileMenu] Reset");
        m_io->Reset();
    }

   private:
    std::optional<std::string> filename;
    const GraphIO* m_io;
};
