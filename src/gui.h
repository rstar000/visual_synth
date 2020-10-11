#pragma once

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>            // Initialize with glewInit()

#include "keyboard.h"

class Gui {
 public:
  Gui();
  ~Gui();

  // Blocks calling thread
  void Spin();

  const KeyboardState& GetKeyboardState() {
    return kb_state_;
  }

 private:
  void InitWindow();

  SDL_Window* window_;
  SDL_GLContext gl_context_;
  KeyboardState kb_state_;
};
