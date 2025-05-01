project(Game-D3XP)

file(GLOB src_d3xp_base *.cpp *.h game.def)
set_source_files_properties(${CMAKE_SOURCE_DIR}/d3xp/precompiled.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
list(REMOVE_ITEM src_d3xp_base ${CMAKE_SOURCE_DIR}/d3xp/EndLevel.cpp ${CMAKE_SOURCE_DIR}/d3xp/EndLevel.h)
source_group("" FILES ${src_d3xp_base})

file(GLOB src_d3xp_ai ai/*.cpp ai/*.h)
source_group("AI" FILES ${src_d3xp_ai})

file(GLOB src_d3xp_anim anim/*.cpp anim/*.h)
source_group("Anim" FILES ${src_d3xp_anim})

file(GLOB src_d3xp_gamesys gamesys/*.cpp gamesys/*.h)
list(REMOVE_ITEM src_d3xp_gamesys ${CMAKE_SOURCE_DIR}/d3xp/gamesys/Callbacks.cpp)
source_group("GameSys" FILES ${src_d3xp_gamesys})

file(GLOB src_d3xp_physics physics/*.cpp physics/*.h)
source_group("Physics" FILES ${src_d3xp_physics})

file(GLOB src_d3xp_script script/*.cpp script/*.h)
source_group("Script" FILES ${src_d3xp_script})

file(GLOB src_d3xp_menus menus/*.cpp menus/*.h)
source_group("Menus" FILES ${src_d3xp_menus})

set(src_d3xp
	${src_d3xp_base}
	${src_d3xp_ai}
	${src_d3xp_anim}
	${src_d3xp_gamesys}
	${src_d3xp_physics}
	${src_d3xp_script}
	${src_d3xp_menus})

add_library(${PROJECT_NAME} STATIC ${src_d3xp})
target_compile_definitions(${PROJECT_NAME} PRIVATE __IDLIB__)
if (USE_PRECOMPILED_HEADERS)
target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/idlib/precompiled.h)
endif()