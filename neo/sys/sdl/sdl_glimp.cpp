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

#include <SDL.h>

#include "renderer/RenderCommon.h"
#include "sdl_local.h"

idCVar in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing" );

// RB: FIXME this shit. We need the OpenGL alpha channel for advanced rendering effects
idCVar r_waylandcompat( "r_waylandcompat", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "wayland compatible framebuffer" );

// RB: only relevant if using SDL 2.0
#if defined(__APPLE__)
	// only core profile is supported on OS X
	idCVar r_useOpenGL32( "r_useOpenGL32", "2", CVAR_INTEGER, "0 = OpenGL 3.x, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );
#elif defined(__linux__)
	// Linux open source drivers suck
	idCVar r_useOpenGL32( "r_useOpenGL32", "0", CVAR_INTEGER, "0 = OpenGL 3.x, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );
#else
	idCVar r_useOpenGL32( "r_useOpenGL32", "1", CVAR_INTEGER, "0 = OpenGL 3.x, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );
#endif
// RB end

static bool grabbed = false;
static SDL_Window* window = NULL;
static SDL_GLContext context = NULL;

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

	common->Printf( "Initializing OpenGL subsystem\n" );

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

		// RB begin
		if( r_useOpenGL32.GetInteger() > 0 )
		{
			glConfig.driverType = GLDRV_OPENGL32_COMPATIBILITY_PROFILE;

			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );

			if( r_debugContext.GetBool() )
			{
				SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
			}
		}

		if( r_useOpenGL32.GetInteger() > 1 )
		{
			glConfig.driverType = GLDRV_OPENGL32_CORE_PROFILE;

			SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
		}
		// RB end

		// DG: set display num for fullscreen
		int windowPos = SDL_WINDOWPOS_UNDEFINED;
		if( parms.fullScreen > 0 )
		{
			if( parms.fullScreen > SDL_GetNumVideoDisplays() )
			{
				common->Warning( "Couldn't set display to num %i because we only have %i displays",
								 parms.fullScreen, SDL_GetNumVideoDisplays() );
			}
			else
			{
				// -1 because SDL starts counting displays at 0, while parms.fullScreen starts at 1
				windowPos = SDL_WINDOWPOS_UNDEFINED_DISPLAY( ( parms.fullScreen - 1 ) );
			}
		}
		// TODO: if parms.fullScreen == -1 there should be a borderless window spanning multiple displays
		/*
		 * NOTE that this implicitly handles parms.fullScreen == -2 (from r_fullscreen -2) meaning
		 * "do fullscreen, but I don't care on what monitor", at least on my box it's the monitor with
		 * the mouse cursor.
		 */


		window = SDL_CreateWindow( GAME_NAME,
								   windowPos,
								   windowPos,
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
			common->Warning( "SDL_GL_SWAP_CONTROL not supported" );
		}

		Sys_SDLIcon( window );

		// RB begin
		SDL_GetWindowSize( window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );
		// RB end

		glConfig.isFullscreen = ( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) == SDL_WINDOW_FULLSCREEN;

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

#if defined(__APPLE__)
	// SRS - On OSX enable SDL2 relative mouse mode warping to capture mouse properly if outside of window
	SDL_SetHintWithPriority( SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE );
#endif

	// DG: disable cursor, we have two cursors in menu (because mouse isn't grabbed in menu)
	SDL_ShowCursor( SDL_DISABLE );
	// DG end

	return true;
}
/*
===================
 Helper functions for GLimp_SetScreenParms()
===================
*/
static int ScreenParmsHandleDisplayIndex( glimpParms_t parms )
{
	int displayIdx;
	if( parms.fullScreen > 0 )
	{
		displayIdx = parms.fullScreen - 1; // first display for SDL is 0, in parms it's 1
	}
	else // -2 == use current display
	{
		displayIdx = SDL_GetWindowDisplayIndex( window );
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
			SDL_SetWindowFullscreen( window, SDL_FALSE );
		}
		// select display ; SDL_WINDOWPOS_UNDEFINED_DISPLAY() doesn't work.
		int x = SDL_WINDOWPOS_CENTERED_DISPLAY( displayIdx );
		// move window to the center of selected display
		SDL_SetWindowPosition( window, x, x );
	}
	return displayIdx;
}

static bool SetScreenParmsFullscreen( glimpParms_t parms )
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
	if( SDL_SetWindowDisplayMode( window, &m ) < 0 )
	{
		common->Warning( "Couldn't set window mode for fullscreen, reason: %s", SDL_GetError() );
		return false;
	}

	// if we're currently not in fullscreen mode, we need to switch to fullscreen
	if( !( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) )
	{
		if( SDL_SetWindowFullscreen( window, SDL_TRUE ) < 0 )
		{
			common->Warning( "Couldn't switch to fullscreen mode, reason: %s!", SDL_GetError() );
			return false;
		}
	}
	return true;
}

static bool SetScreenParmsWindowed( glimpParms_t parms )
{
	SDL_SetWindowSize( window, parms.width, parms.height );
	SDL_SetWindowPosition( window, parms.x, parms.y );

	// if we're currently in fullscreen mode, we need to disable that
	if( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN )
	{
		if( SDL_SetWindowFullscreen( window, SDL_FALSE ) < 0 )
		{
			common->Warning( "Couldn't switch to windowed mode, reason: %s!", SDL_GetError() );
			return false;
		}
	}
	return true;
}

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms( glimpParms_t parms )
{
	if( parms.fullScreen > 0 || parms.fullScreen == -2 )
	{
		if( !SetScreenParmsFullscreen( parms ) )
		{
			return false;
		}
	}
	else if( parms.fullScreen == 0 ) // windowed mode
	{
		if( !SetScreenParmsWindowed( parms ) )
		{
			return false;
		}
	}
	else
	{
		common->Warning( "GLimp_SetScreenParms: fullScreen -1 (borderless window for multiple displays) currently unsupported!" );
		return false;
	}

	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples );

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.displayFrequency = parms.displayHz;
	glConfig.multisamples = parms.multiSamples;

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
	SDL_GL_SwapWindow( window );
}

/*
=================
GLimp_SetGamma
=================
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
GLimp_GrabInput
===================
*/
void GLimp_GrabInput( int flags )
{
	bool grab = flags & GRAB_ENABLE;

	if( grab && ( flags & GRAB_REENABLE ) )
	{
		grab = false;
	}

	if( flags & GRAB_SETSTATE )
	{
		grabbed = grab;
	}

	if( in_nograb.GetBool() )
	{
		grab = false;
	}

	if( !window )
	{
		common->Warning( "GLimp_GrabInput called without window" );
		return;
	}

	// DG: disabling the cursor is now done once in GLimp_Init() because it should always be disabled

	// DG: check for GRAB_ENABLE instead of GRAB_HIDECURSOR because we always wanna hide it
	SDL_SetRelativeMouseMode( flags & GRAB_ENABLE ? SDL_TRUE : SDL_FALSE );
	SDL_SetWindowGrab( window, grab ? SDL_TRUE : SDL_FALSE );
}