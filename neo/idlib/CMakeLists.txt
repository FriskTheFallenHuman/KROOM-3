project(idLib)

file(GLOB src_idlib_base *.cpp *.h)
source_group("" FILES ${src_idlib_base})

file(GLOB src_idlib_bv bv/*.cpp bv/*.h)
source_group("BV" FILES ${src_idlib_bv})

file(GLOB src_idlib_containers containers/*.cpp containers/*.h)
source_group("Containers" FILES ${src_idlib_containers})

file(GLOB src_idlib_geometry geometry/*.cpp geometry/*.h)
source_group("Geometry" FILES ${src_idlib_geometry})

file(GLOB src_idlib_hashing hashing/*.cpp hashing/*.h)
source_group("Hashing" FILES ${src_idlib_hashing})

file(GLOB src_idlib_math math/*.cpp math/*.h)
source_group("Math" FILES ${src_idlib_math})

file(GLOB src_idlib_sys sys/*.cpp sys/*.h)
source_group("Sys" FILES ${src_idlib_sys})

if(MSVC)
file(GLOB src_idlib_sys_platform sys/win32/*.cpp sys/win32/*.h)
else()
file(GLOB src_idlib_sys_platform sys/posix/*.cpp sys/posix/*.h)
endif()
source_group("Sys\\Threads" FILES ${src_idlib_sys_platform})

set(src_idlib
	${src_idlib_base}
	${src_idlib_bv}
	${src_idlib_containers}
	${src_idlib_geometry}
	${src_idlib_hashing}
	${src_idlib_math}
	${src_idlib_sys}
	${src_idlib_sys_platform})

# These files are funny
set_source_files_properties(
	${CMAKE_SOURCE_DIR}/idlib/geometry/RenderMatrix.cpp
	${CMAKE_SOURCE_DIR}/idlib/SoftwareCache.cpp
	${CMAKE_SOURCE_DIR}/idlib/precompiled.cpp
	PROPERTIES
	SKIP_PRECOMPILE_HEADERS ON)

add_library(${PROJECT_NAME} STATIC ${src_idlib})
target_compile_definitions(${PROJECT_NAME} PRIVATE __IDLIB__)
if (USE_PRECOMPILED_HEADERS)
target_precompile_headers(${PROJECT_NAME} PRIVATE precompiled.h)
endif()