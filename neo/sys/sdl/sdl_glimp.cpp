/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/

#include <SDL.h>
#include <SDL_syswm.h>

#include "precompiled.h"
#pragma hdrstop

#include "../renderer/tr_local.h"
#include "sdl_local.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

idCVar in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing" );

// RB: FIXME this shit. We need the OpenGL alpha channel for advanced rendering effects
idCVar r_waylandcompat( "r_waylandcompat", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "wayland compatible framebuffer" );

static bool grabbed = false;

#if SDL_VERSION_ATLEAST(2, 0, 0)
	static SDL_Window* window = NULL;
	static SDL_GLContext context = NULL;
#else
	static SDL_Surface* window = NULL;
	#define SDL_WINDOW_OPENGL SDL_OPENGL
	#define SDL_WINDOW_FULLSCREEN SDL_FULLSCREEN
	#define SDL_WINDOW_RESIZABLE SDL_RESIZABLE
#endif

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
	if( !SDL_WasInit( SDL_INIT_VIDEO | SDL_INIT_TIMER ) )
	{
		if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER ) )
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
	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
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

		SDL_GL_SetAttribute( SDL_GL_STEREO, parms.stereo ? 1 : 0 );

		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples );

#if SDL_VERSION_ATLEAST(2, 0, 0)

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

		// RB begin
		SDL_GetWindowSize( window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );
		// RB end

		glConfig.isFullscreen = ( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) == SDL_WINDOW_FULLSCREEN;
#else
		// no SDL 2 -> no OpenGL 3 context
		r_useOpenGL32.SetInteger( 0 );
		glConfig.driverType = GLDRV_OPENGL3X;

		SDL_WM_SetCaption( GAME_NAME, GAME_NAME );

		if( SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, r_swapInterval.GetInteger() ) < 0 )
		{
			common->Warning( "SDL_GL_SWAP_CONTROL not supported" );
		}

		window = SDL_SetVideoMode( parms.width, parms.height, colorbits, flags );
		if( !window )
		{
			common->DPrintf( "Couldn't set GL mode %d/%d/%d: %s",
							 channelcolorbits, tdepthbits, tstencilbits, SDL_GetError() );
			continue;
		}

		glConfig.nativeScreenWidth = window->w;
		glConfig.nativeScreenHeight = window->h;

		glConfig.isFullscreen = ( window->flags & SDL_FULLSCREEN ) == SDL_FULLSCREEN;
#endif

		common->Printf( "Using %d color bits, %d depth, %d stencil display\n",
						channelcolorbits, tdepthbits, tstencilbits );

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;

		// RB begin
		glConfig.displayFrequency = 60;
		glConfig.isStereoPixelFormat = parms.stereo;
		glConfig.multisamples = parms.multiSamples;

		glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted
		// should side-by-side stereo modes be consider aspect 0.5?

		// RB end

		break;
	}

	if( !window )
	{
		common->Printf( "No usable GL mode found: %s", SDL_GetError() );
		return false;
	}

	// RB: use glewExperimental to avoid issues with OpenGL 3.x core profiles
#if SDL_VERSION_ATLEAST(2, 0, 0)
	if( r_useOpenGL32.GetInteger() > 1 )
	{
		glewExperimental = GL_TRUE;
	}
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

#if SDL_VERSION_ATLEAST(2, 0, 0)
// SDL1 doesn't support multiple displays, so the source is much shorter and doesn't need separate functions
// makes sure the window will be full-screened on the right display and returns the SDL display index
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
#endif // SDL_VERSION_ATLEAST(2, 0, 0)

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms( glimpParms_t parms )
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
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
#else // SDL 1.2 - so much shorter, but doesn't handle multiple displays
	SDL_Surface* s = SDL_GetVideoSurface();
	if( s == NULL )
	{
		common->Warning( "GLimp_SetScreenParms: Couldn't get video information, reason: %s", SDL_GetError() );
		return false;
	}


	int bitsperpixel = 24;
	if( s->format )
	{
		bitsperpixel = s->format->BitsPerPixel;
	}

	Uint32 flags = s->flags;

	if( parms.fullScreen )
	{
		flags |= SDL_FULLSCREEN;
	}
	else
	{
		flags &= ~SDL_FULLSCREEN;
	}

	s = SDL_SetVideoMode( parms.width, parms.height, bitsperpixel, flags );
	if( s == NULL )
	{
		common->Warning( "GLimp_SetScreenParms: Couldn't set video information, reason: %s", SDL_GetError() );
		return false;
	}
#endif // SDL_VERSION_ATLEAST(2, 0, 0)

	// Note: the following stuff would also work with SDL1.2
	SDL_GL_SetAttribute( SDL_GL_STEREO, parms.stereo ? 1 : 0 );

	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples );

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.isStereoPixelFormat = parms.stereo;
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

#if SDL_VERSION_ATLEAST(2, 0, 0)
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
#endif
}

/*
===================
GLimp_SwapBuffers
===================
*/
void GLimp_SwapBuffers()
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_GL_SwapWindow( window );
#else
	SDL_GL_SwapBuffers();
#endif
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

#if SDL_VERSION_ATLEAST(2, 0, 0)
	if( SDL_SetWindowGammaRamp( window, red, green, blue ) )
#else
	if( SDL_SetGammaRamp( red, green, blue ) )
#endif
		common->Warning( "Couldn't set gamma ramp: %s", SDL_GetError() );
}

/*
===================
GLimp_ExtensionPointer
===================
*/
/*
GLExtension_t GLimp_ExtensionPointer(const char *name) {
	assert(SDL_WasInit(SDL_INIT_VIDEO));

	return (GLExtension_t)SDL_GL_GetProcAddress(name);
}
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

#if SDL_VERSION_ATLEAST(2, 0, 0)
	// DG: disabling the cursor is now done once in GLimp_Init() because it should always be disabled

	// DG: check for GRAB_ENABLE instead of GRAB_HIDECURSOR because we always wanna hide it
	SDL_SetRelativeMouseMode( flags & GRAB_ENABLE ? SDL_TRUE : SDL_FALSE );
	SDL_SetWindowGrab( window, grab ? SDL_TRUE : SDL_FALSE );
#else
	// DG end
	SDL_WM_GrabInput( grab ? SDL_GRAB_ON : SDL_GRAB_OFF );
#endif
}

/*
====================
DumpAllDisplayDevices
====================
*/
void DumpAllDisplayDevices()
{
	common->DPrintf( "TODO: DumpAllDisplayDevices\n" );
}



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

// RB: resolutions supported by XreaL
static void FillStaticVidModes( idList<vidMode_t>& modeList )
{
	modeList.AddUnique( vidMode_t( 320,   240, 60 ) );
	modeList.AddUnique( vidMode_t( 400,   300, 60 ) );
	modeList.AddUnique( vidMode_t( 512,   384, 60 ) );
	modeList.AddUnique( vidMode_t( 640,   480, 60 ) );
	modeList.AddUnique( vidMode_t( 800,   600, 60 ) );
	modeList.AddUnique( vidMode_t( 960,   720, 60 ) );
	modeList.AddUnique( vidMode_t( 1024,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1152,  864, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  720, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  800, 60 ) );
	modeList.AddUnique( vidMode_t( 1280, 1024, 60 ) );
	modeList.AddUnique( vidMode_t( 1360,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1440,  900, 60 ) );
	modeList.AddUnique( vidMode_t( 1680, 1050, 60 ) );
	modeList.AddUnique( vidMode_t( 1600, 1200, 60 ) );
	modeList.AddUnique( vidMode_t( 1920, 1080, 60 ) );
	modeList.AddUnique( vidMode_t( 1920, 1200, 60 ) );
	modeList.AddUnique( vidMode_t( 2048, 1536, 60 ) );
	modeList.AddUnique( vidMode_t( 2560, 1600, 60 ) );

	modeList.SortWithTemplate( idSort_VidMode() );
}

/*
====================
R_GetModeListForDisplay
====================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList )
{
	assert( requestedDisplayNum >= 0 );

	modeList.Clear();
#if SDL_VERSION_ATLEAST(2, 0, 0)
	// DG: SDL2 implementation
	if( requestedDisplayNum >= SDL_GetNumVideoDisplays() )
	{
		// requested invalid displaynum
		return false;
	}

	int numModes = SDL_GetNumDisplayModes( requestedDisplayNum );
	if( numModes > 0 )
	{
		for( int i = 0; i < numModes; i++ )
		{
			SDL_DisplayMode m;
			int ret = SDL_GetDisplayMode( requestedDisplayNum, i, &m );
			if( ret != 0 )
			{
				common->Warning( "Can't get video mode no %i, because of %s\n", i, SDL_GetError() );
				continue;
			}

			vidMode_t mode;
			mode.width = m.w;
			mode.height = m.h;
			mode.displayHz = m.refresh_rate ? m.refresh_rate : 60; // default to 60 if unknown (0)
			modeList.AddUnique( mode );
		}

		if( modeList.Num() < 1 )
		{
			common->Warning( "Couldn't get a single video mode for display %i, using default ones..!\n", requestedDisplayNum );
			FillStaticVidModes( modeList );
		}

		// sort with lowest resolution first
		modeList.SortWithTemplate( idSort_VidMode() );
	}
	else
	{
		common->Warning( "Can't get Video Info, using default modes...\n" );
		if( numModes < 0 )
		{
			common->Warning( "Reason was: %s\n", SDL_GetError() );
		}
		FillStaticVidModes( modeList );
	}

	return true;
	// DG end

#else // SDL 1

	// DG: SDL1 only knows of one display - some functions rely on
	// R_GetModeListForDisplay() returning false for invalid displaynum to iterate all displays
	if( requestedDisplayNum >= 1 )
	{
		return false;
	}
	// DG end

	const SDL_VideoInfo* videoInfo = SDL_GetVideoInfo();
	if( videoInfo == NULL )
	{
		// DG: yes, this can actually fail, e.g. if SDL_Init( SDL_INIT_VIDEO ) wasn't called
		common->Warning( "Can't get Video Info, using default modes...\n" );
		FillStaticVidModes( modeList );
		return true;
	}

	SDL_Rect** modes = SDL_ListModes( videoInfo->vfmt, SDL_OPENGL | SDL_FULLSCREEN );

	if( !modes )
	{
		common->Warning( "Can't get list of available modes, using default ones...\n" );
		FillStaticVidModes( modeList );
		return true;
	}

	if( modes == ( SDL_Rect** ) - 1 )
	{
		common->Printf( "Display supports any resolution\n" );
		FillStaticVidModes( modeList );
		return true;
	}

	int numModes;
	for( numModes = 0; modes[numModes]; numModes++ );

	if( numModes > 1 )
	{
		for( int i = 0; i < numModes; i++ )
		{
			vidMode_t mode;
			mode.width =  modes[i]->w;
			mode.height =  modes[i]->h;
			mode.displayHz = 60; // FIXME;
			modeList.AddUnique( mode );
		}

		// sort with lowest resolution first
		modeList.SortWithTemplate( idSort_VidMode() );

		return true;
	}

	return false;
#endif
}
