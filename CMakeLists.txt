cmake_minimum_required(VERSION 3.5)
set (CMAKE_CXX_STANDARD 20)

project(Synth)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_CXX_FLAGS "-g -O3 -fsanitize=undefined -fno-omit-frame-pointer -Wall")
set(CMAKE_CXX_FLAGS "-g -O3  -Wall")

set(SOURCES
    "src/main.cpp"
    "src/midi.cpp"
    "src/output.cpp"
    "src/multigraph.cpp"
    "src/node_factory.cpp"
    "src/gui.cpp"
    "src/GridUI/GridUI.cpp"
    "src/GridUI/Colors.cpp"
    "src/Oscillators/Waveform.cpp"
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_impl_sdl2.cpp
    external/imgui/imgui_impl_opengl3.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/misc/freetype/imgui_freetype.cpp
    external/imgui-knobs/imgui_knobs.cpp
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
    external/imgui-knobs
    external/include
    src/
    /usr/include/freetype2
)


target_link_libraries(${PROJECT_NAME} pthread GL SDL2 pulse rtaudio rtmidi freetype)
