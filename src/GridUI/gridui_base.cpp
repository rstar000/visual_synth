#include <cmath>
#include <stdio.h>
#include <iostream>
#include <variant>
#include <algorithm>
#include <numeric>
#include <optional>
#include <map>

#include <memory>
#include <thread>
#include <span>
#include <vector>
#include <queue>
#include <stack>
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "imgui_knobs.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <imgui_internal.h>

#include "ui/GridLayout.hpp"
#include "ui/Fader.hpp"
#include "ui/Knob.hpp"
#include "ui/Pin.hpp"

constexpr float GRID_STEP = 100.0f;
constexpr float TITLE_BAR_OFFSET = 20.0f;

ImU32 GenGrey(int x)
{
    return IM_COL32(x, x, x, 255);
}


enum class PinType
{
    kIn, kOut
};

class GridUI
{
    struct DraggedNodeInfo
    {
        uint32_t id;
        ImVec2 dragOrigin;
        ImVec2 startPosition;
        ImVec2 endPosition;
    };

    ImVec2 m_scroll;
    ImRect m_bounds;
    bool m_enableGrid;
    bool m_enableContextMenu;
    ImVec2 m_textSize;

    // Rect of current node
    ImRect m_currentRect;
    // Rect of current node title bar
    ImRect m_titleBarRect;
    ImVec2* m_nodeGridPos;
    ImVec2 m_nodeGridSize;

    // Last node being drawn
    uint32_t m_nodeId;

    std::optional<DraggedNodeInfo> m_draggedNode;
    std::optional<uint32_t> m_selectedNode;
    std::optional<uint32_t> m_deletedNode;

    // Pins
    struct PinDescription {
        uint32_t id;
        ImRect bbox;
        PinType type;
    };

    // All added pins by pin id
    std::map<uint32_t, PinDescription> m_pins;
    // Pins of current node
    std::vector<PinDescription> m_currentNodePins;
    std::optional<uint32_t> m_selectedPin;


    // Links
    struct LinkQuery {
        uint32_t pinSrc;
        uint32_t pinDst;
    };

    std::optional<LinkQuery> m_linkQuery;

public:
    GridUI()
        : m_scroll{ImVec2{0.0f, 0.0f}}
        , m_bounds{}
        , m_enableGrid{true}
        , m_enableContextMenu{false}
    {}

    void Init()
    {
    }

    void CanvasEnd()
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->PopClipRect();

        auto& io = ImGui::GetIO();
        if (!io.MouseDown[0]) {
            m_draggedNode = std::nullopt;
        }
        ImGui::PopID();
    }

    void CanvasBegin(const char* label)
    {
        float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        m_textSize = {TEXT_BASE_WIDTH, TEXT_BASE_HEIGHT};

        m_bounds.Min = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
        ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
        m_bounds.Max = m_bounds.Min + canvas_sz;

        // Draw border and background color
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->AddRectFilled(m_bounds.Min, m_bounds.Max, GenGrey(20));
        draw_list->AddRect(m_bounds.Min, m_bounds.Max, GenGrey(255));

        // This will catch our interactions
        ImGui::PushID(label);
        if (!(canvas_sz.x == 0.0f || canvas_sz.y == 0.0f))
        {
            ImGui::SetNextItemAllowOverlap();
            ImGui::InvisibleButton("", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        }
        const bool is_hovered = ImGui::IsItemHovered(); // Hovered
        const bool is_active = ImGui::IsItemActive();   // Held

        const ImVec2 origin = m_bounds.Min + m_scroll; 
        const ImVec2 mouse_pos_in_canvas = io.MousePos - origin;

        // Pan (we use a zero mouse threshold when there's no context menu)
        // You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
        const float mouse_threshold_for_pan = m_enableContextMenu ? -1.0f : 0.0f;
        if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouse_threshold_for_pan)) {
            m_scroll.x += io.MouseDelta.x;
            m_scroll.y += io.MouseDelta.y;
        }
        
        // Left clicked on empty canvas area
        if (is_active && ImGui::IsMouseClicked(0)) {
            m_selectedNode = std::nullopt;
            m_selectedPin = std::nullopt;
        }

        // Context menu (under default mouse threshold)
        ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
        if (m_enableContextMenu && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
            ImGui::OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
        if (ImGui::BeginPopup("context"))
        {
            ImGui::EndPopup();
        }

        // Draw grid + all lines in the canvas
        draw_list->PushClipRect(m_bounds.Min, m_bounds.Max, true);

        ImU32 COLOR_GRAY = GenGrey(80);
        ImU32 COLOR_WHITE = GenGrey(200);

        if (m_enableGrid)
        {
            for (float x = fmodf(m_scroll.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP) {
                float world_x = x - m_scroll.x;
                ImU32 color = COLOR_GRAY;
                if (std::fabs(world_x) < 0.1f) {
                    color = COLOR_WHITE;
                }
                draw_list->AddLine(ImVec2(m_bounds.Min.x + x, m_bounds.Min.y), ImVec2(m_bounds.Min.x + x, m_bounds.Max.y), color);
            }
            for (float y = fmodf(m_scroll.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP) {
                float world_y = y - m_scroll.y;
                ImU32 color = COLOR_GRAY;
                if (std::fabs(world_y) < 0.1f) {
                    color = COLOR_WHITE;
                }
                draw_list->AddLine(ImVec2(m_bounds.Min.x, m_bounds.Min.y + y), ImVec2(m_bounds.Max.x, m_bounds.Min.y + y), color);
            }
        }

        static bool disable_mouse_wheel = false;
        static bool disable_menu = false;

        m_pins.clear();
        m_linkQuery = std::nullopt;
    }

    void BeginNode(const char* label, uint32_t nodeId, ImVec2* position, ImVec2 size)
    {
        ImGui::PushID(label);
        const ImVec2 origin = m_bounds.Min + m_scroll; 
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        m_currentNodePins.clear();

        m_nodeGridPos = position;
        m_nodeGridSize = size;
        m_nodeId = nodeId;
        m_currentRect = ImRect{origin + *position * GRID_STEP, origin + (*position + size) * GRID_STEP};

        // Draw title bar above node content
        ImVec2 title_bar_offset{-1.0f, -TITLE_BAR_OFFSET};
        m_titleBarRect = ImRect{m_currentRect.Min + title_bar_offset, ImVec2{m_currentRect.Max.x + 1.0f, m_currentRect.Min.y}};
        draw_list->AddRectFilled(m_currentRect.Min + title_bar_offset, m_currentRect.Max, IM_COL32(90, 90, 120, 255));
        draw_list->AddRect(m_titleBarRect.Min, m_titleBarRect.Max, IM_COL32(255, 255, 255, 255));

        // Draw window title
        float max_text_width = m_titleBarRect.GetWidth();
        ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true, max_text_width); 
        float label_offset = (TITLE_BAR_OFFSET - label_size.y) * 0.5f;
        ImGui::SetCursorScreenPos(m_titleBarRect.Min + ImVec2{2.0, label_offset});
        ImGui::PushItemWidth(max_text_width);
        const char* text_end = ImGui::FindRenderedTextEnd(label);
        ImGui::TextEx(label, text_end);
        ImGui::PopItemWidth();

        // Draw delete button
        ImGui::SetCursorScreenPos(ImVec2{m_titleBarRect.Max.x - TITLE_BAR_OFFSET, m_titleBarRect.Min.y});
        if (ImGui::ButtonEx("x##close", ImVec2{TITLE_BAR_OFFSET, TITLE_BAR_OFFSET}, ImGuiButtonFlags_PressedOnDoubleClick)) {
            m_deletedNode = nodeId;
            std::cout << "Deleted " << nodeId << std::endl;
        }
    }

    void DrawGridLayout(const char* label, const GridLayout& layout, ImU32 col1u, ImU32 col2u)
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImColor col1 = ImColor(col1u);
        ImColor col2 = ImColor(col2u);
        const auto& rect = m_currentRect;
        ImVec2 scale = rect.GetSize();

        ImVec2 cursorBegin = ImGui::GetCursorScreenPos();
        for (int subIdx = 0; subIdx < layout.NumComponnets(); ++subIdx) {
            const GridComponent& sub = layout.GetComponent(subIdx);
            ImRect subRect = sub.Rect();
            ImVec4 newColor = lerp(ImVec4(col1), ImVec4(col2), static_cast<float>(subIdx) / layout.NumComponnets());
            subRect.Translate(rect.Min);
            draw_list->AddRectFilled(subRect.Min, subRect.Max, ImU32(ImColor(newColor)));
            draw_list->AddText(subRect.GetCenter() - m_textSize / 2.0f, IM_COL32(255, 255, 200, 255), std::to_string(subIdx).c_str());

            // char buf[256];
            // sprintf(buf, "##%s%d", label, subIdx);
            // ImGui::PushID(buf);
            // ImGui::SetCursorScreenPos(subRect.Min);
            // ImGui::SetNextItemAllowOverlap();
            // ImGui::InvisibleButton("", subRect.GetSize());
            // const bool is_hovered = ImGui::IsItemHovered(); // Hovered
            // if (is_hovered)
            // {
            //     // Draw border around hovered cell
            //     draw_list->AddRect(subRect.Min, subRect.Max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
            // }
            // ImGui::PopID();
        }

        ImGui::SetCursorScreenPos(cursorBegin);
    }

    void DrawComponent(const GridLayout& layout, uint32_t subIdx, auto drawFunc)
    {
        const ImRect rect = m_currentRect;
        ImVec2 scale = rect.GetSize();
        const GridComponent& sub = layout.GetComponent(subIdx);
        ImRect subScaled = sub.Rect();
        subScaled.Translate(rect.Min);

        drawFunc(subScaled);
    }

    ImRect GetNodeRect() const {
        return m_currentRect;
    }

    void EndNode()
    {
        // Draw node frame
        auto& io = ImGui::GetIO();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImU32 DEFAULT_FRAME_COLOR = GenGrey(255);
        ImU32 SELECTED_FRAME_COLOR = IM_COL32(230, 150, 0, 255);
        bool is_selected = m_selectedNode && *m_selectedNode == m_nodeId;
        ImU32 frameColor = is_selected ? SELECTED_FRAME_COLOR : DEFAULT_FRAME_COLOR;

        draw_list->AddRect(m_currentRect.Min - ImVec2{1.0f, 1.0f}, m_currentRect.Max + ImVec2{1.0f, 1.0f}, frameColor, 0.0f, 0, 1.0f);

        ImGui::SetCursorScreenPos(m_currentRect.Min);
        ImGui::SetNextItemAllowOverlap();
        ImGui::InvisibleButton("btn_frame", m_currentRect.GetSize());

        // Handle title bar interaction
        ImGui::SetCursorScreenPos(m_titleBarRect.Min);
        ImGui::SetNextItemAllowOverlap();
        ImGui::InvisibleButton("btn_titlebar", m_titleBarRect.GetSize());
        bool is_active = ImGui::IsItemActive();
        if (is_active) {
            // ImGui::GetForegroundDrawList()->AddLine(io.MouseClickedPos[0], io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f); // Draw a line between the button and the mouse cursor
            if (!m_draggedNode) {
                m_draggedNode = DraggedNodeInfo {
                    .id = m_nodeId,
                    .dragOrigin = io.MouseClickedPos[0],
                    .startPosition = *m_nodeGridPos
                };
            }
        }

        // Set current node as selected
        if (is_active && ImGui::IsMouseClicked(0)) {
            m_selectedNode = m_nodeId;
        }

        if (m_draggedNode && m_draggedNode->id == m_nodeId)
        {
            // Current node is dragged: recompute end pos
            ImVec2 curMousePos = io.MousePos;
            ImVec2 gridDelta = {
                static_cast<int>((curMousePos.x - m_draggedNode->dragOrigin.x) / (GRID_STEP * 0.5f)),
                static_cast<int>((curMousePos.y - m_draggedNode->dragOrigin.y) / (GRID_STEP * 0.5f))
            };
            gridDelta *= 0.5f;

            m_draggedNode->endPosition = m_draggedNode->startPosition + gridDelta;
            *m_nodeGridPos = m_draggedNode->endPosition;
        }

        // Draw node pins
        for (auto& pin : m_currentNodePins) {
            ImGui::PushID(pin.id);
            ImGui::SetCursorScreenPos(pin.bbox.Min);
            ImGui::SetNextItemAllowOverlap();
            ImGui::InvisibleButton("", pin.bbox.GetSize());

            bool is_active = ImGui::IsItemActive();
            bool is_hovered = ImGui::IsItemHovered();
            ImGui::PopID();

            if (is_hovered) {
                if (pin.type == PinType::kIn) {
                    DrawInPin("", pin.bbox, IM_COL32(50, 100, 50, 255));
                } else {
                    DrawOutPin("", pin.bbox, IM_COL32(50, 100, 50, 255));
                }
                std::cout << "Pin " << pin.id << std::endl;
            }
            
            if (m_selectedPin && *m_selectedPin == pin.id) {
                if (pin.type == PinType::kIn) {
                    DrawInPin("", pin.bbox, IM_COL32(200, 50, 200, 255));
                } else {
                    DrawOutPin("", pin.bbox, IM_COL32(200, 50, 200, 255));
                }

            }

            bool link_add_mode = io.KeyCtrl;
            if (is_active) {
                std::cout << "Pin " << pin.id << " active" << std::endl;
                if (m_selectedPin && link_add_mode) {
                    m_linkQuery = LinkQuery {
                        .pinSrc = *m_selectedPin,
                        .pinDst = pin.id
                    };
                    std::cout << "Added link query: " << m_linkQuery->pinSrc << ' ' << m_linkQuery->pinDst << std::endl;
                    m_selectedPin = std::nullopt;
                } else {
                    m_selectedPin = pin.id;
                }
            }

        }

        ImGui::PopID();
    }

    void HandleDrag(auto dragCallback) {
        if (m_draggedNode) {
            dragCallback(*m_draggedNode);
        }
    }

    void HandleDelete(auto deleteCallback) {
        if (m_deletedNode) {
            deleteCallback(*m_deletedNode);
        }
    }

    void AddPin(uint32_t pinId, PinType pinType, ImRect bbox)
    {
        m_currentNodePins.push_back(PinDescription{
            .id = pinId,
            .bbox = bbox,
            .type = pinType
        });

        m_pins[pinId] = m_currentNodePins.back();
    }

    void AddLink(uint32_t pinSrc, uint32_t pinDst)
    {
        auto& pinA = m_pins.at(pinSrc);
        auto& pinB = m_pins.at(pinDst);

        bool is_selected = false;
        if (m_selectedPin && (*m_selectedPin == pinSrc || *m_selectedPin == pinDst)) {
            is_selected = true;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddLine(pinA.bbox.GetCenter(), pinB.bbox.GetCenter(), IM_COL32(200, 200, 0, 160), 3.0f);
    }

    bool QueryLink(uint32_t* pinSrc, uint32_t* pinDst) 
    {
        if (!m_linkQuery) {
            return false;
        }

        *pinSrc = m_linkQuery->pinSrc;
        *pinDst = m_linkQuery->pinDst;
        return true;
    }
};

struct Node {
    std::unique_ptr<GridLayout> layout;
    uint32_t id;
    ImVec2 size;
    ImVec2 position;

    const GridLayout& GetLayout() const {
        return *layout;
    }

    virtual void Draw(GridUI* ui) {}
};

struct OscNode : public Node {
    float x = 0.0f;
    float y = 0.0f;
    OscNode() {
        size = ImVec2(3, 2);

        std::array<int, 3> w0 = {1, 4, 1};
        std::array<int, 2> w1 = {5, 1};
        layout = std::make_unique<GridLayout>(GridLayoutBuilder().
            AddColumnsEx(3, w0)
                .Push(0).AddRows(3).Pop()
                .Push(2).AddRows(3).Pop()
                .Push(1).AddColumns(2)
                    .Push(0).AddRowsEx(2, w1).Pop()
                    .Push(1).AddRowsEx(2, w1).Pop()
            .Build().Scale(size * GRID_STEP));
        
        position = ImVec2(1, 1);
    }

    void DrawComponent(ImVec2 origin, uint32_t compIdx, auto compFunc)
    {
        const auto& component = layout->GetComponent(compIdx);
        ImRect dstRect = component.Rect();
        dstRect.Translate(origin);
        compFunc(dstRect);
    }

    void Draw(GridUI* ui) override {
        ImVec2 origin = ui->GetNodeRect().Min;
        DrawComponent(origin, 3, [this] (ImRect rect) {
            DrawFader("fader0", rect, x, 0.0f, 100.0f, "%0.1f");
            if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
            {
                ImGui::Text("Context menu");
                if (ImGui::Button("Close"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
        });

        DrawComponent(origin, 5, [this] (ImRect rect) {
            DrawKnob("knob0", rect, y, 0.0f, 100.0f, 50.0f, NULL, true);
        });

        DrawComponent(origin, 0, [this, &ui] (ImRect rect) {
            DrawInPin("pin0", rect, IM_COL32(100, 0, 0, 255));
            ui->AddPin(id * 10 + 1, PinType::kIn, rect);
        });

        DrawComponent(origin, 1, [this, &ui] (ImRect rect) {
            DrawInPin("pin0", rect, IM_COL32(100, 0, 0, 255));
            ui->AddPin(id * 10 + 2, PinType::kIn, rect);
        });

        DrawComponent(origin, 7, [this, &ui] (ImRect rect) {
            DrawOutPin("pin0", rect, IM_COL32(100, 0, 0, 255));
            ui->AddPin(id * 10 + 3, PinType::kOut, rect);
        });
    }
};

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
        0) {
        printf("Error: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Window* window;
    SDL_GLContext gl_context;
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

    GridUI ui{};
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    bool running = true;
    bool show_demo_window = true;


    GridLayout gridA = GridLayoutBuilder()
        .AddColumns(4)
        .Push(0)
        .AddRows(2)
        .Push(1).AddColumns(5).Pop()
        .Pop()
        .Push(3)
        .AddColumns(3)
        .Build();

    // std::array<int, 2> weights0 = {3, 1};
    // GridLayout gridB = GridLayoutBuilder()
    //     .AddColumnsEx(2, std::span(weights0))
    //     .Push(0)
    //     .AddRows(2)
    //     .Push(0).MakeRectGrid(4, 6).Pop()
    //     .Push(1).MakeRectGrid(2, 12).Pop()
    //     .Pop()
    //     .Push(1)
    //     .AddColumns(3)
    //     .Build();

    // GridLayout gridC = GridLayoutBuilder().MakeRectGrid(8, 8).Build();

    // auto builder = GridLayoutBuilder().AddColumns(4);
    // for (int i = 0; i < 4; ++i) {
    //     builder.Push(i);
    //     builder.MakeRectGrid(4, 4);
    //     builder.Pop();
    // }
    // GridLayout gridD = builder.Build();

    GridLayout gridBase = GridLayoutBuilder()
        .MakeRectGrid(2, 4).Build(); 
    GridLayout gridSquare = gridBase.Scale({4 * GRID_STEP, 2 * GRID_STEP});

    std::shared_ptr<Node> node1 = std::make_shared<OscNode>();
    std::shared_ptr<Node> node2 = std::make_shared<OscNode>();

    node1->id = 1;
    node2->id = 2;

    std::vector<std::pair<uint32_t, uint32_t>> links;

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                running = false;

            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                auto sym = event.key.keysym.sym;
            }

            if (event.type == SDL_KEYUP) {
                auto sym = event.key.keysym.sym;
            }
        }

        if (!running) {
            break;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ImGui::ShowDemoWindow(&show_demo_window);
        ImGui::Begin("GridUI");
        ImGui::BeginGroup();
        float sidebarWidth = 250.0f;
        ImVec2 winSize = ImGui::GetContentRegionAvail();
        ImGui::BeginChild("Grid", ImVec2{winSize.x - sidebarWidth,0}, true);
            {
            ui.CanvasBegin("canvas0");

            // ui.BeginNode({0, 0}, {3, 2});
            // ui.DrawGridLayout("gridA", gridA, IM_COL32(255, 30, 0, 255), IM_COL32(0, 30, 255, 255));
            // ui.EndNode();

            // ui.BeginNode({0, 3}, {3, 3});
            // ui.DrawGridLayout("gridB", gridB, IM_COL32(255, 150, 0, 255), IM_COL32(200, 0, 255, 255));
            // ui.EndNode();

            // ui.BeginNode({4, 0}, {4, 4});
            // ui.DrawGridLayout("gridC", gridC, IM_COL32(30, 200, 30, 255), IM_COL32(255, 100, 50, 255));
            // ui.EndNode();

            // ui.BeginNode({4, 5}, {4, 1});
            // ui.DrawGridLayout("gridD", gridD, IM_COL32(200, 200, 200, 255), IM_COL32(10, 10, 15, 255));
            // ui.EndNode();

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->ChannelsSplit(2);
            draw_list->ChannelsSetCurrent(1);

            ui.BeginNode("nodeA", node1->id, &node1->position, node1->size);
            ui.DrawGridLayout("gridSquare", node1->GetLayout(), IM_COL32(200, 200, 200, 255), IM_COL32(10, 10, 15, 255));
            node1->Draw(&ui);
            ui.EndNode();

            draw_list->ChannelsSetCurrent(0);
            ui.BeginNode("nodeB", node2->id, &node2->position, node2->size);
            ui.DrawGridLayout("gridSquare", node2->GetLayout(), IM_COL32(200, 50, 30, 255), IM_COL32(10, 150, 15, 255));
            node2->Draw(&ui);
            ui.EndNode();

            draw_list->ChannelsMerge();

            for (auto& link : links) {
                ui.AddLink(link.first, link.second);
            }

            uint32_t pinSrc, pinDst;
            if (ui.QueryLink(&pinSrc, &pinDst)) {
                links.push_back(std::make_pair(pinSrc, pinDst));
            }

            ui.CanvasEnd();
            }

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
        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    return 0;
}