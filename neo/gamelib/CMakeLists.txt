project(gameLib)

file(GLOB src_aas2 aas2/*.cpp aas2/*.h)
source_group("AAS2" FILES ${src_aas2})

set(src_gamelib
	${src_aas2})

add_library(${PROJECT_NAME} STATIC ${src_gamelib})
target_compile_definitions(${PROJECT_NAME} PRIVATE __GAMELIB__)
if (USE_PRECOMPILED_HEADERS)
target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/idlib/precompiled.h)
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "libs")