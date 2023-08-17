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
            std::cout << "Didn't select a file" << std::endl;
            return;
        }

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

        m_io->LoadFile(selection[0]);
    }
    
    void Reset() {
        m_io->Reset();
    }

   private:
    std::optional<std::string> filename;
    const GraphIO* m_io;
};