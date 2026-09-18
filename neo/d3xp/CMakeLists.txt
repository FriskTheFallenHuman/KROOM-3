project(game)

file(GLOB src_d3xp_base *.cpp *.h game.def)
list(REMOVE_ITEM src_d3xp_base ${CMAKE_CURRENT_SOURCE_DIR}/precompiled.cpp ${CMAKE_CURRENT_SOURCE_DIR}/EndLevel.cpp ${CMAKE_CURRENT_SOURCE_DIR}/EndLevel.h)
source_group("Game-D3XP" FILES ${src_d3xp_base})

file(GLOB src_d3xp_ai ai/*.cpp ai/*.h)
source_group("Game-D3XP\\AI" FILES ${src_d3xp_ai})

file(GLOB src_d3xp_anim anim/*.cpp anim/*.h)
source_group("Game-D3XP\\Anim" FILES ${src_d3xp_anim})

file(GLOB src_d3xp_decls decls/*.cpp decls/*.h)
source_group("Game-D3XP\\Decls" FILES ${src_d3xp_decls})

file(GLOB src_d3xp_gamesys gamesys/*.cpp gamesys/*.h)
list(REMOVE_ITEM src_d3xp_gamesys ${CMAKE_CURRENT_SOURCE_DIR}/gamesys/Callbacks.cpp)
source_group("Game-D3XP\\GameSys" FILES ${src_d3xp_gamesys})

file(GLOB src_d3xp_physics physics/*.cpp physics/*.h)
source_group("Game-D3XP\\Physics" FILES ${src_d3xp_physics})

file(GLOB src_d3xp_script script/*.cpp script/*.h)
source_group("Game-D3XP\\Script" FILES ${src_d3xp_script})

file(GLOB src_d3xp_swf swf/*.cpp swf/*.h)
source_group("Game-D3XP\\SWF" FILES ${src_d3xp_swf})

file(GLOB src_d3xp_menus menus/*.cpp menus/*.h)
source_group("Game-D3XP\\Menus" FILES ${src_d3xp_menus})

set(src_d3xp
	${src_d3xp_base}
	${src_d3xp_ai}
	${src_d3xp_anim}
	${src_d3xp_decls}
	${src_d3xp_gamesys}
	${src_d3xp_physics}
	${src_d3xp_script}
	${src_d3xp_swf}
	${src_d3xp_menus})

if(${BUILD_GAME_DLL})
	add_library(${PROJECT_NAME} SHARED ${src_d3xp})
	target_compile_definitions(${PROJECT_NAME} PRIVATE GAME_DLL)
	target_link_libraries(${PROJECT_NAME} idLib gameLib external)
else()
	add_library(${PROJECT_NAME} STATIC ${src_d3xp})
endif()

if (USE_PRECOMPILED_HEADERS)
target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/idlib/precompiled.h)
endif()

if(${BUILD_GAME_DLL})
	set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "game${cpu_arch}")
	set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "dlls")
else()
	set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "libs")
endif()

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD 
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
		  $<TARGET_FILE:${PROJECT_NAME}>
		  "$<TARGET_PROPERTY:${PROJECT_NAME},RUNTIME_OUTPUT_DIRECTORY>/base"
  VERBATIM)