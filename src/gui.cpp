#include "gui.h"
// #include "portable-file-dialogs.h"
#include "misc/freetype/imgui_freetype.h"

Gui::Gui(Synth* synth)
    : m_synth(synth),
      file_menu(m_synth->GetIO()),
      key_input(m_synth->GetTracker()) {
    InitWindow();
}

Gui::~Gui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ed::DestroyEditor(g_Context);
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
    g_Context = ed::CreateEditor();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.Fonts->AddFontFromFileTTF("../res/NotoSans-Regular.ttf", 24.0f);
    // Load color font
    static ImWchar ranges[] = {0x1, 0x1FFFF, 0};
    static ImFontConfig cfg;
    cfg.OversampleH = cfg.OversampleV = 1;
    cfg.MergeMode = true;
    cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
    io.Fonts->AddFontFromFileTTF("../res/seguiemj.ttf", 24.0f, &cfg, ranges);
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

    // return;
    ed::SetCurrentEditor(g_Context);
    ImGuiWindowFlags wf = ImGuiWindowFlags_None;

    bool ed_open = true;

    if (!ImGui::Begin("editor", &ed_open, wf)) {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }

    // DrawToolbar();
    // Start interaction with editor.
    ed::Begin("My Editor", ImVec2(0.0f, 0.0f));

    if (g_FirstFrame) ed::NavigateToContent(0.0f);

    //
    // 1) Commit known data to editor
    //

    int num_traversed = 0;
    auto& nodes = graph->GetNodes();
    auto& pins = graph->GetPins();
    auto& links = graph->GetLinks();

    const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    const ImU32 pin_color = ImColor(180, 180, 180, 150);
    auto draw_list = ImGui::GetWindowDrawList();

    for (auto& [node_id, wrapper] : nodes) {
        auto& node = wrapper.node;
        ++num_traversed;
        // NodePtr node = attrs.ptr;
        // attrs.position = Position{100 * num_traversed, 0};
        // }
        ed::NodeId g_node_id = node_id;
        auto& node_pins = MapGetConstRef(pins.node_to_pins, node_id);
        ed::BeginNode(g_node_id);
        if (!wrapper.attrs.is_placed) {
            ed::SetNodePosition(
                g_node_id, ImVec2(wrapper.attrs.pos_x, wrapper.attrs.pos_y));
            wrapper.attrs.is_placed = true;
        } else if (ed::GetHoveredNode() == g_node_id) {
            auto pos = ed::GetNodePosition(g_node_id);
            wrapper.attrs.pos_x = pos.x;
            wrapper.attrs.pos_y = pos.y;
        }

        ImGui::BeginGroup();
        ImGui::Text("%s|%s|%d", node->GetDisplayName().c_str(), wrapper.attrs.display_name.c_str(), node_id);
        ImGui::EndGroup();
        ImGui::BeginGroup();
        ImGuiEx_BeginColumn();
        for (int input_idx = 0; input_idx < node_pins.inputs.size();
             ++input_idx) {
            auto pin_id = node_pins.inputs[input_idx];
            auto input = node->GetInputByIndex(input_idx);

            ed::PinId g_pin_id = pin_id;

            ImVec2 p = ImGui::GetCursorScreenPos();
            p.x -= TEXT_BASE_WIDTH;
            p.y += TEXT_BASE_HEIGHT / 2;

            draw_list->AddCircleFilled(p, 5, pin_color, 10);
            ed::BeginPin(g_pin_id, ed::PinKind::Input);
            ed::PinRect(ImVec2(p.x - 5, p.y - 5), ImVec2(p.x + 5, p.y + 5));
            // ImGui::Bullet();
            ImGui::Text("%s", input->name.c_str());
            ed::EndPin();
        }

        ImGuiEx_NextColumn();

        node->Draw();

        ImGuiEx_NextColumn();

        for (int output_idx = 0; output_idx < node_pins.outputs.size();
             ++output_idx) {
            auto pin_id = node_pins.outputs[output_idx];
            auto output = node->GetOutputByIndex(output_idx);

            ed::PinId g_pin_id = pin_id;
            ed::BeginPin(g_pin_id, ed::PinKind::Output);
            ImGui::Text("%s %d", output->name.c_str(), pin_id);
            ImGui::SameLine();
            ImVec2 p = ImGui::GetCursorScreenPos();
            // p.x += TEXT_BASE_WIDTH;
            p.y += TEXT_BASE_HEIGHT / 2;
            ed::PinRect(ImVec2(p.x - 5, p.y - 5), ImVec2(p.x + 5, p.y + 5));

            draw_list->AddCircleFilled(p, 5, pin_color, 10);
            // ImGui::Bullet();
            ed::EndPin();
        }
        ImGuiEx_EndColumn();
        ImGui::EndGroup();
        ed::EndNode();
    }

    // Submit Links
    for (auto& [link_id, pins] : links.link_id_to_pins) {
        ed::LinkId g_link_id = link_id;
        ed::Link(g_link_id, pins.first, pins.second);
    }

    //
    // 2) Handle interactions
    //

    // Handle creation action, returns true if editor want to create new object
    // (node or link)
    if (ed::BeginCreate()) {
        ed::PinId inputPinId = 0, outputPinId = 0;
        if (ed::QueryNewLink(&inputPinId, &outputPinId)) {
            // QueryNewLink returns true if editor want to create new link
            // between pins.
            //
            // Link can be created only for two valid pins, it is up to you to
            // validate if connection make sense. Editor is happy to make any.
            //
            // Link always goes from input to output. User may choose to drag
            // link from output pin or input pin. This determine which pin ids
            // are valid and which are not:
            //   * input valid, output invalid - user started to drag new ling
            //   from input pin
            //   * input invalid, output valid - user started to drag new ling
            //   from output pin
            //   * input valid, output valid   - user dragged link over other
            //   pin, can be validated

            int new_link_id = -1;

            pin_id_t input_pin = static_cast<pin_id_t>(size_t(inputPinId));
            pin_id_t output_pin = static_cast<pin_id_t>(size_t(outputPinId));
            bool res = graph->CanAddLink(input_pin, output_pin);
            if (res && inputPinId &&
                outputPinId)  // both are valid, let's accept link
            {
                // ed::AcceptNewItem() return true when user release mouse
                // button.
                if (ed::AcceptNewItem()) {
                    // Since we accepted new link, lets add one to our list of
                    // links.
                    bool res = graph->GetAccess()->AddLink(
                        input_pin, output_pin, &new_link_id);
                    ed::Link(new_link_id, inputPinId, outputPinId);
                }

                // You may choose to reject connection between these nodes
                // by calling ed::RejectNewItem(). This will allow editor to
                // give visual feedback by changing link thickness and color.
            } else {
                ed::RejectNewItem();
            }
        }
    }
    ed::EndCreate();  // Wraps up object creation action handling.

    // Handle deletion action
    if (ed::BeginDelete()) {
        // There may be many links marked for deletion, let's loop over them.
        ed::LinkId deletedLinkId;
        while (ed::QueryDeletedLink(&deletedLinkId)) {
            // If you agree that link can be deleted, accept deletion.
            if (ed::AcceptDeletedItem()) {
                int link_id = static_cast<int>(size_t(deletedLinkId));
                graph->GetAccess()->RemoveLink(link_id);
            }

            // You may reject link deletion by calling:
            // ed::RejectDeletedItem();
        }

        ed::NodeId deletedNodeId;
        while (ed::QueryDeletedNode(&deletedNodeId)) {
            if (ed::AcceptDeletedItem()) {
                int node_id = static_cast<int>(size_t(deletedNodeId));
                graph->GetAccess()->RemoveNode(node_id);
            }
        }
    }
    ed::EndDelete();  // Wrap up deletion action

    // End of interaction with editor.
    //
    //
    ed::Suspend();
    if (ed::ShowBackgroundContextMenu()) {
        ImGui::OpenPopup("context_menu_popup");
    }
    ed::Resume();

    ed::Suspend();
    ShowContextMenu();
    ed::Resume();

    ed::End();

    if (g_FirstFrame) ed::NavigateToContent(0.0f);

    ed::SetCurrentEditor(nullptr);
    ImGui::End();

    g_FirstFrame = false;

    // ImGui::ShowMetricsWindow();
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
    auto openPopupPosition = ImGui::GetMousePos();
    auto canvas_pos = ed::ScreenToCanvas(openPopupPosition);
    if (ImGui::BeginPopup("context_menu_popup")) {
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
            for (auto& [category, category_name] :
                 factory->GetCategoryNames()) {
                if (ImGui::BeginMenu(category_name.c_str())) {
                    for (auto& type :
                         MapGetConstRef(nodes_by_category, category)) {
                        auto& display_name =
                            MapGetConstRef(display_names, type);
                        if (ImGui::MenuItem(display_name.c_str())) {
                            NodeWrapper wrapper;
                            wrapper.node = factory->CreateNode(type);
                            wrapper.attrs.is_placed = true;
                            wrapper.attrs.pos_x = canvas_pos.x;
                            wrapper.attrs.pos_y = canvas_pos.y;

                            int new_node_id =
                                m_synth->GetGraph()->GetAccess()->AddNode(
                                    std::move(wrapper));
                            ed::SetNodePosition(new_node_id, canvas_pos);
                        }
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }
}
