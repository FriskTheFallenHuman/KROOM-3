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

#ifdef _WIN32
	#include "glew/include/GL/glew.h"
#else
	#include <GL/glew.h>
#endif

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL2/SDL.h>

#include "renderer/RenderCommon.h"
#include "sdl_local.h"

// RB: FIXME this shit. We need the OpenGL alpha channel for advanced rendering effects
idCVar r_waylandcompat( "r_waylandcompat", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "wayland compatible framebuffer" );

// TODO: i really need to make a struct similar to win32 one
SDL_Window* window = NULL;
static SDL_GLContext context = NULL;
static unsigned short oldHardwareGamma[3][256];
static bool gammaSaved = false;

/*
========================
GLimp_TestSwapBuffers
========================
*/
static void GLimp_TestSwapBuffers( const idCmdArgs& args )
{
	if( !window )
	{
		common->Printf( "No window available.\n" );
		return;
	}

	common->Printf( "GLimp_TestSwapBuffers\n" );
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
			SDL_GL_SwapWindow( window );
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
}

/*
========================
GLimp_SaveGamma
========================
*/
static void GLimp_SaveGamma()
{
	if( SDL_GetWindowGammaRamp( window, oldHardwareGamma[0], oldHardwareGamma[1], oldHardwareGamma[2] ) == 0 )
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
GLimp_RestoreGamma
========================
*/
static void GLimp_RestoreGamma()
{
	if( gammaSaved )
	{
		if( SDL_SetWindowGammaRamp( window, oldHardwareGamma[0], oldHardwareGamma[1], oldHardwareGamma[2] ) != 0 )
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
GLimp_SetGamma

The renderer calls this when the user adjusts r_gamma or r_brightness
========================
*/
void GLimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
{
	if( !window )
	{
		common->Warning( "GLimp_SetGamma called without window" );
		return;
	}

	if( SDL_SetWindowGammaRamp( window, red, green, blue ) )
	{
		common->Warning( "Couldn't set gamma ramp: %s", SDL_GetError() );
	}
}

/*
===================
GLimp_PreInit

 R_GetModeListForDisplay is called before GLimp_Init(), but SDL needs SDL_Init() first.
 So do that in GLimp_PreInit()
 Calling that function more than once doesn't make a difference
===================
*/
void GLimp_PreInit() // DG: added this function for SDL compatibility
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
GLimp_Init
===================
*/
bool GLimp_Init( glimpParms_t parms )
{
	cmdSystem->AddCommand( "testSwapBuffers", GLimp_TestSwapBuffers, CMD_FL_SYSTEM, "Times swapbuffer options" );

	common->Printf( "Initializing OpenGL subsystem with multisamples:%i fullscreen:%i\n",
					parms.multiSamples, parms.fullScreen );

	GLimp_PreInit(); // DG: make sure SDL is initialized

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

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );

		if( r_waylandcompat.GetBool() )
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

		// DG: set display num for fullscreen
		Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

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

		window = SDL_CreateWindow( title.c_str(),
								   windowPosX, windowPosY,
								   parms.width, parms.height, flags );
		// DG end

		context = SDL_GL_CreateContext( window );

		if( !window )
		{
			common->DPrintf( "Couldn't set GL mode %d/%d/%d: %s",
							 channelcolorbits, tdepthbits, tstencilbits, SDL_GetError() );
			continue;
		}

		if( SDL_GL_SetSwapInterval( r_swapInterval.GetInteger() ) < 0 )
		{
			common->Warning( "SDL_GL_SetSwapInterval(%d) failed: %s", r_swapInterval.GetInteger(), SDL_GetError() );
		}

		Sys_SDLIcon( window );

		// RB begin
		SDL_GetWindowSize( window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );
		// RB end

		if( parms.fullScreen == -1 )
		{
			glConfig.isFullscreen = -1;
		}
		else
		{
			glConfig.isFullscreen = ( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) ? parms.fullScreen : 0;
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

	if( !window )
	{
		common->Printf( "No usable GL mode found: %s", SDL_GetError() );
		return false;
	}

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

#if defined(__APPLE__)
	// SRS - On OSX enable SDL2 relative mouse mode warping to capture mouse properly if outside of window
	SDL_SetHintWithPriority( SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE );
#endif

	// DG: disable cursor, we have two cursors in menu (because mouse isn't grabbed in menu)
	SDL_ShowCursor( SDL_DISABLE );
	// DG end

	GLimp_SaveGamma();

	return true;
}

/*
===================
GLimp_Shutdown
===================
*/
void GLimp_Shutdown()
{
	common->Printf( "Shutting down OpenGL subsystem\n" );

	GLimp_RestoreGamma();

	if( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN )
	{
		SDL_SetWindowFullscreen( window, SDL_FALSE );
		SDL_Delay( 100 );
	}

	if( context )
	{
		SDL_GL_DeleteContext( context );
		context = NULL;
	}

	if( window )
	{
		SDL_DestroyWindow( window );
		window = NULL;
	}
}

/*
===================
GLimp_SwapBuffers
===================
*/
void GLimp_SwapBuffers()
{
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

	SDL_GL_SwapWindow( window );
}