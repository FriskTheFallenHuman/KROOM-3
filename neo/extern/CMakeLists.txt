project(external)

set(src_external)
set(src_external_includes)

file(GLOB src_external_miniz miniz/*.c miniz/*.h)
source_group("Miniz" FILES ${src_external_miniz})
list(APPEND src_external ${src_external_miniz})
list(APPEND src_external_includes ${CMAKE_CURRENT_SOURCE_DIR}/miniz)
#always_optimize_sourcefile(miniz/miniz.c)

file(GLOB src_external_minizip minizip/*.cpp minizip/*.c minizip/*.h)
source_group("Minizip" FILES ${src_external_minizip})
list(APPEND src_external ${src_external_minizip})
list(APPEND src_external_includes ${CMAKE_CURRENT_SOURCE_DIR}/minizip)
#always_optimize_sourcefile(minizip/ioapi.c)
#always_optimize_sourcefile(minizip/unzip.cpp)

file(GLOB src_external_mikktspace mikktspace/*.hh)
source_group("MikktSpace" FILES ${src_external_mikktspace})
list(APPEND src_external ${src_external_mikktspace})

file(GLOB src_external_mesa mesa/*.h)
source_group("Mesa" FILES ${src_external_mesa})
list(APPEND src_external ${src_external_mesa})

file(GLOB src_external_imgui imgui/*.cpp imgui/misc/cpp/*.cpp imgui/misc/cpp/*.h imgui/*.h)
source_group("ImGUI\\Core" FILES ${src_external_imgui})
list(APPEND src_external ${src_external_imgui})

if(UNIX)
file(GLOB src_external_imgui_sdl
	imgui/backends/imgui_impl_sdl2.cpp
	imgui/backends/imgui_impl_sdl2.h
	imgui/backends/imgui_impl_sdlrenderer2.cpp
	imgui/backends/imgui_impl_sdlrenderer2.h)
source_group("ImGUI\\Backends" FILES ${src_external_imgui_sdl})
list(APPEND src_external ${src_external_imgui_sdl})
endif()

file(GLOB src_external_imguizmo imguizmo/*.cpp imguizmo/*.h)
source_group("ImGUI\\ImGUIZmo" FILES ${src_external_imguizmo})
list(APPEND src_external ${src_external_imguizmo})

file(GLOB src_external_imguicolortextedit ImGuiColorTextEdit/*.cpp ImGuiColorTextEdit/*.h)
source_group("ImGUI\\ImGuiColorTextEdit" FILES ${src_external_imguicolortextedit})
list(APPEND src_external ${src_external_imguicolortextedit})

file(GLOB src_external_stb stb/*.h)
source_group("Stb" FILES ${src_external_stb})
list(APPEND src_external ${src_external_stb})

file(GLOB src_external_tinyexr tinyexr/*.h)
source_group("TinyEXR" FILES ${src_external_tinyexr})
list(APPEND src_external ${src_external_tinyexr})

add_library(${PROJECT_NAME} STATIC ${src_external})
target_include_directories(${PROJECT_NAME} PRIVATE ${src_external_includes})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "libs")
