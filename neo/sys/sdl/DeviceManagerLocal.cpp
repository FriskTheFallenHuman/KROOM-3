/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012-2014 Robert Beckebans
Copyright (C) 2013 Daniel Gibson

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#if !defined(USE_VULKAN)
	#ifdef _WIN32
		#include "glew/include/GL/glew.h"
	#else
		#include <GL/glew.h>
	#endif
#endif

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL2/SDL.h>
#if defined(USE_VULKAN)
	#include <SDL2/SDL_vulkan.h>
	#include <vulkan/vulkan.h>

	// SRS - optinally needed for VK_MVK_MOLTENVK_EXTENSION_NAME visibility
	#if defined(__APPLE__) && defined(USE_MoltenVK)
		#include <MoltenVK/vk_mvk_moltenvk.h>
	#endif
#endif

#include "../renderer/RenderCommon.h"
#include "DeviceSDL.h"
#include "DeviceManagerLocal.h"

#if !defined(USE_VULKAN)
	// RB: FIXME this shit. We need the OpenGL alpha channel for advanced rendering effects
	idCVar SDLVars_t::sdl_waylandcompat( "sdl_waylandcompat", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "wayland compatible framebuffer" );
#endif

/*
========================
idDeviceManagerSDL::TestSwapBuffers
========================
*/
void idDeviceManagerSDL::TestSwapBuffers( const idCmdArgs& args )
{
	if( !sdl.window )
	{
		common->Printf( "No window available.\n" );
		return;
	}

#if !defined(USE_VULKAN)
	common->Printf( "idDeviceManagerSDL::TestSwapBuffers\n" );
	static const int MAX_FRAMES = 5;
	uint64 timestamps[MAX_FRAMES];

	int oldInterval = SDL_GL_GetSwapInterval();

	// Disable scissor test for consistent timing
	GLboolean scissorEnabled = glIsEnabled( GL_SCISSOR_TEST );
	if( scissorEnabled )
	{
		glDisable( GL_SCISSOR_TEST );
	}

	// Test intervals: -1 (adaptive vsync if available), 0 (vsync off), 1 (vsync on), 2 (double vsync)
	int intervals[] = { -1, 0, 1, 2 };
	for( int idx = 0; idx < 4; idx++ )
	{
		int swapInterval = intervals[idx];

		// Try to set the interval; skip if unsupported
		if( SDL_GL_SetSwapInterval( swapInterval ) != 0 )
		{
			common->Printf( "Swap interval %d not supported: %s\n", swapInterval, SDL_GetError() );
			continue;
		}

		// Allow a few frames to settle
		for( int i = 0; i < MAX_FRAMES; i++ )
		{
			if( swapInterval == -1 && i == 0 )
			{
				SDL_Delay( 16 );    // ~60 Hz frame time
			}

			if( i & 1 )
			{
				glClearColor( 0, 1, 0, 1 );
			}
			else
			{
				glClearColor( 1, 0, 0, 1 );
			}

			glClear( GL_COLOR_BUFFER_BIT );
			SDL_GL_SwapWindow( sdl.window );
			glFinish();

			timestamps[i] = SDL_GetPerformanceCounter();
		}

		common->Printf( "\nswapinterval %i\n", swapInterval );
		uint64 freq = SDL_GetPerformanceFrequency();
		for( int i = 1; i < MAX_FRAMES; i++ )
		{
			uint64 dt = timestamps[i] - timestamps[i - 1];
			int microseconds = ( int )( ( dt * 1000000 ) / freq );
			common->Printf( "%i microseconds\n", microseconds );
		}
	}

	SDL_GL_SetSwapInterval( oldInterval );

	if( scissorEnabled )
	{
		glEnable( GL_SCISSOR_TEST );
	}
#else
	common->Printf( "Not implemented under vulkan\n" );
#endif
}

/*
========================
idDeviceManagerSDL::SaveGamma
========================
*/
void idDeviceManagerSDL::SaveGamma()
{
	if( SDL_GetWindowGammaRamp( sdl.window, oldHardwareGamma[0], oldHardwareGamma[1], oldHardwareGamma[2] ) == 0 )
	{
		gammaSaved = true;
		common->Printf( "Saved initial gamma ramp.\n" );
	}
	else
	{
		common->Printf( "Could not save initial gamma ramp: %s\n", SDL_GetError() );
	}
}

/*
========================
idDeviceManagerSDL::RestoreGamma
========================
*/
void idDeviceManagerSDL::RestoreGamma()
{
	if( gammaSaved )
	{
		if( SDL_SetWindowGammaRamp( sdl.window, oldHardwareGamma[0], oldHardwareGamma[1], oldHardwareGamma[2] ) != 0 )
		{
			common->Warning( "Failed to restore gamma ramp: %s", SDL_GetError() );
		}
		else
		{
			common->Printf( "Restored initial gamma ramp.\n" );
		}
		gammaSaved = false;
	}
}


/*
========================
idDeviceManagerSDL::SetGamma

The renderer calls this when the user adjusts r_gamma or r_brightness
========================
*/
void idDeviceManagerSDL::SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
{
	if( !sdl.window )
	{
		common->Warning( "GLimp_SetGamma called without window" );
		return;
	}

	if( SDL_SetWindowGammaRamp( sdl.window, red, green, blue ) )
	{
		common->Warning( "Couldn't set gamma ramp: %s", SDL_GetError() );
	}
}

/*
===================
idDeviceManagerSDL::PreInit

 GetModeListForDisplay is called before Init(), but SDL needs SDL_Init() first.
 So do that in PreInit()
 Calling that function more than once doesn't make a difference
===================
*/
void idDeviceManagerSDL::PreInit()
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		if( SDL_Init( SDL_INIT_VIDEO ) )
		{
			common->Error( "Error while initializing SDL: %s", SDL_GetError() );
		}
	}
}


/*
===================
idDeviceManagerSDL::Init
===================
*/
bool idDeviceManagerSDL::Init( vidParms_t parms )
{
	cmdSystem->AddCommand( "testSwapBuffers", idDeviceManagerSDL::TestSwapBuffers, CMD_FL_SYSTEM, "Times swapbuffer options" );

	common->Printf( "Initializing render subsystem with multisamples:%i fullscreen:%i\n",
					parms.multiSamples, parms.fullScreen );

	PreInit(); // DG: make sure SDL is initialized

	// DG: make window resizable
	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	// DG end

	if( parms.fullScreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	int colorbits = 24;
	int depthbits = 24;
	int stencilbits = 8;

	for( int i = 0; i < 16; i++ )
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if( ( i % 4 ) == 0 && i )
		{
			// one pass, reduce
			switch( i / 4 )
			{
				case 2 :
					if( colorbits == 24 )
					{
						colorbits = 16;
					}
					break;
				case 1 :
					if( depthbits == 24 )
					{
						depthbits = 16;
					}
					else if( depthbits == 16 )
					{
						depthbits = 8;
					}
				case 3 :
					if( stencilbits == 24 )
					{
						stencilbits = 16;
					}
					else if( stencilbits == 16 )
					{
						stencilbits = 8;
					}
			}
		}

		int tcolorbits = colorbits;
		int tdepthbits = depthbits;
		int tstencilbits = stencilbits;

		if( ( i % 4 ) == 3 )
		{
			// reduce colorbits
			if( tcolorbits == 24 )
			{
				tcolorbits = 16;
			}
		}

		if( ( i % 4 ) == 2 )
		{
			// reduce depthbits
			if( tdepthbits == 24 )
			{
				tdepthbits = 16;
			}
			else if( tdepthbits == 16 )
			{
				tdepthbits = 8;
			}
		}

		if( ( i % 4 ) == 1 )
		{
			// reduce stencilbits
			if( tstencilbits == 24 )
			{
				tstencilbits = 16;
			}
			else if( tstencilbits == 16 )
			{
				tstencilbits = 8;
			}
			else
			{
				tstencilbits = 0;
			}
		}

		int channelcolorbits = 4;
		if( tcolorbits == 24 )
		{
			channelcolorbits = 8;
		}

		Uint32 driverFlag = 0; // Unknow, yet to be initialied

#if !defined(USE_VULKAN)
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );

		if( SDLVars_t::sdl_waylandcompat.GetBool() )
		{
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0 );
		}
		else
		{
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, channelcolorbits );
		}

		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples );

		const int glMajorVersion = r_glVersionMajor.GetInteger();
		const int glMinorVersion = r_glVersionMinor.GetInteger();

		// Krispy: This is Mega stupid, im sorry
		if( glMajorVersion >= 3 && glMajorVersion >= 1 )
		{
			glConfig.driverVersion = GL_OPENGL_31;
		}
		else if( glMajorVersion >= 3 && glMajorVersion >= 2 )
		{
			glConfig.driverVersion = GL_OPENGL_32;
		}
		else if( glMajorVersion >= 3 && glMajorVersion >= 3 )
		{
			glConfig.driverVersion = GL_OPENGL_33;
		}
		else if( glMajorVersion >= 4 && glMajorVersion >= 0 )
		{
			glConfig.driverVersion = GL_OPENGL_40;
		}
		else if( glMajorVersion >= 4 && glMajorVersion >= 1 )
		{
			glConfig.driverVersion = GL_OPENGL_41;
		}
		else if( glMajorVersion >= 4 && glMajorVersion >= 2 )
		{
			glConfig.driverVersion = GL_OPENGL_42;
		}
		else if( glMajorVersion >= 4 && glMajorVersion >= 3 )
		{
			glConfig.driverVersion = GL_OPENGL_43;
		}
		else if( glMajorVersion >= 4 && glMajorVersion >= 4 )
		{
			glConfig.driverVersion = GL_OPENGL_44;
		}
		else if( glMajorVersion >= 4 && glMajorVersion >= 5 )
		{
			glConfig.driverVersion = GL_OPENGL_45;
		}
		else if( glMajorVersion >= 4 && glMajorVersion >= 6 )
		{
			glConfig.driverVersion = GL_OPENGL_46;
		}

		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, glMajorVersion );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, glMinorVersion );

		int useCoreProfile = r_glProfile.GetInteger();
		int profileCore = 0;

		if( useCoreProfile == 2 )
		{
			glConfig.driverType = GLDRV_OPENGL_CORE_PROFILE;
			profileCore = SDL_GL_CONTEXT_PROFILE_CORE;
		}
		else if( useCoreProfile == 1 )
		{
			glConfig.driverType = GLDRV_OPENGL_COMPATIBILITY_PROFILE;
			profileCore = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
		}
		else
		{
			glConfig.driverType = GLDRV_OPENGL;
			profileCore = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY; // ES support?
		}

		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, profileCore ); // ES support?

		if( r_debugContext.GetBool() )
		{
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
		}

		driverFlag = SDL_WINDOW_OPENGL;
#else
		glConfig.driverType = GLDRV_VULKAN;
#if defined(__APPLE__)
		driverFlag = SDL_WINDOW_METAL;
#else
		driverFlag = SDL_WINDOW_VULKAN;
#endif
#endif

		// DG: set display num for fullscreen
		Uint32 flags = driverFlag | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

		int windowPosX = SDL_WINDOWPOS_UNDEFINED;
		int windowPosY = SDL_WINDOWPOS_UNDEFINED;

		if( parms.fullScreen == -1 )
		{
			// Borderless window at explicit coordinates (for multi‑monitor spanning)
			flags |= SDL_WINDOW_BORDERLESS;
			windowPosX = parms.x;
			windowPosY = parms.y;
		}
		else if( parms.fullScreen > 0 )
		{
			// Regular fullscreen on a specific monitor
			flags |= SDL_WINDOW_FULLSCREEN;
			if( parms.fullScreen <= SDL_GetNumVideoDisplays() )
			{
				windowPosX = SDL_WINDOWPOS_UNDEFINED_DISPLAY( parms.fullScreen - 1 );
				windowPosY = windowPosX;
			}
			else
			{
				common->Warning( "Couldn't set display to num %i because we only have %i displays", parms.fullScreen, SDL_GetNumVideoDisplays() );
			}
		}
		else // parms.fullScreen == 0 (windowed) or -2 (fullscreen on current display)
		{
			// For windowed mode, use provided coordinates (if any) or let SDL choose
			windowPosX = ( parms.x != -1 ) ? parms.x : SDL_WINDOWPOS_UNDEFINED;
			windowPosY = ( parms.y != -1 ) ? parms.y : SDL_WINDOWPOS_UNDEFINED;
		}

		// Show whether this is a 32-bit or 64-bit binary
		idStr title = va( "%s - %s %s (x%u Build)", GAME_NAME, ID__DATE__, ID__TIME__, sizeof( void* ) * 8 );

		sdl.window = SDL_CreateWindow( title.c_str(),
									   windowPosX, windowPosY,
									   parms.width, parms.height, flags );
		// DG end
#if !defined(USE_VULKAN)
		sdl.context = SDL_GL_CreateContext( sdl.window );
#endif

		if( !sdl.window )
		{
			common->DPrintf( "Couldn't set render mode %d/%d/%d: %s",
							 channelcolorbits, tdepthbits, tstencilbits, SDL_GetError() );
			continue;
		}

#if !defined(USE_VULKAN)
		if( SDL_GL_SetSwapInterval( r_swapInterval.GetInteger() ) < 0 )
		{
			common->Warning( "SDL_GL_SetSwapInterval(%d) failed: %s", r_swapInterval.GetInteger(), SDL_GetError() );
		}
#endif

		SetWindowsIcon( sdl.window );

#if defined(USE_VULKAN)
		vkcontext.sdlWindow = sdl.window;
#endif

		// RB begin
		SDL_GetWindowSize( sdl.window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );
		// RB end

		if( parms.fullScreen == -1 )
		{
			glConfig.isFullscreen = -1;
		}
		else
		{
			glConfig.isFullscreen = ( SDL_GetWindowFlags( sdl.window ) & SDL_WINDOW_FULLSCREEN ) ? parms.fullScreen : 0;
		}

		common->Printf( "Using %d color bits, %d depth, %d stencil display\n",
						channelcolorbits, tdepthbits, tstencilbits );

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;

		// RB begin
		glConfig.displayFrequency = 60;
		glConfig.multisamples = parms.multiSamples;
		// RB end

		break;
	}

	if( !sdl.window )
	{
		common->Printf( "No usable GL mode found: %s", SDL_GetError() );
		return false;
	}

#if !defined(USE_VULKAN)
#ifdef __APPLE__
	glewExperimental = GL_TRUE;
#endif

	GLenum glewResult = glewInit();
	if( GLEW_OK != glewResult )
	{
		// glewInit failed, something is seriously wrong
		common->Printf( "^3GLimp_Init() - GLEW could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else
	{
		common->Printf( "Using GLEW %s\n", glewGetString( GLEW_VERSION ) );
	}

	int originalInterval = SDL_GL_GetSwapInterval();
	if( SDL_GL_SetSwapInterval( -1 ) == 0 )
	{
		glConfig.swapControlTearAvailable = true;
		SDL_GL_SetSwapInterval( originalInterval );
	}
	else
	{
		glConfig.swapControlTearAvailable = false;
	}
#endif

#if defined(__APPLE__)
	// SRS - On OSX enable SDL2 relative mouse mode warping to capture mouse properly if outside of window
	SDL_SetHintWithPriority( SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE );
#endif

	// DG: disable cursor, we have two cursors in menu (because mouse isn't grabbed in menu)
	SDL_ShowCursor( SDL_DISABLE );
	// DG end

	SaveGamma();

	return true;
}

/*
====================
idDeviceManagerSDL::DumpAllDisplayDevices
====================
*/
void idDeviceManagerSDL::DumpAllDisplayDevices()
{
	int numDisplays = SDL_GetNumVideoDisplays();
	if( numDisplays <= 0 )
	{
		common->Printf( "No displays found.\n" );
		return;
	}

	common->Printf( "\nDumping display devices:\n" );
	for( int i = 0; i < numDisplays; i++ )
	{
		SDL_Rect bounds;
		if( SDL_GetDisplayBounds( i, &bounds ) != 0 )
		{
			common->Printf( "  Display %d: error getting bounds: %s\n", i, SDL_GetError() );
			continue;
		}

		SDL_DisplayMode mode;
		if( SDL_GetCurrentDisplayMode( i, &mode ) != 0 )
		{
			common->Printf( "  Display %d: error getting current mode: %s\n", i, SDL_GetError() );
			continue;
		}

		common->Printf( "  Display %d: '%s'\n", i, SDL_GetDisplayName( i ) );
		common->Printf( "      Bounds       : %d,%d %dx%d\n", bounds.x, bounds.y, bounds.w, bounds.h );
		common->Printf( "      Current mode : %dx%d @ %d Hz, format 0x%x\n", mode.w, mode.h, mode.refresh_rate, mode.format );

		int numModes = SDL_GetNumDisplayModes( i );
		common->Printf( "      Available modes (%d):\n", numModes );
		for( int m = 0; m < numModes && m < 20; m++ )
		{
			SDL_DisplayMode dm = {};
			if( SDL_GetDisplayMode( i, m, &dm ) == 0 )
			{
				common->Printf( "          %dx%d @ %d Hz\n", dm.w, dm.h, dm.refresh_rate );
			}
		}
		if( numModes > 20 )
		{
			common->Printf( "          ...\n" );
		}
	}
	common->Printf( "\n" );
}

/*
====================
idSort_VidMode
====================
*/
class idSort_VidMode : public idSort_Quick< vidMode_t, idSort_VidMode >
{
public:
	int Compare( const vidMode_t& a, const vidMode_t& b ) const
	{
		int wd = a.width - b.width;
		int hd = a.height - b.height;
		int fd = a.displayHz - b.displayHz;
		return ( hd != 0 ) ? hd : ( wd != 0 ) ? wd : fd;
	}
};

/*
====================
idDeviceManagerSDL::GetModeListForDisplay
====================
*/
bool idDeviceManagerSDL::GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList, const int minHeight )
{
	assert( requestedDisplayNum >= 0 );
	modeList.Clear();

	if( requestedDisplayNum >= SDL_GetNumVideoDisplays() )
	{
		common->Warning( "idDeviceManagerSDL::GetModeListForDisplay: invalid display index %d (only %d displays)", requestedDisplayNum, SDL_GetNumVideoDisplays() );
		return false;
	}

	int numModes = SDL_GetNumDisplayModes( requestedDisplayNum );
	if( numModes <= 0 )
	{
		common->Warning( "idDeviceManagerSDL::GetModeListForDisplay: no modes for display %d", requestedDisplayNum );
		return false;
	}

	for( int i = 0; i < numModes; i++ )
	{
		SDL_DisplayMode mode = {};
		if( SDL_GetDisplayMode( requestedDisplayNum, i, &mode ) != 0 )
		{
			common->Warning( "SDL_GetDisplayMode %d failed: %s", i, SDL_GetError() );
			continue;
		}

		if( mode.h < minHeight )
		{
			continue;
		}

		vidMode_t vid = {};
		vid.width = mode.w;
		vid.height = mode.h;
		vid.displayHz = ( mode.refresh_rate > 0 ) ? mode.refresh_rate : 60;
		modeList.AddUnique( vid );
	}

	if( modeList.Num() == 0 )
	{
		common->Warning( "idDeviceManagerSDL::GetModeListForDisplay: no modes passed filtering (minHeight=%d)", minHeight );
		return false;
	}

	modeList.SortWithTemplate( idSort_VidMode() );
	return true;
}

/*
====================
idDeviceManagerSDL::GetDefaultDisplayMode
====================
*/
bool idDeviceManagerSDL::GetDefaultDisplayMode( int& defaultDisplayNum, vidMode_t& defaultMode )
{
	defaultDisplayNum = 0;

	if( defaultDisplayNum >= SDL_GetNumVideoDisplays() )
	{
		common->Warning( "R_GetDefaultDisplayMode: no displays available" );
		return false;
	}

	SDL_DisplayMode current = {};
	if( SDL_GetCurrentDisplayMode( defaultDisplayNum, &current ) != 0 )
	{
		common->Warning( "SDL_GetCurrentDisplayMode failed: %s", SDL_GetError() );
		return false;
	}

	defaultMode.width = current.w;
	defaultMode.height = current.h;
	defaultMode.displayHz = ( current.refresh_rate > 0 ) ? current.refresh_rate : 60;
	return true;
}

/*
===================
idDeviceManagerSDL::ScreenParmsHandleDisplayIndex

Helper functions for SetScreenParms()
===================
*/
int idDeviceManagerSDL::ScreenParmsHandleDisplayIndex( vidParms_t parms )
{
	int displayIdx;
	if( parms.fullScreen > 0 )
	{
		displayIdx = parms.fullScreen - 1; // first display for SDL is 0, in parms it's 1
	}
	else // -2 == use current display
	{
		displayIdx = SDL_GetWindowDisplayIndex( sdl.window );
		if( displayIdx < 0 ) // for some reason the display for the window couldn't be detected
		{
			displayIdx = 0;
		}
	}

	if( parms.fullScreen > SDL_GetNumVideoDisplays() )
	{
		common->Warning( "Can't set fullscreen mode to display number %i, because SDL2 only knows about %i displays!",
						 parms.fullScreen, SDL_GetNumVideoDisplays() );
		return -1;
	}

	if( parms.fullScreen != glConfig.isFullscreen )
	{
		// we have to switch to another display
		if( glConfig.isFullscreen )
		{
			// if we're already in fullscreen mode but want to switch to another monitor
			// we have to go to windowed mode first to move the window.. SDL-oddity.
			SDL_SetWindowFullscreen( sdl.window, SDL_FALSE );
		}
		// select display ; SDL_WINDOWPOS_UNDEFINED_DISPLAY() doesn't work.
		int x = SDL_WINDOWPOS_CENTERED_DISPLAY( displayIdx );
		// move window to the center of selected display
		SDL_SetWindowPosition( sdl.window, x, x );
	}
	return displayIdx;
}

/*
===================
idDeviceManagerSDL::SetScreenParmsFullscreen
===================
*/
bool idDeviceManagerSDL::SetScreenParmsFullscreen( vidParms_t parms )
{
	SDL_DisplayMode m = {0};
	int displayIdx = ScreenParmsHandleDisplayIndex( parms );
	if( displayIdx < 0 )
	{
		return false;
	}

	// get current mode of display the window should be full-screened on
	SDL_GetCurrentDisplayMode( displayIdx, &m );

	// change settings in that display mode according to parms
	// FIXME: check if refreshrate, width and height are supported?
	// m.refresh_rate = parms.displayHz;
	m.w = parms.width;
	m.h = parms.height;

	// set that displaymode
	if( SDL_SetWindowDisplayMode( sdl.window, &m ) < 0 )
	{
		common->Warning( "Couldn't set window mode for fullscreen, reason: %s", SDL_GetError() );
		return false;
	}

	// if we're currently not in fullscreen mode, we need to switch to fullscreen
	if( !( SDL_GetWindowFlags( sdl.window ) & SDL_WINDOW_FULLSCREEN ) )
	{
		if( SDL_SetWindowFullscreen( sdl.window, SDL_TRUE ) < 0 )
		{
			common->Warning( "Couldn't switch to fullscreen mode, reason: %s!", SDL_GetError() );
			return false;
		}
	}
	return true;
}

/*
===================
idDeviceManagerSDL::SetScreenParmsWindowed
===================
*/
bool idDeviceManagerSDL::SetScreenParmsWindowed( vidParms_t parms )
{
	SDL_SetWindowSize( sdl.window, parms.width, parms.height );
	SDL_SetWindowPosition( sdl.window, parms.x, parms.y );

	// if we're currently in fullscreen mode, we need to disable that
	if( SDL_GetWindowFlags( sdl.window ) & SDL_WINDOW_FULLSCREEN )
	{
		if( SDL_SetWindowFullscreen( sdl.window, SDL_FALSE ) < 0 )
		{
			common->Warning( "Couldn't switch to windowed mode, reason: %s!", SDL_GetError() );
			return false;
		}
	}
	return true;
}

/*
===================
idDeviceManagerSDL::SetScreenParms
===================
*/
bool idDeviceManagerSDL::SetScreenParms( vidParms_t parms )
{
	if( parms.fullScreen == -1 )
	{
		// Switch to borderless window at given position/size
		SDL_SetWindowBordered( sdl.window, SDL_FALSE );
		SDL_SetWindowPosition( sdl.window, parms.x, parms.y );
		SDL_SetWindowSize( sdl.window, parms.width, parms.height );

		// If currently fullscreen, exit fullscreen mode first
		if( SDL_GetWindowFlags( sdl.window ) & SDL_WINDOW_FULLSCREEN )
		{
			SDL_SetWindowFullscreen( sdl.window, SDL_FALSE );
		}
	}
	else if( parms.fullScreen > 0 || parms.fullScreen == -2 )
	{
		if( !SetScreenParmsFullscreen( parms ) )
		{
			return false;
		}
	}
	else if( parms.fullScreen == 0 )
	{
		if( !SetScreenParmsWindowed( parms ) )
		{
			return false;
		}
	}
	else
	{
		common->Warning( "idDeviceManagerSDL::SetScreenParms: unsupported fullScreen value %d", parms.fullScreen );
		return false;
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.displayFrequency = parms.displayHz;
	glConfig.multisamples = parms.multiSamples;

#if !defined(USE_VULKAN)
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples );
#endif

	return true;
}

/*
===================
idDeviceManagerSDL::Shutdown
===================
*/
void idDeviceManagerSDL::Shutdown( bool shutdownSDL )
{
	common->Printf( "Shutting down OpenGL subsystem\n" );

	RestoreGamma();

	if( SDL_GetWindowFlags( sdl.window ) & SDL_WINDOW_FULLSCREEN )
	{
		SDL_SetWindowFullscreen( sdl.window, SDL_FALSE );
		SDL_Delay( 100 );
	}

#if !defined(USE_VULKAN)
	if( sdl.context )
	{
		SDL_GL_DeleteContext( sdl.context );
		sdl.context = NULL;
	}
#endif

	if( sdl.window )
	{
		SDL_DestroyWindow( sdl.window );
		sdl.window = NULL;
	}

#if defined(USE_VULKAN)
	if( shutdownSDL && SDL_WasInit( 0 ) )
	{
		SDL_Quit();
	}
#endif
}

/*
===================
idDeviceManagerSDL::SwapBuffers
===================
*/
void idDeviceManagerSDL::SwapBuffers()
{
#if !defined(USE_VULKAN)
	if( r_swapInterval.IsModified() )
	{
		r_swapInterval.ClearModified();

		int interval = r_swapInterval.GetInteger();
		if( interval == 1 && glConfig.swapControlTearAvailable )
		{
			interval = -1;
		}
		if( SDL_GL_SetSwapInterval( interval ) < 0 )
		{
			common->Warning( "Failed to set swap interval %d: %s", interval, SDL_GetError() );
		}
	}

	SDL_GL_SwapWindow( sdl.window );
#endif
}

/*
===================
idDeviceManagerSDL::GrabInput
===================
*/
void idDeviceManagerSDL::GrabInput( int flags )
{
	bool grab = flags & GRAB_ENABLE;

	if( grab && ( flags & GRAB_REENABLE ) )
	{
		grab = false;
	}

	if( flags & GRAB_SETSTATE )
	{
		sdl.grabbed = grab;
	}

	if( SDLVars_t::in_nograb.GetBool() )
	{
		grab = false;
	}

	if( !sdl.window )
	{
		common->Warning( "GLimp_GrabInput called without window" );
		return;
	}

	// DG: disabling the cursor is now done once in GLimp_Init() because it should always be disabled

	// DG: check for GRAB_ENABLE instead of GRAB_HIDECURSOR because we always wanna hide it
	SDL_SetRelativeMouseMode( flags & GRAB_ENABLE ? SDL_TRUE : SDL_FALSE );
	SDL_SetWindowGrab( sdl.window, grab ? SDL_TRUE : SDL_FALSE );
}

/*
========================
idDeviceManagerSDL::SetWindowsIcon
========================
*/
void idDeviceManagerSDL::SetWindowsIcon( void* window )
{
	Uint32 rmask, gmask, bmask, amask;

	// ok, the following is pretty stupid.. SDL_CreateRGBSurfaceFrom() pretends to use a void* for the data,
	// but it's really treated as endian-specific Uint32* ...
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

#include "doom_ico.h" // contains the struct doom_icon

	SDL_Surface* icon = SDL_CreateRGBSurfaceFrom( ( void* )doom_icon.pixel_data, doom_icon.width, doom_icon.height,
						doom_icon.bytes_per_pixel * 8, doom_icon.bytes_per_pixel * doom_icon.width,
						rmask, gmask, bmask, amask );

	SDL_SetWindowIcon( ( SDL_Window* )window, icon );

	SDL_FreeSurface( icon );
}

#if defined(USE_VULKAN)
// Eric: Integrate this into RBDoom3BFG's source code ecosystem.
// Helper function for using SDL2 and Vulkan on Linux.

/*
========================
idDeviceManagerSDL::GetRequiredExtensions
========================
*/
std::vector<const char*> idDeviceManagerSDL::GetRequiredExtensions()
{
	uint32_t sdlCount = 0;
	std::vector<const char*> sdlInstanceExtensions = {};

	SDL_Vulkan_GetInstanceExtensions( nullptr, &sdlCount, nullptr );
	sdlInstanceExtensions.resize( sdlCount );
	SDL_Vulkan_GetInstanceExtensions( nullptr, &sdlCount, sdlInstanceExtensions.data() );

	// SRS - needed for MoltenVK portability implementation and optionally for MoltenVK configuration on OSX
#if defined(__APPLE__)
	sdlInstanceExtensions.push_back( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );
#if defined(USE_MoltenVK)
	sdlInstanceExtensions.push_back( VK_MVK_MOLTENVK_EXTENSION_NAME );
#endif
#endif

	return sdlInstanceExtensions;
}
#endif