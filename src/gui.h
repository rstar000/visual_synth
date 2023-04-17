#pragma once

#include <stdio.h>

#include <memory>
#include <thread>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "imgui_node_editor.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <imgui_internal.h>

#include "file_menu.h"
#include "midi.h"
#include "multigraph.h"
#include "node_factory.h"
#include "note.h"
#include "synth.h"

namespace ed = ax::NodeEditor;

class Gui {
   public:
    Gui(Synth* synth);
    ~Gui();

    void Start() {
        if (_running) {
            return;
        }
        _running = true;
        _thread = std::thread(&Gui::Spin, this);
        SPDLOG_INFO("[Gui] Thread started");
    }

    void Stop() {
        if (!_running) {
            return;
        }

        _running = false;
        if (_thread.joinable()) {
            _thread.join();
        }

        SPDLOG_INFO("[Gui] Thread stopped");
    }

    // Blocks calling thread!
    void Spin();

   private:
    void InitWindow();
    void DrawFrame();
    // void DrawToolbar();
    void ShowContextMenu();

    SDL_Window* window;
    SDL_GLContext gl_context;

    Synth* m_synth;

    KeyboardInput key_input;
    FileMenu file_menu;

    ax::NodeEditor::EditorContext* g_Context = nullptr;
    bool g_FirstFrame = true;

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color;

    std::thread _thread;
    bool _running = false;
};