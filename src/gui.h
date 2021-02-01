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
#include <GL/glew.h>            // Initialize with glewInit()

#include "bridge.h"
#include "keyboard.h"

namespace ed = ax::NodeEditor;

struct LinkInfo
{
    ed::LinkId Id;
    ed::PinId  InputId;
    ed::PinId  OutputId;
};


class Gui {
 public:
  Gui(BridgePtr bridge);
  ~Gui();

  // Blocks calling thread!
  void Spin();

  std::shared_ptr<const KeyboardState> GetKeyboardState() {
    return key_state_;
  }

 private:
  void InitWindow();
  void DrawFrame();

  BridgePtr bridge_;
  SDL_Window* window_;
  SDL_GLContext gl_context_;
  std::shared_ptr<KeyboardState> key_state_;
  ax::NodeEditor::EditorContext* g_Context = nullptr;
  bool g_FirstFrame = true;

  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color;
};
