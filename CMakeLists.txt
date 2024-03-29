cmake_minimum_required(VERSION 3.5)
set (CMAKE_CXX_STANDARD 20)

project(Synth)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-O2 -Wall")

set(SOURCES
    "src/main.cpp"
    "src/output.cpp"
    "src/multigraph.cpp"
    "src/node_factory.cpp"
    "src/gui.cpp"
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_impl_sdl.cpp
    external/imgui/imgui_impl_opengl3.cpp
    external/imgui/imgui_demo.cpp
    external/imgui-node-editor/imgui_node_editor.cpp
    external/imgui-node-editor/imgui_node_editor_api.cpp
    external/imgui-node-editor/imgui_canvas.cpp
    external/imgui-node-editor/crude_json.cpp
)

add_definitions(-DREENTRANT)      
add_executable(
    ${PROJECT_NAME}
    ${SOURCES}
)

target_compile_definitions(${PROJECT_NAME} PUBLIC _REENTRANT)
target_include_directories(
    ${PROJECT_NAME} PUBLIC
    external/imgui
    external/imgui-node-editor
    external/include
    src/
)


target_link_libraries(${PROJECT_NAME} pthread GL SDL2 pulse rtaudio)

