#pragma once

#include <memory>
#include <stdio.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <SDL2/SDL.h>
#include <GL/glew.h>            // Initialize with glewInit()

#include "keyboard.h"

class Gui {
 public:
  Gui();
  ~Gui();

  // Blocks calling thread
  void Spin();

  std::shared_ptr<const KeyboardState> GetKeyboardState() {
    return key_state_;
  }

 private:
  void InitWindow();

  SDL_Window* window_;
  SDL_GLContext gl_context_;
  std::shared_ptr<KeyboardState> key_state_;
};
