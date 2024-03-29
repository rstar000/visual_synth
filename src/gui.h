#pragma once

#include <memory>
#include <stdio.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "imgui_node_editor.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "multigraph.h"
#include "graph_io.h"
#include "node_factory.h"
#include "audio_thread.h"

namespace ed = ax::NodeEditor;

struct LinkInfo
{
  ed::LinkId Id;
  ed::PinId  InputId;
  ed::PinId  OutputId;
};


class Gui {
 public:
  Gui(
    std::shared_ptr<Multigraph> graph,
    std::shared_ptr<NodeFactory> factory,
    std::shared_ptr<AudioThread> audio_thread);
  ~Gui();

  // Blocks calling thread!
  void Spin();

  // std::shared_ptr<const KeyboardState> GetKeyboardState() {
  //   return key_state_;
  // }

 private:
  void InitWindow();
  void DrawFrame();
  void DrawToolbar();
  void ShowContextMenu();

  SDL_Window* window;
  SDL_GLContext gl_context;
  // std::shared_ptr<KeyboardState> key_state_;
  std::shared_ptr<Multigraph> graph;
  std::shared_ptr<NodeFactory> factory;
  std::shared_ptr<AudioThread> audio_thread;
  
  FileMenu file_menu;
  
  ax::NodeEditor::EditorContext* g_Context = nullptr;
  bool g_FirstFrame = true;

  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color;
};

