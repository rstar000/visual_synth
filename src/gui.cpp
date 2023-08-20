#include "gui.h"
// #include "portable-file-dialogs.h"
#include "misc/freetype/imgui_freetype.h"

Gui::Gui(Synth* synth)
    : m_synth(synth),
      file_menu(m_synth->GetIO()),
      key_input(m_synth->GetTracker()),
      m_ui(m_synth->GetGridUI()) {
    InitWindow();
}

Gui::~Gui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Gui::InitWindow() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
        0) {
        printf("Error: %s\n", SDL_GetError());
        exit(1);
    }

    // Decide GL+GLSL versions
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              1280, 720, window_flags);
    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);  // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.Fonts->AddFontFromFileTTF("../res/NotoSans-Regular.ttf", 24.0f);
    // Load color font
    // static ImWchar ranges[] = {0x1, 0x1FFFF, 0};
    // static ImFontConfig cfg;
    // cfg.OversampleH = cfg.OversampleV = 1;
    // cfg.MergeMode = true;
    // cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
    // io.Fonts->AddFontFromFileTTF("../res/seguiemj.ttf", 24.0f, &cfg, ranges);
}

void Gui::Spin() {
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    _running = true;

    // Main loop
    size_t frame_idx = 0;
    while (_running) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
        // tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data
        // to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
        // data to your main application. Generally you may always pass all
        // inputs to dear imgui, and hide them from your application based on
        // those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) _running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                _running = false;

            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                auto sym = event.key.keysym.sym;
                key_input.ProcessKey(sym, true);
            }

            if (event.type == SDL_KEYUP) {
                auto sym = event.key.keysym.sym;
                key_input.ProcessKey(sym, false);
            }
        }

        if (!_running) {
            return;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        DrawFrame();

        // Rendering
        ++frame_idx;
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w,
                     clear_color.y * clear_color.w,
                     clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
}

void ImGuiEx_BeginColumn() { ImGui::BeginGroup(); }

void ImGuiEx_NextColumn() {
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
}

void ImGuiEx_EndColumn() { ImGui::EndGroup(); }

void Gui::DrawFrame() {
    Multigraph* graph = m_synth->GetGraph();
    ImGui::Separator();
    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Begin("GridUI");

    ImGui::BeginGroup();
    float sidebarWidth = 250.0f;
    ImVec2 winSize = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("Grid", ImVec2{winSize.x - sidebarWidth,0}, true);
    m_ui->CanvasBegin("canvas0", [this] () {ShowContextMenu();});

    auto& io = ImGui::GetIO();

    int num_traversed = 0;
    auto& nodes = graph->GetNodes();
    auto& pins = graph->GetPins();
    auto& links = graph->GetLinks();

    const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    auto draw_list = ImGui::GetWindowDrawList();

    // Draw nodes

    for (auto& [node_id, wrapper] : nodes) {
        auto& node = wrapper.node;
        ++num_traversed;

        m_ui->BeginNode(node->GetDisplayName().c_str(), node_id, &wrapper.attrs.pos, node->GetShape());
        node->Draw();

        // Draw pins
        auto& nodePins = pins.node_to_pins.at(node_id);
        for (uint32_t inputIdx = 0; inputIdx < nodePins.inputs.size(); ++inputIdx) {
            pin_id_t pinId = nodePins.inputs.at(inputIdx);
            uint32_t componentIdx = node->GetInputByIndex(inputIdx)->componentIdx;
            const auto& component = node->GetLayout().GetComponent(componentIdx);
            m_ui->AddPin(pinId, PinKind::kIn, component.Rect());
        }

        for (uint32_t outputIdx = 0; outputIdx < nodePins.outputs.size(); ++outputIdx) {
            pin_id_t pinId = nodePins.outputs.at(outputIdx);
            uint32_t componentIdx = node->GetOutputByIndex(outputIdx)->componentIdx;
            const auto& component = node->GetLayout().GetComponent(componentIdx);
            m_ui->AddPin(pinId, PinKind::kOut, component.Rect());
        }

        m_ui->EndNode();
    }

    // Draw links
    for (auto& [link_id, pins] : links.link_id_to_pins) {
        m_ui->AddLink(pins.first, pins.second);
    }

    m_ui->HandleDelete([&graph] (uint32_t deletedNodeId) {
        graph->GetAccess()->RemoveNode(deletedNodeId);
    });

    m_ui->HandleLinkQuery([&graph] (GridUI::LinkQuery query) {
        if (graph->LinkExists(query.pinSrc, query.pinDst)) {
            graph->GetAccess()->RemoveLink(query.pinSrc, query.pinDst);
        } else if (graph->CanAddLink(query.pinSrc, query.pinDst)) {
            link_id_t newLinkId = 0;
            graph->GetAccess()->AddLink(query.pinSrc, query.pinDst, &newLinkId, true);
        }
    }); 

    m_ui->CanvasEnd();

    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::BeginChild("Sidebar", ImVec2(0.0f, 0.0f), true);

        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Description"))
            {
                ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Details"))
            {
                ImGui::Text("ID: 0123456789");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

    ImGui::EndChild();
    ImGui::EndGroup();
    ImGui::EndGroup();
    ImGui::End();

    return;
}

// void Gui::DrawToolbar() {
//   bool playing = audio_thread->IsPlaying();
//   static const char* play_label = "|>";
//   static const char* pause_label = "||";
//   static const char* rec_label = "o";

//   const auto btn_size = ImVec2(100, 100);

//   const char* play_pause_label = pause_label;
//   if (playing) {
//     play_pause_label = play_label;
//   }

//   ImGui::BeginGroup();

//   if (ImGui::Button(play_pause_label, btn_size)) {
//     if (playing) {
//       audio_thread->Stop();
//     } else {
//       audio_thread->Start();
//     }
//   }

//   ImGui::SameLine();

//   if (ImGui::Button("Save", btn_size)) {
//     file_menu.Save();
//   }

//   ImGui::SameLine();

//   if (ImGui::Button("Import", btn_size)) {
//     file_menu.Import();
//   }

//   ImGui::SameLine();

//   ImGui::Text("%.3f", audio_thread->GetTimestamp());

//   ImGui::EndGroup();
// }

void Gui::ShowContextMenu() {
    auto openPopupPosition = ImGui::GetMousePosOnOpeningCurrentPopup();
    auto gridPos = m_ui->ScreenPosToGridPos(openPopupPosition);
        ImGui::Text("Menu");
        ImGui::Separator();

        if (ImGui::BeginMenu("File")) {
            auto& filename = file_menu.GetFilename();
            if (filename) {
                ImGui::TextWrapped("Current file: %s", filename->c_str());
                ImGui::Separator();
            }

            if (ImGui::MenuItem("Load JSON")) {
                file_menu.Load();
            }

            if (filename && ImGui::MenuItem("Save")) {
                file_menu.Save();
            }

            if (ImGui::MenuItem("Save As")) {
                file_menu.SaveAs();
            }

            if (ImGui::BeginMenu("Reset")) {
                if (ImGui::MenuItem("Yes")) {
                    file_menu.Reset();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Create node")) {
            const NodeFactory* factory = m_synth->GetFactory();
            auto& nodes_by_category = factory->GetNodesByCategory();
            auto& display_names = factory->GetDisplayNames();
            for (auto& [category, category_name] : factory->GetCategoryNames()) {
                if (!nodes_by_category.contains(category)) {
                    continue;
                }
                if (ImGui::BeginMenu(category_name.c_str())) {
                    for (auto& type : MapGetConstRef(nodes_by_category, category)) {
                        auto& display_name = MapGetConstRef(display_names, type);
                        if (ImGui::MenuItem(display_name.c_str())) {
                            NodeWrapper wrapper;
                            wrapper.node = factory->CreateNode(type);
                            wrapper.attrs.is_placed = true;
                            wrapper.attrs.pos.x = static_cast<int>(gridPos.x / GRID_STEP);
                            wrapper.attrs.pos.y = static_cast<int>(gridPos.y / GRID_STEP);

                            int new_node_id =
                                m_synth->GetGraph()->GetAccess()->AddNode(
                                    std::move(wrapper));
                        }
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenu();
        }
}
