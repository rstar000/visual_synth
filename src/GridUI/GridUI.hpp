#pragma once

#include <stdio.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <span>
#include <stack>
#include <thread>
#include <variant>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"

#include "GridUI/Colors.hpp"
#include "GridUI/GridLayout.hpp"
#include "GridUI/Widgets/Pin.hpp"

constexpr float GRID_STEP = 100.0f;
constexpr float TITLE_BAR_OFFSET = 20.0f;
constexpr float ROUNDING = 5.0f;

ImRect TransformComponent(ImRect const& componentRect, ImRect const& contentRect);
ImVec2 TransformPoint(ImVec2 const& pt, ImRect const& contentRect);

class GridUI 
{
   public:
    struct DraggedNodeInfo {
        uint32_t id;
        ImVec2 dragOrigin;
        ImVec2 startPosition;
        ImVec2 endPosition;
    };

    struct PinDescription {
        uint32_t id;
        ImRect bbox;
        PinKind type;
    };

    struct LinkQuery {
        uint32_t pinSrc;
        uint32_t pinDst;
    };

   private:
    ImVec2 m_scroll;
    ImRect m_bounds;
    ImVec2 m_textSize;

    // Nodes

    // Rect of current node
    ImRect m_currentRect;
    std::string m_nodeTitle;
    ImRect m_titleBarRect;
    ImRect m_contentRect;
    ImVec2* m_nodeGridPos;
    ImVec2 m_nodeGridSize;
    uint32_t m_nodeId;

    std::optional<DraggedNodeInfo> m_draggedNode;
    std::optional<uint32_t> m_selectedNode;
    std::optional<uint32_t> m_deletedNode;

    // Pins
    // All added pins by pin id
    std::map<uint32_t, PinDescription> m_pins;
    // Pins of current node
    std::vector<PinDescription> m_currentNodePins;
    std::optional<uint32_t> m_selectedPin;

    // Links
    std::optional<LinkQuery> m_linkQuery;

    ColorScheme m_colors;

   public:
    bool enableGrid;
    bool enableContextMenu;
    bool enableDebugLayout;

    GridUI()
        : m_scroll{ImVec2{0.0f, 0.0f}},
          m_bounds{},
          enableGrid{true},
          enableContextMenu{true},
          enableDebugLayout{false},
          m_colors{ColorScheme::GenerateDefault()} {}

    void CanvasEnd() {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->PopClipRect();

        auto& io = ImGui::GetIO();
        if (!io.MouseDown[0]) {
            m_draggedNode = std::nullopt;
        }
        ImGui::PopID();
    }

    void CanvasBegin(const char* label, auto contextMenuCallback) {
        float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        m_textSize = {TEXT_BASE_WIDTH, TEXT_BASE_HEIGHT};

        m_bounds.Min = ImGui::GetCursorScreenPos();  // ImDrawList API uses
                                                     // screen coordinates!
        ImVec2 canvas_sz = ImGui::GetContentRegionAvail();  // Resize canvas to
                                                            // what's available
        m_bounds.Max = m_bounds.Min + canvas_sz;

        // Draw border and background color
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->AddRectFilled(m_bounds.Min, m_bounds.Max,
                                 m_colors.gridColors.background);
        draw_list->AddRect(m_bounds.Min, m_bounds.Max,
                           m_colors.gridColors.lineAxis);

        // This will catch our interactions
        ImGui::PushID(label);
        if (!(canvas_sz.x == 0.0f || canvas_sz.y == 0.0f)) {
            ImGui::SetNextItemAllowOverlap();
            ImGui::InvisibleButton("", canvas_sz,
                                   ImGuiButtonFlags_MouseButtonLeft |
                                       ImGuiButtonFlags_MouseButtonRight);
        }
        const bool is_hovered = ImGui::IsItemHovered();  // Hovered
        const bool is_active = ImGui::IsItemActive();    // Held

        const ImVec2 origin = m_bounds.Min + m_scroll;
        const ImVec2 mouse_pos_in_canvas = io.MousePos - origin;

        // Pan (we use a zero mouse threshold when there's no context menu)
        // You may decide to make that threshold dynamic based on whether the
        // mouse is hovering something etc.
        const float mouse_threshold_for_pan = enableContextMenu ? -1.0f : 0.0f;
        if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right,
                                                mouse_threshold_for_pan)) {
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
        if (enableContextMenu && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
            ImGui::OpenPopupOnItemClick("context",
                                        ImGuiPopupFlags_MouseButtonRight);
        if (ImGui::BeginPopup("context")) {
            contextMenuCallback();
            ImGui::EndPopup();
        }

        // Draw grid + all lines in the canvas
        draw_list->PushClipRect(m_bounds.Min, m_bounds.Max, true);

        if (enableGrid) {
            for (float x = fmodf(m_scroll.x, GRID_STEP); x < canvas_sz.x;
                 x += GRID_STEP) {
                float world_x = x - m_scroll.x;
                ImU32 color = m_colors.gridColors.line;
                if (std::fabs(world_x) < 0.1f) {
                    color = m_colors.gridColors.lineAxis;
                }
                draw_list->AddLine(ImVec2(m_bounds.Min.x + x, m_bounds.Min.y),
                                   ImVec2(m_bounds.Min.x + x, m_bounds.Max.y),
                                   color);
            }
            for (float y = fmodf(m_scroll.y, GRID_STEP); y < canvas_sz.y;
                 y += GRID_STEP) {
                float world_y = y - m_scroll.y;
                ImU32 color = m_colors.gridColors.line;
                if (std::fabs(world_y) < 0.1f) {
                    color = m_colors.gridColors.lineAxis;
                }
                draw_list->AddLine(ImVec2(m_bounds.Min.x, m_bounds.Min.y + y),
                                   ImVec2(m_bounds.Max.x, m_bounds.Min.y + y),
                                   color);
            }
        }

        static bool disable_mouse_wheel = false;
        static bool disable_menu = false;

        m_pins.clear();
        m_linkQuery = std::nullopt;
        m_deletedNode = std::nullopt;
    }

    void BeginNode(const char* label, uint32_t nodeId, ImVec2* position,
                   ImVec2 size) {
        ImGui::PushID("Node");
        ImGui::PushID(nodeId);
        const ImVec2 origin = m_bounds.Min + m_scroll;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        m_currentNodePins.clear();

        m_nodeGridPos = position;
        m_nodeGridSize = size;
        m_nodeId = nodeId;
        m_currentRect = ImRect{origin + *position * GRID_STEP,
                               origin + (*position + size) * GRID_STEP};
        m_nodeTitle = std::string(label);

        // Draw title bar above node content
        m_titleBarRect =
            ImRect{ImVec2{m_currentRect.Min.x - 1.0f, m_currentRect.Min.y},
                   ImVec2{m_currentRect.Max.x + 1.0f, m_currentRect.Min.y + TITLE_BAR_OFFSET}};

        m_contentRect = ImRect{m_titleBarRect.GetBL(), m_currentRect.Max};

        // Full window fill
        draw_list->AddRectFilled(m_titleBarRect.Min, m_currentRect.Max,
                                 m_colors.nodeColors.background, ROUNDING,
                                 ImDrawFlags_RoundCornersAll);
    }

    ImVec2 ScreenPosToGridPos(ImVec2 screenPos) {
        // Screen position grid zero
        const ImVec2 origin = m_bounds.Min + m_scroll;
        return screenPos - origin;
    }

    void DrawGridLayout(const char* label, const GridLayout& layout,
                        ImU32 col1u, ImU32 col2u) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImColor col1 = ImColor(col1u);
        ImColor col2 = ImColor(col2u);
        const auto& rect = m_currentRect;
        ImVec2 scale = rect.GetSize();

        ImVec2 cursorBegin = ImGui::GetCursorScreenPos();
        for (int subIdx = 0; subIdx < layout.NumComponnets(); ++subIdx) {
            const GridComponent& sub = layout.GetComponent(subIdx);
            ImRect subRect = sub.Rect();
            ImVec4 newColor =
                lerp(ImVec4(col1), ImVec4(col2),
                     static_cast<float>(subIdx) / layout.NumComponnets());
            subRect.Translate(rect.Min);
            draw_list->AddRectFilled(subRect.Min, subRect.Max,
                                     ImU32(ImColor(newColor)));
            draw_list->AddText(subRect.GetCenter() - m_textSize / 2.0f,
                               IM_COL32(255, 255, 200, 255),
                               std::to_string(subIdx).c_str());

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
            //     draw_list->AddRect(subRect.Min, subRect.Max, IM_COL32(255,
            //     255, 0, 255), 0.0f, 0, 2.0f);
            // }
            // ImGui::PopID();
        }

        ImGui::SetCursorScreenPos(cursorBegin);
    }

    void DrawComponent(const GridComponent& component, auto drawFunc) {
        drawFunc(TransformComponent(component.Rect(), m_contentRect));
    }

    ImRect GetNodeRect() const { return m_currentRect; }

    void EndNode() {
        // Draw node frame
        auto& io = ImGui::GetIO();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        bool is_selected = m_selectedNode && *m_selectedNode == m_nodeId;

        // Draw frame outline
        ImGui::SetCursorScreenPos(m_currentRect.Min);
        ImGui::SetNextItemAllowOverlap();
        ImGui::InvisibleButton("btn_frame", m_currentRect.GetSize());

        // Handle title bar interaction
        ImGui::SetCursorScreenPos(m_titleBarRect.Min);
        ImGui::SetNextItemAllowOverlap();
        ImGui::InvisibleButton("btn_titlebar", m_titleBarRect.GetSize());
        bool is_active = ImGui::IsItemActive();
        bool is_hovered = ImGui::IsItemHovered();
        if (is_active) {
            // ImGui::GetForegroundDrawList()->AddLine(io.MouseClickedPos[0],
            // io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f); // Draw
            // a line between the button and the mouse cursor
            if (!m_draggedNode) {
                m_draggedNode =
                    DraggedNodeInfo{.id = m_nodeId,
                                    .dragOrigin = io.MouseClickedPos[0],
                                    .startPosition = *m_nodeGridPos};
            }
        }
        int titleFlags = ImDrawFlags_RoundCornersTop;
        int frameFlags = ImDrawFlags_RoundCornersAll;

        if (is_selected) {
            draw_list->AddRect(m_titleBarRect.Min, m_currentRect.Max,
                               m_colors.nodeColors.border.selected, ROUNDING,
                               frameFlags, 3.0f);
            draw_list->AddRectFilled(m_titleBarRect.Min, m_titleBarRect.Max,
                                     m_colors.nodeColors.titleBar.selected,
                                     ROUNDING, titleFlags);
        } else if (is_hovered) {
            // draw_list->AddRect(m_titleBarRect.Min, m_currentRect.Max,
            // m_colors.nodeColors.border.normal);
            draw_list->AddRectFilled(m_titleBarRect.Min, m_titleBarRect.Max,
                                     m_colors.nodeColors.titleBar.hovered,
                                     ROUNDING, titleFlags);
        } else {
            // draw_list->AddRect(m_titleBarRect.Min, m_currentRect.Max,
            // m_colors.nodeColors.border.normal);
            draw_list->AddRectFilled(m_titleBarRect.Min, m_titleBarRect.Max,
                                     m_colors.nodeColors.titleBar.normal,
                                     ROUNDING, titleFlags);
        }

        // Draw window title
        float max_text_width = m_titleBarRect.GetWidth();
        ImVec2 label_size = ImGui::CalcTextSize(m_nodeTitle.c_str(), NULL, true,
                                                max_text_width);
        float label_offset = (TITLE_BAR_OFFSET - label_size.y) * 0.5f;
        ImGui::SetCursorScreenPos(m_titleBarRect.Min +
                                  ImVec2{ROUNDING, label_offset});
        ImGui::PushItemWidth(max_text_width);
        ImGui::Text(m_nodeTitle.c_str());
        ImGui::PopItemWidth();

        // Draw delete button
        ImGui::SetCursorScreenPos(ImVec2{
            m_titleBarRect.Max.x - TITLE_BAR_OFFSET, m_titleBarRect.Min.y});
        if (ImGui::ButtonEx("x##close",
                            ImVec2{TITLE_BAR_OFFSET, TITLE_BAR_OFFSET},
                            ImGuiButtonFlags_PressedOnDoubleClick)) {
            m_deletedNode = m_nodeId;
            SPDLOG_INFO("[GridUI] Delete node: {}", m_nodeId);
        }

        // Set current node as selected
        if (is_active && ImGui::IsMouseClicked(0)) {
            m_selectedNode = m_nodeId;
        }

        if (m_draggedNode && m_draggedNode->id == m_nodeId) {
            // Current node is dragged: recompute end pos
            ImVec2 curMousePos = io.MousePos;
            ImVec2 gridDelta = {
                static_cast<int>((curMousePos.x - m_draggedNode->dragOrigin.x) /
                                 (GRID_STEP * 0.5f)),
                static_cast<int>((curMousePos.y - m_draggedNode->dragOrigin.y) /
                                 (GRID_STEP * 0.5f))};
            gridDelta *= 0.5f;

            m_draggedNode->endPosition =
                m_draggedNode->startPosition + gridDelta;
            *m_nodeGridPos = m_draggedNode->endPosition;
        }

        // Draw node pins
        for (auto& pin : m_currentNodePins) {
            ImRect pinBbox = pin.bbox;

            ImGui::PushID(pin.id);
            ImGui::SetCursorScreenPos(pinBbox.Min);
            ImGui::SetNextItemAllowOverlap();
            ImGui::InvisibleButton("", pinBbox.GetSize());

            bool is_active = ImGui::IsItemActive();
            bool is_hovered = ImGui::IsItemHovered();
            bool is_selected = m_selectedPin && *m_selectedPin == pin.id;

            PinState state = PinState::kNormal;
            if (is_hovered) {
                state = PinState::kHovered;
            }
            if (is_selected) {
                state = PinState::kSelected;
            }

            DrawPin("", pinBbox, pin.type, state, m_colors.pinColors);
            ImGui::PopID();

            bool link_add_mode = io.KeyCtrl;
            if (is_active) {
                if (m_selectedPin && link_add_mode) {
                    m_linkQuery =
                        LinkQuery{.pinSrc = *m_selectedPin, .pinDst = pin.id};

                    SPDLOG_INFO("[GridUI] Add link query: {} {}", m_linkQuery->pinSrc, m_linkQuery->pinDst);
                    m_selectedPin = std::nullopt;
                } else {
                    m_selectedPin = pin.id;
                }
            }
        }

        ImGui::PopID();
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

    void AddPin(uint32_t pinId, PinKind pinType, ImRect bbox) {
        m_currentNodePins.push_back(
            PinDescription{
                .id = pinId, 
                .bbox = TransformComponent(bbox, m_contentRect), 
                .type = pinType});

        m_pins[pinId] = m_currentNodePins.back();
    }

    void AddLink(uint32_t pinSrc, uint32_t pinDst) {
        auto& pinA = m_pins.at(pinSrc);
        auto& pinB = m_pins.at(pinDst);

        bool is_selected = false;
        if (m_selectedPin &&
            (*m_selectedPin == pinSrc || *m_selectedPin == pinDst)) {
            is_selected = true;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddLine(pinA.bbox.GetCenter(),
                           pinB.bbox.GetCenter(),
                           IM_COL32(200, 200, 0, 160), 3.0f);
    }

    void HandleLinkQuery(auto linkFunc) {
        if (m_linkQuery) {
            linkFunc(*m_linkQuery);
        }
    }
};
