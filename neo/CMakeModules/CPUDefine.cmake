if( NOT CMAKE_SYSTEM_PROCESSOR )
	message( FATAL_ERROR "No target CPU architecture set" )
endif()

if( NOT CMAKE_SYSTEM_NAME )
	message( FATAL_ERROR "No target OS set" )
endif()

# target library
add_definitions( -DLIBRARYSUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}" )

# target cpu
set( cpu ${CMAKE_SYSTEM_PROCESSOR} )

# Originally, ${CMAKE_SYSTEM_PROCESSOR} was supposed to contain the *target* CPU, according to CMake's documentation.
# As far as I can tell this has always been broken (always returns host CPU) at least on Windows
# (see e.g. https://cmake.org/pipermail/cmake-developers/2014-September/011405.html) and wasn't reliable on
# other systems either, for example on Linux with 32bit userland but 64bit kernel it returned the kernel CPU type
# (e.g. x86_64 instead of i686). Instead of fixing this, CMake eventually updated their documentation in 3.20,
# now it's officially the same as CMAKE_HOST_SYSTEM_PROCESSOR except when cross-compiling (where it's explicitly set)
# So we gotta figure out the actual target CPU type ourselves.. (why am I sticking to this garbage buildsystem?)
if( NOT ( CMAKE_SYSTEM_PROCESSOR STREQUAL CMAKE_HOST_SYSTEM_PROCESSOR ) )
	# special case: cross-compiling, here CMAKE_SYSTEM_PROCESSOR should be correct, hopefully
	# (just leave cpu at ${CMAKE_SYSTEM_PROCESSOR})
elseif( MSVC )
	# because all this wasn't ugly enough, it turned out that, unlike standalone CMake, Visual Studio's
	# integrated CMake doesn't set CMAKE_GENERATOR_PLATFORM, so I gave up on guessing the CPU arch here
	# and moved the CPU detection to MSVC-specific code in neo/sys/platform.h
else() # not MSVC and not cross-compiling, assume GCC or clang (-compatible), seems to work for MinGW as well
	execute_process( COMMAND ${CMAKE_C_COMPILER} "-dumpmachine"
					RESULT_VARIABLE cc_dumpmachine_res
					OUTPUT_VARIABLE cc_dumpmachine_out )
	if( cc_dumpmachine_res EQUAL 0 )
		string( STRIP ${cc_dumpmachine_out} cc_dumpmachine_out ) # get rid of trailing newline
		message( STATUS "`${CMAKE_C_COMPILER} -dumpmachine` says: \"${cc_dumpmachine_out}\"" )
		# gcc -dumpmachine and clang -dumpmachine seem to print something like "x86_64-linux-gnu" (gcc)
		# or "x64_64-pc-linux-gnu" (clang) or "i686-w64-mingw32" (32bit mingw-w64) i.e. starting with the CPU,
		# then "-" and then OS or whatever - so use everything up to first "-"
		string( REGEX MATCH "^[^-]+" cpu ${cc_dumpmachine_out} )
		message( STATUS "  => CPU architecture extracted from that: \"${cpu}\"" )
	else()
		message( WARNING "${CMAKE_C_COMPILER} -dumpmachine failed with error (code) ${cc_dumpmachine_res}" )
		message( WARNING "will use the (sometimes incorrect) CMAKE_SYSTEM_PROCESSOR (${cpu}) to determine CPUSTRING" )
	endif()
endif()

if( cpu STREQUAL "powerpc" )
	set( cpu "ppc" )
elseif( cpu STREQUAL "aarch64" )
	# "arm64" is more obvious, and some operating systems (like macOS) use it instead of "aarch64"
	set( cpu "arm64" )
elseif( cpu MATCHES "i.86" )
	set( cpu "x86" )
elseif( cpu MATCHES "[aA][mM][dD]64" OR cpu MATCHES "[xX]64" )
	set( cpu "x86_64" )
elseif( cpu MATCHES "[aA][rR][mM].*" ) # some kind of arm..
	# On 32bit Raspbian gcc -dumpmachine returns sth starting with "arm-",
	# while clang -dumpmachine says "arm6k-..." - try to unify that to "arm"
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 ) # sizeof(void*) == 8 => must be arm64
		set( cpu "arm64" )
	else() # should be 32bit arm then (probably "armv7l" "armv6k" or sth like that)
		set( cpu "arm" )
	endif()
endif()

# SRS - Turn on optimization when cross-compiling from Apple arm64 to x86_64
if ( cpu MATCHES "(x86)|(X86)|(amd64)|(AMD64)" OR cpu MATCHES "x86_64" )
	set( CPUOPTIMIZATION "-mmmx -msse -msse2" CACHE STRING "Which CPU specific optimitations should be used beside the compiler's default?" )
endif()

# SRS - Turn off MMX/SSE intrinsics when cross-compiling from Apple x86_64 to arm64
if( cpu MATCHES "(x86)|(X86)|(amd64)|(AMD64)|(e2k)|(E2K)" AND NOT CMAKE_OSX_ARCHITECTURES MATCHES "arm64" )
	option( USE_INTRINSICS_SSE "Compile using SSE intrinsics (e.g mmx, sse, msse2)" ON )
else()
	option( USE_INTRINSICS_SSE "Compile using SSE intrinsics (e.g mmx, sse, msse2)" OFF )
endif()

if( CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang" )
	if ( CPUOPTIMIZATION )
		add_definitions( ${CPUOPTIMIZATION} )
	endif()
endif()

# target os
if( APPLE )
	set( os "macosx" )
else()
	string( TOLOWER "${CMAKE_SYSTEM_NAME}" os )
endif()

# os specific settings
if ( os STREQUAL "windows" )
	set( os_id 0 )
elseif ( os STREQUAL "linux" )
	set( os_id 1 )
elseif ( os STREQUAL "macosx" )
	set( os_id 2 )
endif()

add_definitions( -DOSTYPE="${os}" -DOS_ID=${os_id} -DD3_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P} )

if( MSVC )
	# for MSVC CPUSTRING is set in code (in neo/sys/platform.h)
	message( STATUS "Setting -DD3_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P} -DOSTYPE=\"${os}\" -DOS_ID=${os_id} - NOT setting CPUSTRING, because we're targeting MSVC (VisualC++)" )
	# make sure ${cpu} isn't (cant't be) used by CMake when building with MSVC
	unset( cpu )
else()
	add_definitions( -DCPUSTRING="${cpu}" )
	message( STATUS "Setting -DCPUSTRING=\"${cpu}\" -DOS_ID=${os_id} -DD3_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P} -DOSTYPE=\"${os}\" " )

	if( cpu MATCHES ".*64.*" AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8 )
		# tough luck if some CPU architecture has "64" in its name but uses 32bit pointers
		message( SEND_ERROR "CMake thinks sizeof(void*) == 4, but the target CPU ${cpu} looks like a 64bit CPU!" )
		message( FATAL_ERROR "If you're building in a 32bit chroot on a 64bit host, switch to it with 'linux32 chroot' or at least call cmake with linux32 (or your OSs equivalent)!" )
	endif()
endif()