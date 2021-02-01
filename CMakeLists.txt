cmake_minimum_required(VERSION 3.5)
set (CMAKE_CXX_STANDARD 17)

project(Synth)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-O2 -Wall")

set(SOURCES
    "src/main.cpp"
    "src/output.cpp"
    "src/gui.cpp"
    "src/bridge.cpp"
    "src/node_graph.cpp"
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_impl_sdl.cpp
    external/imgui/imgui_impl_opengl3.cpp
    external/imgui-node-editor/imgui_node_editor.cpp
    external/imgui-node-editor/imgui_node_editor_api.cpp
    external/imgui-node-editor/imgui_canvas.cpp
    external/imgui-node-editor/crude_json.cpp
)

set(HEADERS
    "src/util.h"
    "src/output.h"
    "src/ring_buffer.h"
    "src/gui.h"
    "src/note.h"
    "src/keyboard.h"
    external/imgui/imgui.h
)

add_executable(
    ${PROJECT_NAME}
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    external/imgui
    external/imgui-node-editor
    external/include
    src/
)


target_link_libraries(${PROJECT_NAME} pthread GL GLEW SDL2 pulse)

