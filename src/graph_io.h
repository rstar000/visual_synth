#pragma once

#include <optional>
#include <string>
#include <fstream>

#include "multigraph.h"
#include "json.hpp"
#include "portable-file-dialogs.h"

struct FileMenu {
  FileMenu(std::shared_ptr<Multigraph> graph, std::shared_ptr<NodeFactory> factory) 
    : graph(graph), factory(factory) {}
  
  void SaveAs() {
    std::string destination = pfd::save_file("Select a file").result();
    if (destination.empty()) {
      std::cout << "Didn't select a file" << std::endl;
      return;
    }
    
    SaveImpl(destination);
    filename = destination;
  }

  void Save() {
    if (!filename) {
      SaveAs();
    } else {
      SaveImpl(*filename);
    }
  }

  void Load() {
    Import();
  }
  
  auto& GetFilename() const { return filename; }

  void Import() {
    auto selection = pfd::open_file("Select a file").result();
    if (selection.empty()) {
      std::cout << "Didn't select a file" << std::endl;
      return;
    }
    
    nlohmann::json j;
    std::ifstream f(selection[0]);
    f >> j;
    
    LoadGraph(*graph, j, *factory);
  }
  
 private:
  void SaveImpl(const std::string& dst) {
    std::ofstream f(dst);
    
    auto j = nlohmann::json::object();
    SaveGraph(*graph, j);

    f << j;
  }

  std::shared_ptr<Multigraph> graph;
  std::shared_ptr<NodeFactory> factory;
  std::optional<std::string> filename;
};