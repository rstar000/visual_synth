cmake_minimum_required(VERSION 3.5)
set (CMAKE_CXX_STANDARD 20)

project(Synth)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_CXX_FLAGS "-g -O3 -fsanitize=undefined -fno-omit-frame-pointer -Wall")
set(CMAKE_CXX_FLAGS "-O3  -Wall")

set(HEADERS
    "src/gui.h"
    "src/midi.h"
    "src/multigraph.h"
    "src/node.h"
    "src/node_factory.h"
    "src/note.h"
    "src/output.h"
    "src/synth.h"
    "src/voice.h"
    "src/writer.h"
    "src/gui.h"
    "src/sync.h"
    "src/Param.hpp"
    "src/file_menu.h"
    "src/Filters/BiQuad.hpp"
    "src/GUI/PatchBrowser.hpp"
    "src/GridUI/Widgets/Checkbox.hpp"
    "src/GridUI/Widgets/Controls.hpp"
    "src/GridUI/Widgets/Fader.hpp"
    "src/GridUI/Widgets/Knob.hpp"
    "src/GridUI/Widgets/Pin.hpp"
    "src/GridUI/Widgets/Widget.hpp"
    "src/GridUI/Widgets/WidgetUtils.hpp"
    "src/GridUI/Colors.hpp"
    "src/GridUI/GridLayout.hpp"
    "src/GridUI/Colors.hpp"
    "src/GridUI/GridUI.hpp"
    "src/Oscillators/Common.hpp"
    "src/Oscillators/Waveform.hpp"
    "src/nodes/common.h"
    "src/nodes/env.h"
    "src/nodes/osc.h"
    "src/nodes/seq.h"
    "src/nodes/sink.h"
    "src/nodes/Sequencer.hpp"
    "src/nodes/Filter.hpp"
    "external/imgui/imgui.h"
)

set(SOURCES
    "src/main.cpp"
    "src/midi.cpp"
    "src/output.cpp"
    "src/multigraph.cpp"
    "src/node_factory.cpp"
    "src/gui.cpp"
    "src/Filters/BiQuad.cpp"
    "src/GridUI/GridUI.cpp"
    "src/GridUI/Colors.cpp"
    "src/GridUI/Widgets/Checkbox.cpp"
    "src/GridUI/Widgets/WidgetUtils.cpp"
    "src/Oscillators/Waveform.cpp"
    "src/GUI/PatchBrowser.cpp"
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
    ${HEADERS}
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
