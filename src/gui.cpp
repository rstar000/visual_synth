#include "gui.h"

Gui::Gui(BridgePtr bridge)
    : bridge_(bridge)
    , key_state_(std::make_shared<KeyboardState>()) {
  InitWindow();
}

Gui::~Gui() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
  ed::DestroyEditor(g_Context);
}


void Gui::InitWindow() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
  {
      printf("Error: %s\n", SDL_GetError());
      exit(1);
  }

  g_Context = ed::CreateEditor();

  // Decide GL+GLSL versions
  // GL 3.2 Core + GLSL 150
  const char* glsl_version = "#version 150";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window_ = SDL_CreateWindow("Synthesizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window_);
  SDL_GL_MakeCurrent(window_, gl_context_);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  bool err = glewInit() != GLEW_OK;
  if (err)
  {
      fprintf(stderr, "Failed to initialize OpenGL loader!\n");
      exit(1);
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer bindings
  ImGui_ImplSDL2_InitForOpenGL(window_, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);
}

void Gui::Spin() {
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Main loop
    bool done = false;
    while (!done) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window_))
                done = true;

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
              key_state_->ProcessEvent(event);
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window_);
        ImGui::NewFrame();
        
        DrawFrame();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window_);
  }
}

void ImGuiEx_BeginColumn()
{
    ImGui::BeginGroup();
}

void ImGuiEx_NextColumn()
{
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
}

void ImGuiEx_EndColumn()
{
    ImGui::EndGroup();
}

void Gui::DrawFrame() {

  // ed::Begin("My Editor");
  // int uniqueId = 1;
  // Start drawing nodes.
  // ed::BeginNode(uniqueId++);
  //     ImGui::Text("Node A");
  //     ed::BeginPin(uniqueId++, ed::PinKind::Input);
  //         ImGui::Text("-> In");
  //     ed::EndPin();
  //     ImGui::SameLine();
  //     ed::BeginPin(uniqueId++, ed::PinKind::Output);
  //         ImGui::Text("Out ->");
  //     ed::EndPin();
  // ed::EndNode();

    ImGui::Separator();

    ed::SetCurrentEditor(g_Context);

    // Start interaction with editor.
    ed::Begin("My Editor", ImVec2(0.0, 0.0f));

    int uniqueId = 1;

    //
    // 1) Commit known data to editor
    //

    // Submit Node A
    ed::NodeId nodeA_Id = uniqueId++;
    ed::PinId  nodeA_InputPinId = uniqueId++;
    ed::PinId  nodeA_OutputPinId = uniqueId++;

    if (g_FirstFrame)
        ed::SetNodePosition(nodeA_Id, ImVec2(10, 10));
    
    for (auto& [node_id, pins] : bridge_->node_id_to_pins) {
      NodePtr node = bridge_->node_id_to_ptr[node_id];
      ed::NodeId g_node_id = node_id;
      ed::BeginNode(g_node_id);
          std::stringstream ss;
          ss << "Node " << node_id;
          std::string node_name = ss.str();
          ImGui::Text(node_name.c_str());
          ImGuiEx_BeginColumn();
            for (auto pin_id : pins.input_ids) {
              int input_idx = pins.input_pin_to_input_idx[pin_id];
              auto input = node->GetInputByIndex(input_idx);

              ed::PinId g_pin_id = pin_id;
              ed::BeginPin(g_pin_id, ed::PinKind::Input);
                  ImGui::Text(input->name.c_str());
              ed::EndPin();
            }
          ImGuiEx_NextColumn();
            for (auto pin_id : pins.output_ids) {
              int output_idx = pins.output_pin_to_output_idx[pin_id];
              auto output = node->GetOutputByIndex(output_idx);

              ed::PinId g_pin_id = pin_id;
              ed::BeginPin(g_pin_id, ed::PinKind::Output);
                  ImGui::Text(output->name.c_str());
              ed::EndPin();
            }
          ImGuiEx_EndColumn();
      ed::EndNode();

    }

    // if (g_FirstFrame)
    //     ed::SetNodePosition(nodeB_Id, ImVec2(210, 60));


    // Submit Links
    // for (auto& linkInfo : g_Links)
    //     ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

    //
    // 2) Handle interactions
    //

    // Handle creation action, returns true if editor want to create new object (node or link)
    if (ed::BeginCreate())
    {
        ed::PinId inputPinId, outputPinId;
        if (ed::QueryNewLink(&inputPinId, &outputPinId))
        {
            // QueryNewLink returns true if editor want to create new link between pins.
            //
            // Link can be created only for two valid pins, it is up to you to
            // validate if connection make sense. Editor is happy to make any.
            //
            // Link always goes from input to output. User may choose to drag
            // link from output pin or input pin. This determine which pin ids
            // are valid and which are not:
            //   * input valid, output invalid - user started to drag new ling from input pin
            //   * input invalid, output valid - user started to drag new ling from output pin
            //   * input valid, output valid   - user dragged link over other pin, can be validated

            if (inputPinId && outputPinId) // both are valid, let's accept link
            {
                // ed::AcceptNewItem() return true when user release mouse button.
                if (ed::AcceptNewItem())
                {
                    // Since we accepted new link, lets add one to our list of links.
                    g_Links.push_back({ ed::LinkId(g_NextLinkId++), inputPinId, outputPinId });

                    // Draw new link.
                    ed::Link(g_Links.back().Id, g_Links.back().InputId, g_Links.back().OutputId);
                }

                // You may choose to reject connection between these nodes
                // by calling ed::RejectNewItem(). This will allow editor to give
                // visual feedback by changing link thickness and color.
            }
        }
    }
    ed::EndCreate(); // Wraps up object creation action handling.


    // Handle deletion action
    if (ed::BeginDelete())
    {
        // There may be many links marked for deletion, let's loop over them.
        ed::LinkId deletedLinkId;
        while (ed::QueryDeletedLink(&deletedLinkId))
        {
            // If you agree that link can be deleted, accept deletion.
            if (ed::AcceptDeletedItem())
            {
                // Then remove link from your data.
                for (auto& link : g_Links)
                {
                    if (link.Id == deletedLinkId)
                    {
                        g_Links.erase(&link);
                        break;
                    }
                }
            }

            // You may reject link deletion by calling:
            // ed::RejectDeletedItem();
        }
    }
    ed::EndDelete(); // Wrap up deletion action



    // End of interaction with editor.
    ed::End();

    if (g_FirstFrame)
        ed::NavigateToContent(0.0f);

    ed::SetCurrentEditor(nullptr);

    g_FirstFrame = false;

    // ImGui::ShowMetricsWindow();


  // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
  //if (show_demo_window)
  //    ImGui::ShowDemoWindow(&show_demo_window);

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
  {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

      ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
      ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

      if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
          counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
  }

  // 3. Show another simple window.
  if (show_another_window)
  {
      ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me"))
          show_another_window = false;
      ImGui::End();
  }
}