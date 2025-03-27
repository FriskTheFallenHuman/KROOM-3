cmake_minimum_required(VERSION 2.6...3.22 FATAL_ERROR)

if (EXISTS "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")
	set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
else()
	message(FATAL_ERROR "Vcpkg wasn't not found, aborting, make sure to git clone this with all submodules!")
endif()

set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER  "")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

project(Kroom3BFG
		VERSION "1.2.0"
		DESCRIPTION "Fork of RBDOOM3-BFG v1.3.0 source port aims to modernize DOOM 3 BFG"
		HOMEPAGE_URL "https://github.com/FriskTheFallenHuman/KROOM-3"
		LANGUAGES C CXX)

option(FORCE_COLOR_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." OFF)
option(COMPILE_COMMANDS "Generate compile_commands.json" OFF)
option(TOOLS "Compile the built-in MFC based tools" OFF)
option(USE_PRECOMPILED_HEADERS "Use precompiled headers during build" ON)
option(SDL2 "Use SDL2 instead of platform specific implementations" OFF)
option(OPENAL "Use OpenAL soft instead of XAudio2" OFF)
option(FFMPEG "Use FMPEG to render Bink videos" OFF)
option(BINKDEC "Use included libbinkdec to render Bink videos" ON)
option(USE_VULKAN "Use Vulkan instead of OpenGL" OFF)
option(SPIRV_SHADERC "Compile SPIR-V shader byte code using shaderc instead of using Glslang directly" OFF)
option(USE_MoltenVK	"Use MoltenVK library directly instead of Vulkan loader on macOS" OFF)
option(ONATIVE "Optimize for the host CPU" OFF)
option(REPRODUCIBLE_BUILD "Replace __DATE__ and __TIME__ by hardcoded values for reproducible builds." OFF)
option(WINRT "Build for Windows RT" OFF)
option(WINDOWS10 "Build for Windows 10" ON)

include(CPUDefine)

if(FFMPEG AND BINKDEC)
	message(FATAL_ERROR "Only one of FFMPEG and BINKDEC (or neither) can be enabled at a time")
endif()

if(UNIX)
	set(OPENAL ON)
	set(FFMPEG ON)

	# SRS - Disable precompiled headers when cross-compiling on OSX, e.g. when creating universal binaries
	#     - Also disable precompiled headers when using the Ninja Multi-Config generator on linux or macOS
	if((CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64" AND CMAKE_OSX_ARCHITECTURES MATCHES "arm64" ) OR
	   (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64"  AND CMAKE_OSX_ARCHITECTURES MATCHES "x86_64") OR
	   (CMAKE_GENERATOR MATCHES "Ninja Multi-Config"))
		set(USE_PRECOMPILED_HEADERS OFF)
	endif()

	# SRS - For single-config dev builds, define NO_MULTI_CONFIG to search for basepath up 1 dir level vs 2
	if((CMAKE_GENERATOR MATCHES "Unix Makefiles") OR
	   (CMAKE_GENERATOR MATCHES "Ninja" AND NOT CMAKE_GENERATOR MATCHES "Multi-Config"))
		add_definitions(-DNO_MULTI_CONFIG)
	endif()
endif()

if(REPRODUCIBLE_BUILD)
	# don't use __DATE__ and __TIME__ macros so builds are reproducible
	add_definitions(-DID_REPRODUCIBLE_BUILD)
endif()

if(COMPILE_COMMANDS)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if (FORCE_COLOR_OUTPUT)
	if (CMAKE_COMPILER_IS_GNUCC)
		add_compile_options(-fdiagnostics-color=always)
	elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
		add_compile_options(-fcolor-diagnostics)
	endif ()
endif ()

configure_file("${CMAKE_SOURCE_DIR}/config.h.in" "${CMAKE_BINARY_DIR}/config.h")

if(MSVC)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/../bin/x86_64)
	else()
		set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/../bin/x86)
	endif()

	message(STATUS "Root: ${CMAKE_ROOT}")
	message(STATUS "Output Path: ${EXECUTABLE_OUTPUT_PATH}")
	message(STATUS "Project Binary Path: ${PROJECT_BINARY_DIR}")
	message(STATUS "Binary Path: ${CMAKE_BINARY_DIR}")
else()
	message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")

	add_compile_options(-pipe)
	#add_compile_options(-Wall)
	add_compile_options(-Werror=format-security)
	add_compile_options(-Werror=format)
	add_compile_options(-Wno-format-zero-length)
	add_compile_options(-Wno-nonnull)

	# Compiler check (needs -std=c++14 flag)
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
	if(COMPILER_SUPPORTS_CXX14)
		set(CMAKE_CXX_STANDARD 14)
		set(CMAKE_CXX_STANDARD_REQUIRED ON)
		set(CMAKE_CXX_EXTENSIONS OFF)
	else()
		message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
	endif()

	if (CPU_OPTIMIZATION)
		SEPARATE_ARGUMENTS(CPU_OPTIMIZATION)
		add_compile_options(${CPU_OPTIMIZATION})
	endif()

	if(WIN32)
		# require msvcr70.dll or newer for _aligned_malloc etc
		# I think it is from Visual C++ .NET 2002, so it should be available on any remotely modern system.
		add_definitions(-D__MSVCRT_VERSION__=0x0700)

		include_directories(extern/mingw-hacks)
	endif()

	add_compile_options(-Wno-pragmas -Wno-unused-variable -Wno-switch -Wno-unused-value -Winvalid-pch -Wno-multichar)
	if(CMAKE_C_COMPILER_ID MATCHES "Clang")
		# add clang-specific settings for warnings (the second one make sure clang doesn't complain
		# about unknown -W flags, like -Wno-unused-but-set-variable)
		# SRS - Add -Wno-expansion-to-defined, Wno-nullability-completeness and -Wno-shorten-64-to-32 to list of warning settings
		add_compile_options(-Wno-local-type-template-args -Wno-unknown-warning-option -Wno-inline-new-delete -Wno-switch-enum -Wno-expansion-to-defined -Wno-nullability-completeness -Wno-shorten-64-to-32)
	endif()

	if(NOT CMAKE_CROSSCOMPILING AND ONATIVE)
		if(CMAKE_SYSTEM_PROCESSOR MATCHES "((powerpc|ppc)64le)|(mips64)")
			add_compile_options(-mcpu=native)
		else()
			add_compile_options(-march=native)
		endif()
	endif()

	add_compile_options(-fno-strict-aliasing)

	# SRS - Make sure OSX can find system headers and add support for minimum OSX runtime version
	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		# SRS - Also add -fasm-blocks otherwise Xcode complains and -Qunused-arguments to silence MMX/SSE unused arg warnings when compiling for Apple arm64
		add_compile_options(-fasm-blocks -Qunused-arguments)
		if(CMAKE_OSX_SYSROOT)
			add_compile_options(-isysroot ${CMAKE_OSX_SYSROOT})
			message(STATUS "Using macOS sysroot: " ${CMAKE_OSX_SYSROOT})
		endif()
		if(CMAKE_OSX_DEPLOYMENT_TARGET)
			add_compile_options(-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
		endif()
	endif()

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -D_DEBUG -O0")
	#set(CMAKE_C_FLAGS_DEBUGALL "${CMAKE_C_FLAGS_DEBUGALL} -g -ggdb -D_DEBUG")
	#set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE} -g -ggdb -D_DEBUG -O1 -fno-omit-frame-pointer")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffp-contract=off -fomit-frame-pointer -fno-math-errno -fno-trapping-math")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g -O3 -ffp-contract=off -fomit-frame-pointer -fno-math-errno -fno-trapping-math")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -ffp-contract=off -fomit-frame-pointer -fno-math-errno -fno-trapping-math")

	set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUGALL ${CMAKE_C_FLAGS_DEBUGALL})
	#set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})
	set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
	set(CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL})

	if(NOT WIN32)
		set(sys_libs pthread ${CMAKE_DL_LIBS})

		if(NOT "${CMAKE_SYSTEM}" MATCHES "Darwin")
			list(APPEND sys_libs rt)
		endif()
	endif()

elseif(MSVC)

	set(CMAKE_CXX_STANDARD 14)
	set(CMAKE_CXX_STANDARD_REQUIRED True)

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D _DEBUG /MP /MDd")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP /Oi /Oy /MD")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MP /Oi /MDd")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MP /Oi /Oy /MD")

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP /MDd")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /Oi /Oy /MD")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /MDd")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP /Oi /Oy /MD")

	# RB: without /SAFESEH:NO we can't link against ffmpeg libs and VS2013 or we have to build our own
	# libs for newer compilers
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /SAFESEH:NO")

	add_definitions(-DWIN32
					-D_WINDOWS
					-D_CRT_SECURE_NO_DEPRECATE
					-D_CRT_NONSTDC_NO_DEPRECATE
					-D_CRT_SECURE_NO_WARNINGS
					-D_MBCS)

	if( WINRT OR WINDOWS10 ) # Windows RT
		add_definitions(-DUSE_WINRT)
	endif()

	if(NOT CMAKE_CL_64)
		add_definitions(-D_USE_32BIT_TIME_T)
	endif()

	set(sys_libs dbghelp iphlpapi winmm wsock32 ${CMAKE_DL_LIBS})

	if(MSVC)
		if(WINDOWS10)
			if(OPENAL)
				list(APPEND sys_libs dxguid)
			else()
				list(APPEND sys_libs dsound dxguid xaudio2)
			endif()

			if (NOT SDL2 )
				list(APPEND sys_libs dinput8 Xinput)
			endif()
		else()
			find_package(DirectX REQUIRED)
			include_directories(${DirectX_INCLUDE_DIR})
		endif()
	endif()

	if(TOOLS)
		add_definitions(-D_AFXDLL)

		# 0: Use Standard Windows Libraries
		# 1: Use MFC in a Static Library
		# 2: Use MFC in a Shared DLL
		set(CMAKE_MFC_FLAG 2)
	endif()

endif()

add_definitions(-DID_PC)

if (USE_INTRINSICS_SSE)
	add_definitions(-DUSE_INTRINSICS_SSE)
endif()

macro(SET_OPTION option value)
  set(${option} ${value} CACHE STRING "" FORCE)
endmacro()

if(USE_VULKAN)
	# RB: moved this above the general Vulkan part so glslang does not include Vulkan SDK headers
	# which causes all kinds of weird segmentation faults because struct sizes don't match

	# SRS - Set default VULKAN_SDK location if environment variable not defined on OSX
	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT DEFINED ENV{VULKAN_SDK})
		if(NOT USE_MoltenVK)
			# SRS - Vulkan SDK installer copies standard vulkan headers and libs to /usr/local on OSX
			set(ENV{VULKAN_SDK} /usr/local)
		else()
			message(FATAL_ERROR "Must define VULKAN_SDK location if USE_MoltenVK option enabled!")
		endif()
	endif()

	if(SPIRV_SHADERC)
		add_definitions(-DSPIRV_SHADERC)

		# SRS - Add case for UNIX/OSX
		if(CMAKE_CL_64 OR UNIX)
			link_directories($ENV{VULKAN_SDK}/Lib)
		else()
			link_directories($ENV{VULKAN_SDK}/Lib32)
		endif()

	else()
		# override Glslang build options

		# options required by November 2018 release
		SET_OPTION(ENABLE_SPVREMAPPER ON)
		SET_OPTION(ENABLE_GLSLANG_BINARIES OFF)
		SET_OPTION(ENABLE_HLSL OFF)
		SET_OPTION(ENABLE_OPT ON)
		SET_OPTION(SPIRV_SKIP_EXECUTABLES ON)

		# options required by July 2020 release
		SET_OPTION(ENABLE_PCH OFF)
		SET_OPTION(ENABLE_CTEST OFF)
		#SET_OPTION(BUILD_EXTERNAL OFF)
		SET_OPTION(SKIP_GLSLANG_INSTALL ON)

		set(GLSLANG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/glslang)

		# requiered has a of 2024 version
		find_package(Python REQUIRED COMPONENTS Interpreter)
		execute_process(
			COMMAND ${Python_EXECUTABLE} ${GLSLANG_DIR}/update_glslang_sources.py
			WORKING_DIRECTORY ${GLSLANG_DIR})

		#add_definitions(-DGLSLANG_MINOR_VERSION=13)
		add_subdirectory(${GLSLANG_DIR})
		include_directories(${GLSLANG_DIR})
		#link_directories($ENV{VULKAN_SDK}/Lib)
	endif()

	# Use FindVulkan module added with CMAKE 3.7
	if(NOT CMAKE_VERSION VERSION_LESS 3.7.0)
		message( STATUS "Using module to find Vulkan" )
		find_package(Vulkan)
	endif()

	if(NOT Vulkan_FOUND)
		message(FATAL_ERROR "Could not find Vulkan library!")
	else()
		add_definitions(-DUSE_VULKAN)
		include_directories($ENV{VULKAN_SDK}/include)

		if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
			# SRS - Enable Beta extensions for VULKAN_SDK portability subset features on OSX
			add_definitions(-DVK_ENABLE_BETA_EXTENSIONS)
			# SRS - Optionally use MoltenVK headers/library for runtime config functions on OSX
			if(USE_MoltenVK)
				add_definitions(-DUSE_MoltenVK)
				include_directories($ENV{VULKAN_SDK}/../MoltenVK/include)
				set(Vulkan_LIBRARY $ENV{VULKAN_SDK}/../MoltenVK/dylib/macOS/libMoltenVK.dylib CACHE FILEPATH "Path to MoltenVK library." FORCE)
			endif()
		endif()
		message(STATUS "Using Vulkan: " ${Vulkan_LIBRARY})
	endif()

	# Eric: For use with SDL2/Vulkan and/or for vulkan
	if(UNIX OR SDL2)
		# SRS - For UNIX-like target OSs use the generic SDL Vulkan platform
		# Or for when we build SDL2
		add_definitions(-DVULKAN_USE_PLATFORM_SDL)
	elseif(WIN32)
		add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
	endif()
else()
	add_definitions(-DGLEW_STATIC)
endif()

##################### Dependencies ###########################

set(sys_thirdparty)

if(FFMPEG)
	find_package(FFMPEG REQUIRED)
	include_directories(${FFMPEG_INCLUDE_DIRS})
	add_definitions(-DUSE_FFMPEG)
	list(APPEND sys_thirdparty ${FFMPEG_LIBRARIES})
endif()

if(OPENAL)
	find_package(OpenAL REQUIRED)
	include_directories(${OPENAL_INCLUDE_DIR})
	add_definitions(-DUSE_OPENAL)
	list(APPEND sys_thirdparty ${OPENAL_LIBRARY})
endif()

if(SDL2)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	add_definitions(-DUSE_SDL)
	if(WIN32)
		list(APPEND sys_thirdparty SDL2::SDL2 SDL2::SDL2main)
	else()
		list(APPEND sys_thirdparty ${SDL2_LIBRARIES})
	endif()
endif()

if(NOT USE_VULKAN)
	find_package(OpenGL REQUIRED)
	include_directories(${OPENGL_INCLUDE_DIRS})
	if(UNIX)
		list(APPEND sys_thirdparty OpenGL::GL)
	else()
		list(APPEND sys_thirdparty OpenGL::GL OpenGL::GLU)
	endif()
endif()

if(BINKDEC)
	add_definitions(-DUSE_BINKDEC)
endif()

##################### Core ###########################

file(GLOB src_cm cm/*.cpp cm/*.h)
source_group("CM" FILES ${src_cm})

file(GLOB src_framework framework/*.cpp framework/*.h)
list(REMOVE_ITEM src_framework ${CMAKE_CURRENT_SOURCE_DIR}/framework/Session.cpp)
source_group("Framework" FILES ${src_framework})

# These files are funny
set_source_files_properties(
	${CMAKE_CURRENT_SOURCE_DIR}/framework/precompiled.cpp
	PROPERTIES
	SKIP_PRECOMPILE_HEADERS ON)

file(GLOB src_framework_imgui imgui/*.cpp imgui/*.h)
source_group("Framework\\ImGUI" FILES ${src_framework_imgui})

file(GLOB src_renderer_base renderer/*.cpp renderer/*.c renderer/*.h)
list(REMOVE_ITEM src_renderer_base ${CMAKE_CURRENT_SOURCE_DIR}/renderer/BoundsTrack.cpp ${CMAKE_CURRENT_SOURCE_DIR}/renderer/BoundsTrack.h)
source_group("Renderer" FILES ${src_renderer_base})

file(GLOB src_renderer_color renderer/Color/*.cpp renderer/Color/*.h)
source_group("Renderer\\Color" FILES ${src_renderer_color})

file(GLOB src_renderer_dxt renderer/DXT/*.cpp renderer/DXT/*.h)
source_group("Renderer\\DXT" FILES ${src_renderer_dxt})

file(GLOB src_renderer_smaa renderer/SMAA/*.h)
source_group("Renderer\\SMAA" FILES ${src_renderer_smaa})

file(GLOB src_renderer_jobs renderer/jobs/*.cpp renderer/jobs/*.h)
source_group("Renderer\\Jobs" FILES ${src_renderer_jobs})

file(GLOB src_renderer_jobs_dynamicshadowvolume renderer/jobs/dynamicshadowvolume/*.cpp renderer/jobs/dynamicshadowvolume/*.h)
source_group("Renderer\\Jobs\\DynamicShadowVolume" FILES ${src_renderer_jobs_dynamicshadowvolume})

file(GLOB src_renderer_jobs_prelightshadowvolume renderer/jobs/prelightshadowvolume/*.cpp renderer/jobs/prelightshadowvolume/*.h)
source_group("Renderer\\Jobs\\PrelightShadowVolume" FILES ${src_renderer_jobs_prelightshadowvolume})

file(GLOB src_renderer_jobs_staticshadowvolume renderer/jobs/staticshadowvolume/*.cpp renderer/jobs/staticshadowvolume/*.h)
source_group("Renderer\\Jobs\\StaticShadowVolume" FILES ${src_renderer_jobs_staticshadowvolume})

# These files are funny
set_source_files_properties(
	${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTDecoder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder_SSE2.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/dynamicshadowvolume/DynamicShadowVolume.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/prelightshadowvolume/PreLightShadowVolume.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/staticshadowvolume/StaticShadowVolume.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/ShadowShared.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/renderer/RenderLog.cpp
	PROPERTIES
	SKIP_PRECOMPILE_HEADERS ON)

#always_optimize_sourcefile(renderer/stblib_impls.c)

file(GLOB src_sound sound/*.cpp sound/*.c sound/*.h)
source_group("Sound" FILES ${src_sound})

#always_optimize_sourcefile(sound/stbvorbis_impl.c)

file(GLOB src_swf swf/*.cpp swf/*.h)
source_group("SWF" FILES ${src_swf})

file(GLOB src_sys sys/*.cpp sys/*.h)
source_group("Sys" FILES ${src_sys})

file(GLOB src_sys_common sys/common/*.cpp sys/common/*.h)
source_group("Sys\\Common" FILES ${src_sys_common})

if(SDL2)
	file(GLOB src_sys_sdl sys/sdl/*.cpp sys/sdl/*.h)
	source_group("Sys//SDL" FILES ${src_sys_sdl})

	#if(WIN32)
	#	get_filename_component(sdl_events_cpp_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/sdl/sdl_events.cpp ABSOLUTE)
	#	list(REMOVE_ITEM src_sys_sdl "${sdl_events_cpp_full_path}")

	#	get_filename_component(sdl2_scancode_mappings_h_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/sdl/sdl2_scancode_mappings.h ABSOLUTE)
	#	list(REMOVE_ITEM src_sys_sdl "${sdl2_scancode_mappings_h_full_path}")
	#endif()

	# Eric: Utilize either the Vulkan or GL implementation of SDL
	if(USE_VULKAN)
		get_filename_component(sdl_glimp_cpp_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/sdl/sdl_glimp.cpp ABSOLUTE)
		list(REMOVE_ITEM src_sys_sdl "${sdl_glimp_cpp_full_path}")
	else()
		get_filename_component(sdl_vkimp_cpp_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/sdl/sdl_vkimp.cpp ABSOLUTE)
		list(REMOVE_ITEM src_sys_sdl "${sdl_vkimp_cpp_full_path}")
	endif()
endif()

file(GLOB src_ui ui/*.cpp ui/*.h)
list(REMOVE_ITEM src_ui ${CMAKE_CURRENT_SOURCE_DIR}/ui/GameWindow.cpp)
source_group("UI" FILES ${src_ui})

set(src_core
	${src_cm}
	${src_framework}
	${src_framework_imgui}
	${src_renderer_base}
	${src_renderer_color}
	${src_renderer_dxt}
	${src_renderer_smaa}
	${src_renderer_jobs}
	${src_renderer_jobs_dynamicshadowvolume}
	${src_renderer_jobs_prelightshadowvolume}
	${src_renderer_jobs_staticshadowvolume}
	${src_sound}
	${src_swf}
	${src_sys}
	${src_sys_sdl}
	${src_sys_common}
	${src_ui})

##################### Game-D3XP ###########################

file(GLOB src_d3xp_base d3xp/*.cpp d3xp/*.h d3xp/game.def)
list(REMOVE_ITEM src_d3xp_base ${CMAKE_SOURCE_DIR}/d3xp/precompiled.cpp ${CMAKE_SOURCE_DIR}/d3xp/EndLevel.cpp ${CMAKE_SOURCE_DIR}/d3xp/EndLevel.h)
source_group("Game-D3XP" FILES ${src_d3xp_base})

file(GLOB src_d3xp_ai d3xp/ai/*.cpp d3xp/ai/*.h)
source_group("Game-D3XP\\AI" FILES ${src_d3xp_ai})

file(GLOB src_d3xp_anim d3xp/anim/*.cpp d3xp/anim/*.h)
source_group("Game-D3XP\\Anim" FILES ${src_d3xp_anim})

file(GLOB src_d3xp_gamesys d3xp/gamesys/*.cpp d3xp/gamesys/*.h)
list(REMOVE_ITEM src_d3xp_gamesys ${CMAKE_SOURCE_DIR}/d3xp/gamesys/Callbacks.cpp)
source_group("Game-D3XP\\GameSys" FILES ${src_d3xp_gamesys})

file(GLOB src_d3xp_physics d3xp/physics/*.cpp d3xp/physics/*.h)
source_group("Game-D3XP\\Physics" FILES ${src_d3xp_physics})

file(GLOB src_d3xp_script d3xp/script/*.cpp d3xp/script/*.h)
source_group("Game-D3XP\\Script" FILES ${src_d3xp_script})

file(GLOB src_d3xp_menus d3xp/menus/*.cpp d3xp/menus/*.h)
source_group("Game-D3XP\\Menus" FILES ${src_d3xp_menus})

set(src_d3xp
	${src_d3xp_base}
	${src_d3xp_ai}
	${src_d3xp_anim}
	${src_d3xp_gamesys}
	${src_d3xp_physics}
	${src_d3xp_script}
	${src_d3xp_menus})

list(APPEND src_core ${src_d3xp})

##################### Compilers ###########################

file(GLOB src_tools_compilers tools/compilers/*.h)
source_group("Tools\\Compilers" FILES ${src_tools_compilers})

file(GLOB src_dmap tools/compilers/dmap/*.cpp tools/compilers/dmap/*.h)
source_group("Tools\\Compilers\\Dmap" FILES ${src_dmap})

file(GLOB src_aas tools/compilers/aas/*.cpp tools/compilers/aas/*.h)
source_group("Tools\\Compilers\\AAS" FILES ${src_aas})

file(GLOB src_roq tools/compilers/roqvq/*.cpp tools/compilers/roqvq/*.h)
source_group("Tools\\Compilers\\RoqVq" FILES ${src_roq})

file(GLOB src_renderbump tools/compilers/renderbump/*.cpp tools/compilers/renderbump/*.h)
source_group("Tools\\Compilers\\RenderBump" FILES ${src_renderbump})

file(GLOB src_amplitude tools/compilers/amplitude/*.cpp tools/compilers/amplitude/*.h)
source_group("Tools\\Compilers\\Amplitude" FILES ${src_amplitude})

file(GLOB src_debugger_server tools/debugger/server/*.cpp tools/debugger/server/*.h)
source_group("Tools\\Debugger\\Server" FILES ${src_debugger_server})

set(src_compilers
	${src_tools_compilers}
	${src_dmap}
	${src_aas}
	${src_roq}
	${src_renderbump}
	${src_amplitude}
	${src_debugger_server})

list(APPEND src_core ${src_compilers})

##################### Tools ImGUI ###########################

file(GLOB src_imgui_editor tools/imgui/*.cpp tools/imgui/*.h)
source_group("Tools\\ImGUI" FILES ${src_imgui_editor})

file(GLOB src_imgui_editor_utils tools/imgui/util/*.cpp tools/imgui/util/*.h)
source_group("Tools\\ImGUI\\Util" FILES ${src_imgui_editor_utils})

file(GLOB src_imgui_lighteditor tools/imgui/lighteditor/*.cpp tools/imgui/lighteditor/*.h)
source_group("Tools\\ImGUI\\LightEditor" FILES ${src_imgui_lighteditor})

file(GLOB src_imgui_afeditor tools/imgui/afeditor/*.cpp tools/imgui/afeditor/*.h)
source_group("Tools\\ImGUI\\AfEditor" FILES ${src_imgui_afeditor})

set(src_tools_imgui
	${src_imgui_editor}
	${src_imgui_editor_utils}
	${src_imgui_lighteditor}
	${src_imgui_afeditor})

list(APPEND src_core ${src_tools_imgui})

##################### Tools MFC ###########################

file(GLOB src_tools_base tools/*.h)
source_group("Tools" FILES ${src_tools_base})

file(GLOB src_comafx tools/comafx/*.cpp tools/comafx/*.h)
source_group("Tools\\ComAfxs" FILES ${src_comafx})

file(GLOB src_common tools/common/*.cpp tools/common/*.h)
source_group("Tools\\Common" FILES ${src_common})

file(GLOB src_proptree tools/common/PropTree/*.cpp tools/common/PropTree/*.h)
source_group("Tools\\Common\\PropTree" FILES ${src_proptree})

file(GLOB src_afeditor tools/af/*.cpp tools/af/*.h)
source_group("Tools\\AF" FILES ${src_afeditor})

file(GLOB src_debugger_client tools/debugger/client/*.cpp tools/debugger/client/*.h)
source_group("Tools\\Debugger\\Client" FILES ${src_debugger_client})

file(GLOB src_decleditor tools/decl/*.cpp tools/decl/*.h)
source_group("Tools\\Decl" FILES ${src_decleditor})

file(GLOB src_guieditor tools/guied/*.cpp tools/guied/*.h)
source_group("Tools\\GuiEd" FILES ${src_guieditor})

file(GLOB src_materialeditor tools/materialeditor/*.cpp tools/materialeditor/*.h)
source_group("Tools\\MaterialEditor" FILES ${src_materialeditor})

file(GLOB src_particleeditor tools/particle/*.cpp tools/particle/*.h)
source_group("Tools\\Particle" FILES ${src_particleeditor})

file(GLOB src_pdaeditor tools/pda/*.cpp tools/pda/*.h)
source_group("Tools\\PDA" FILES ${src_pdaeditor})

file(GLOB src_radianteditor tools/radiant/*.cpp tools/radiant/*.h)
source_group("Tools\\Radiant" FILES ${src_radianteditor})

file(GLOB src_scripteditor tools/script/*.cpp tools/script/*.h)
source_group("Tools\\Script" FILES ${src_scripteditor})

file(GLOB src_soundeditor tools/sound/*.cpp tools/sound/*.h)
source_group("Tools\\Sound" FILES ${src_soundeditor})

set(src_tools
	${src_tools_base}
	${src_comafx}
	${src_common}
	${src_proptree}
	${src_afeditor}
	${src_debugger_client}
	${src_decleditor}
	${src_guieditor}
	${src_materialeditor}
	${src_particleeditor}
	${src_pdaeditor}
	${src_radianteditor}
	${src_scripteditor}
	${src_soundeditor})

include_directories(${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/idlib ${CMAKE_SOURCE_DIR}/extern)

add_subdirectory(idlib)
add_subdirectory(extern)
if(USE_INTRINSICS_SSE)
add_subdirectory(extern/moc)
list(APPEND sys_thirdparty MaskedOcclusionCulling)
endif()

add_definitions(-D__DOOM__)

if(MSVC)
	if(USE_VULKAN)
		list(APPEND sys_thirdparty ${Vulkan_LIBRARY})

		if(SPIRV_SHADERC)
			list(APPEND sys_thirdparty shaderc_combined)
		else()
			list(APPEND sys_thirdparty glslang SPIRV)

			if(ENABLE_GLSLANG_BINARIES)
				list(APPEND sys_thirdparty glslang-default-resource-limits)
			endif()
		endif()

		file(GLOB src_renderer_vulkan renderer/Vulkan/*.cpp renderer/Vulkan/*.h)
		source_group("Renderer\\Vulkan" FILES ${src_renderer_vulkan})

		list(APPEND src_core ${src_renderer_vulkan})
	else()
		file(GLOB src_renderer_opengl renderer/OpenGL/*.cpp renderer/OpenGL/*.h)
		source_group("Renderer\\OpenGL" FILES ${src_renderer_opengl})

		list(APPEND src_core ${src_renderer_opengl})
	endif()

	if(OPENAL)
		file(GLOB src_sound_openal sound/OpenAL/*.cpp sound/OpenAL/*.h)
		source_group("Sound\\OpenAL" FILES ${src_sound_openal})

		list(APPEND src_core ${src_sound_openal})
	else()
		file(GLOB src_sound_xaudio2 sound/XAudio2/*.cpp sound/XAudio2/*.h)
		source_group("Sound\\XAudio2" FILES ${src_sound_xaudio2})

		list(APPEND src_core ${src_sound_xaudio2})
	endif()

	if(TOOLS)
		list(APPEND src_core ${src_tools})
	endif()

	file(GLOB src_sys_win32 sys/win32/*.cpp sys/win32/*.h)
	list(REMOVE_ITEM src_sys_win32
		${CMAKE_CURRENT_SOURCE_DIR}/sys/win32/win_gamma.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sys/win32/win_snd.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sys/win32/win_stats.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sys/win32/win_stats.h)
	source_group("Sys//Win32" FILES ${src_sys_win32})

	if(SDL2)
		get_filename_component(win_input_cpp_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/win32/win_input.cpp ABSOLUTE)
		list(REMOVE_ITEM src_sys_win32 "${win_input_cpp_full_path}")

		get_filename_component(win_input_h_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/win32/win_input.h ABSOLUTE)
		list(REMOVE_ITEM src_sys_win32 "${win_input_h_full_path}")

		get_filename_component(win_glimp_cpp_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/win32/win_glimp.cpp ABSOLUTE)
		list(REMOVE_ITEM src_sys_win32 "${win_glimp_cpp_full_path}")

		get_filename_component(win_wndproc_cpp_full_path ${CMAKE_CURRENT_SOURCE_DIR}/sys/win32/win_wndproc.cpp ABSOLUTE)
		list(REMOVE_ITEM src_sys_win32 "${win_wndproc_cpp_full_path}")
	endif()

	if(NOT MSVC)
		list(REMOVE_ITEM src_sys_win32 ${CMAKE_SOURCE_DIR}/sys/win32/win_cpu.cpp)
	endif()

	file(GLOB_RECURSE src_sys_win32_rc sys/win32/rc/res/*.ico sys/win32/rc/*.manifest sys/win32/rc/*.rc sys/win32/rc/*.h)
	source_group("Sys//Win32//RC" FILES ${src_sys_win32_rc})

	list(APPEND src_core ${src_sys_win32} ${src_sys_win32_rc})

else()

	file(GLOB src_sys_posix sys/posix/*.cpp sys/posix/*.h)
	list(REMOVE_ITEM src_sys_posix
		${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/platform_linux.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/platform_osx.cpp)
	source_group("Sys//Posix" FILES ${src_sys_posix})

	if(UNIX)
		if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
			list(APPEND src_sys_posix ${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/platform_linux.cpp)
		else()
			list(APPEND src_sys_posix ${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/platform_osx.cpp)
		endif()
	endif()

	list(APPEND src_core ${src_sys_posix})

	if(UNIX)
		if(OPENAL)
			# SRS - Added support for OpenAL Soft headers on OSX (vs default macOS SDK headers)
			if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT OPENAL_INCLUDE_DIR MATCHES "SDKs/MacOSX.*\.sdk")
				include_directories(${OPENAL_INCLUDE_DIR})
				add_definitions(-DUSE_OPENAL_SOFT_INCLUDES)
			endif()

			file(GLOB src_sound_openal sound/OpenAL/*.cpp sound/OpenAL/*.h)
			source_group("Sound\\OpenAL" FILES ${src_sound_openal})

			list(APPEND src_core ${src_sound_openal})
		else()
			file(GLOB src_sound_stub sound/stub/*.cpp sound/stub/*.h)
			source_group("Sound\\Stub" FILES ${src_sound_stub})

			list(APPEND src_core ${src_sound_stub})
		endif()

		if(USE_VULKAN)
			set(Vulkan_LIBRARIES ${Vulkan_LIBRARY})

			# SRS - Added UNIX/OSX support for shaderc
			if(SPIRV_SHADERC)
				list(APPEND Vulkan_LIBRARIES shaderc_combined)
			else()
				list(APPEND Vulkan_LIBRARIES glslang SPIRV)

				if(ENABLE_GLSLANG_BINARIES)
					list(APPEND Vulkan_LIBRARIES glslang-default-resource-limits)
				endif()
			endif()

			file(GLOB src_renderer_vulkan renderer/Vulkan/*.cpp renderer/Vulkan/*.h)
			source_group("Renderer\\Vulkan" FILES ${src_renderer_vulkan})

			list(APPEND src_core ${src_renderer_vulkan})
		else()
			file(GLOB src_renderer_opengl renderer/OpenGL/*.cpp renderer/OpenGL/*.h)
			source_group("Renderer\\OpenGL" FILES ${src_renderer_opengl})

			list(APPEND src_core ${src_renderer_opengl})
		endif()

	endif()

	if(MACOSX_BUNDLE)
		# define contents of macOS app bundle Resources folder
		set(MACOS_RESOURCES sys/posix/res/Doom3BFG.icns ../base)
		source_group("Resources" FILES ${MACOS_RESOURCES})
		set_source_files_properties(${MACOS_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

		# enable Xcode to display the icons file properly
		set_source_files_properties(sys/posix/res/Doom3BFG.icns PROPERTIES XCODE_EXPLICIT_FILE_TYPE .icns)

		# add macOS bundle properties to executable
		set(MACOSX_BUNDLE_BUNDLE_NAME ${CMAKE_PROJECT_NAME})
		set(MACOSX_BUNDLE_BUNDLE_VERSION ${CMAKE_PROJECT_VERSION})
		set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${CMAKE_PROJECT_VERSION})
		set(MACOSX_BUNDLE_ICON_FILE Doom3BFG.icns)
		set(MACOSX_BUNDLE_INFO_STRING "${CMAKE_PROJECT_DESCRIPTION}")
		set(MACOSX_BUNDLE_COPYRIGHT "")						# to be added by author if desired
	endif()
endif()

add_executable(${CMAKE_PROJECT_NAME} MACOSX_BUNDLE WIN32 ${src_core})
add_dependencies(${CMAKE_PROJECT_NAME} idLib external)

target_link_libraries(${CMAKE_PROJECT_NAME} idLib external ${sys_libs} ${sys_thirdparty})
if(USE_PRECOMPILED_HEADERS)
	target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/idlib/precompiled.h)
endif()

if(MSVC)
	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/MAP:\"${EXECUTABLE_OUTPUT_PATH}/${CMAKE_PROJECT_NAME}.map\"")

	# set MSVC default debug directory on executable
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME}) # set MSVC startup project
	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")
	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "+set r_fullscreen 0 +set win_viewlog 1 +set developter 1")

	#CMAKE_BINARY_DIR
	if(CMAKE_CL_64)
		install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION .)
	else()
		install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION .)
	endif()
endif()