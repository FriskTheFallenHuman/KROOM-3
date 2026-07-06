#!/bin/sh

BUILD_DIR=build/clang/debug
DIRECTORY="clang"
BUILDTYPE="debug"
CMAKE_BUILD_TYPE="Debug"
CMAKE_OSX_ARCHITECTURES=""

# default to MoltenVK when the Vulkan SDK is present, OpenGL otherwise
if [ -n "$VULKAN_SDK" ]; then
	RENDERER="moltenvk"
else
	RENDERER="opengl"
fi

usage()
{
	echo "Usage: cmake_macos.sh <clang|clang-universal> <debug|release|reldeb> [opengl|vulkan|moltenvk]"
	echo ""
	echo "  clang             host architecture (arm64 on Apple Silicon, x86_64 on Intel)"
	echo "  clang-universal   universal binary (arm64 + x86_64)"
	echo "  debug|release|reldeb   CMake build type"
	echo "  opengl            OpenGL 4.1 + GLEW backend (default when VULKAN_SDK is unset)"
	echo "  vulkan            Vulkan loader backend (defaults VULKAN_SDK to /usr/local if unset)"
	echo "  moltenvk          link directly to libMoltenVK.dylib (default when VULKAN_SDK is set)"
	echo ""
	echo "Optional environment variables:"
	echo "  MACOSX_DEPLOYMENT_TARGET  minimum macOS version (e.g. 11.0)"
	echo "  MACOSX_SYSROOT            path to a macOS SDK"
	echo "  VULKAN_SDK                path to the Vulkan/MoltenVK SDK"
	echo ""
	echo "Homebrew dependencies (OpenGL path): brew install cmake ninja sdl2 openal-soft glew"
	exit 1
}

if [ "$#" -lt "2" ]; then
	usage
fi

# arg 1: compiler / architecture
case "$1" in
	clang)
		export CXX="clang++"
		export CC="clang"
		DIRECTORY="clang"
		;;
	clang-universal)
		export CXX="clang++"
		export CC="clang"
		DIRECTORY="clang-universal"
		CMAKE_OSX_ARCHITECTURES="arm64;x86_64"
		;;
	*)
		echo "Unknown compiler/architecture: $1"
		usage
		;;
esac

# arg 2: build type
case "$2" in
	debug)   BUILDTYPE="debug";          CMAKE_BUILD_TYPE="Debug" ;;
	release) BUILDTYPE="release";        CMAKE_BUILD_TYPE="Release" ;;
	reldeb)  BUILDTYPE="relwithdebinfo"; CMAKE_BUILD_TYPE="RelWithDebInfo" ;;
	*)
		echo "Unknown build type: $2"
		usage
		;;
esac

# arg 3 (optional): renderer backend
if [ "$#" -ge "3" ]; then
	RENDERER="$3"
fi

case "$RENDERER" in
	opengl)
		RENDERER_FLAGS="-DUSE_VULKAN=OFF"
		;;
	vulkan)
		RENDERER_FLAGS="-DUSE_VULKAN=ON -DUSE_MoltenVK=OFF"
		;;
	moltenvk)
		RENDERER_FLAGS="-DUSE_VULKAN=ON -DUSE_MoltenVK=ON"
		if [ -z "$VULKAN_SDK" ]; then
			echo "ERROR: 'moltenvk' requires VULKAN_SDK to point at the LunarG Vulkan SDK for macOS"
			echo "       (it bundles MoltenVK under \$VULKAN_SDK/../MoltenVK/)."
			echo "       Install it from https://vulkan.lunarg.com/ or use:"
			echo "         ./cmake_macos.sh $1 $2 opengl"
			echo "         ./cmake_macos.sh $1 $2 vulkan"
			exit 1
		fi
		;;
	*)
		echo "Unknown renderer: $RENDERER"
		usage
		;;
esac

# pick Ninja when available, fall back to Unix Makefiles
if command -v ninja >/dev/null 2>&1; then
	CMAKE_GENERATOR="Ninja"
else
	CMAKE_GENERATOR="Unix Makefiles"
	echo "==> ninja not found, falling back to Unix Makefiles ('brew install ninja' for faster builds)"
fi

BUILDDIR=build/$DIRECTORY/$BUILDTYPE

mkdir -p "$BUILDDIR"
cd "$BUILDDIR" || exit 1

echo "==> Build dir     : $BUILDDIR"
echo "==> Compiler      : $CC / $CXX"
echo "==> Architecture  : ${CMAKE_OSX_ARCHITECTURES:-host}"
echo "==> Build type    : $CMAKE_BUILD_TYPE"
echo "==> Renderer      : $RENDERER"
echo "==> Generator     : $CMAKE_GENERATOR"
[ -n "$MACOSX_DEPLOYMENT_TARGET" ] && echo "==> Min macOS ver : $MACOSX_DEPLOYMENT_TARGET"
[ -n "$MACOSX_SYSROOT" ]           && echo "==> SDK sysroot  : $MACOSX_SYSROOT"
[ -n "$VULKAN_SDK" ]               && echo "==> VULKAN_SDK   : $VULKAN_SDK"
echo ""

cmake -G"$CMAKE_GENERATOR" \
	-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE \
	-DFORCE_COLOR_OUTPUT=ON \
	$RENDERER_FLAGS \
	${CMAKE_OSX_ARCHITECTURES:+-DCMAKE_OSX_ARCHITECTURES=$CMAKE_OSX_ARCHITECTURES} \
	${MACOSX_DEPLOYMENT_TARGET:+-DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET} \
	${MACOSX_SYSROOT:+-DCMAKE_OSX_SYSROOT=$MACOSX_SYSROOT} \
	../../../neo
