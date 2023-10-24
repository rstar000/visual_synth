#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <imgui_internal.h>
#include <stdio.h>

#include <memory>
#include <thread>

#include "GUI/PatchBrowser.hpp"
#include "file_menu.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "midi.h"
#include "multigraph.h"
#include "node_factory.h"
#include "note.h"
#include "synth.h"
#include "util.h"

class Gui {
   public:
    Gui(Synth&, MidiInput&);
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
    void DrawMainMenu();
    void DrawFrame();
    void DrawGraph();
    void DrawSidebar();
    void DrawMidiSettings();
    // void DrawToolbar();
    void ShowContextMenu();

    SDL_Window* window;
    SDL_GLContext gl_context;

    Synth* m_synth;
    MidiInput* m_midi;

    KeyboardInput key_input;
    FileMenu file_menu;
    PatchBrowser m_patchBrowser;

    GridUI* m_ui;

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color;

    std::thread _thread;
    bool _running = false;

    bool m_showMidiSettings = false;
};
