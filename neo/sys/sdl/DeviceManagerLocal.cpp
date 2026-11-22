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

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL2/SDL.h>
#include <SDL_image.h>
#ifdef _WIN32
	#include <SDL2/SDL_syswm.h>
#endif

#include "../renderer/RenderCommon.h"
#include "DeviceManagerLocal.h"
#include "DeviceManagerAPI.h"

extern idCVar r_windowWidth;
extern idCVar r_windowHeight;

static struct deviceWindowState_t
{
	SDL_Window*		window = NULL;
	bool			grabbed = false;
} sdl;

idDeviceManagerLocal deviceManagerLocal;
idDeviceManager* deviceManager = &deviceManagerLocal;


/*
========================
idDeviceManagerLocal::TestSwapBuffers
========================
*/
void idDeviceManagerLocal::TestSwapBuffers( const idCmdArgs& args )
{
	if( !sdl.window )
	{
		common->Printf( "No window available.\n" );
		return;
	}

	DM_TestSwapBuffers( sdl.window );
}

/*
========================
idDeviceManagerLocal::SaveGamma
========================
*/
void idDeviceManagerLocal::SaveGamma()
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
idDeviceManagerLocal::RestoreGamma
========================
*/
void idDeviceManagerLocal::RestoreGamma()
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
idDeviceManagerLocal::SetGamma

The renderer calls this when the user adjusts r_gamma or r_brightness
========================
*/
void idDeviceManagerLocal::SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
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
idDeviceManagerLocal::PreInit

 GetModeListForDisplay is called before Init(), but SDL needs SDL_Init() first.
 So do that in PreInit()
 Calling that function more than once doesn't make a difference
===================
*/
void idDeviceManagerLocal::PreInit()
{
}


/*
===================
idDeviceManagerLocal::Init
===================
*/
bool idDeviceManagerLocal::Init( vidParms_t parms )
{
	cmdSystem->AddCommand( "testSwapBuffers", idDeviceManagerLocal::TestSwapBuffers, CMD_FL_SYSTEM, "Times swapbuffer options" );

	common->Printf( "Initializing render subsystem with multisamples:%i fullscreen:%i\n",
					parms.multiSamples, parms.fullScreen );

	PreInit(); // DG: make sure SDL is initialized

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

		driverFlag = DM_PrepareWindow( parms, channelcolorbits, tdepthbits, tstencilbits );

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
			windowPosX = ( parms.x != -1 ) ? parms.x : SDL_WINDOWPOS_CENTERED;
			windowPosY = ( parms.y != -1 ) ? parms.y : SDL_WINDOWPOS_CENTERED;
		}

		// Show whether this is a 32-bit or 64-bit binary
		idStr title = va( "%s - %s %s (x%u Build)", GAME_NAME, ID__DATE__, ID__TIME__, ( unsigned int )( sizeof( void* ) * 8 ) );

		sdl.window = SDL_CreateWindow( title.c_str(),
									   windowPosX, windowPosY,
									   parms.width, parms.height, flags );
		// DG end

#if defined(_WIN32)
		// HACK HACK: horrible hack, this forces the windows to be present when relaunching
		SDL_SysWMinfo wmInfo = {};
		SDL_VERSION( &wmInfo.version );
		if( SDL_GetWindowWMInfo( sdl.window, &wmInfo ) )
		{
			HWND hwnd = wmInfo.info.win.window;
			LONG exstyle  = GetWindowLong( hwnd, GWL_EXSTYLE );

			SetWindowLongPtr( hwnd, GWLP_HWNDPARENT, ( LONG_PTR )0 );

			exstyle &= ~WS_EX_TOOLWINDOW;
			exstyle |=  WS_EX_APPWINDOW;
			SetWindowLong( hwnd, GWL_EXSTYLE, exstyle );

			if( IsIconic( hwnd ) )
			{
				ShowWindow( hwnd, SW_RESTORE );
			}
			else
			{
				ShowWindow( hwnd, SW_SHOW );
			}

			SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_FRAMECHANGED );
			SetForegroundWindow( hwnd );
		}
#else
		SDL_ShowWindow( sdl.window );
		SDL_RaiseWindow( sdl.window );
#endif

		if( !sdl.window )
		{
			common->DPrintf( "Couldn't set render mode %d/%d/%d: %s",
							 channelcolorbits, tdepthbits, tstencilbits, SDL_GetError() );
			continue;
		}

		if( !DM_CreateContext( sdl.window ) )
		{
			SDL_DestroyWindow( sdl.window );
			sdl.window = NULL;
			continue;
		}

		SetWindowsIcon( sdl.window );

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

	DM_FinishInit( sdl.window );

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
idDeviceManagerLocal::DumpAllDisplayDevices
====================
*/
void idDeviceManagerLocal::DumpAllDisplayDevices()
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
GetNumVideoDisplays
====================
*/
int idDeviceManagerLocal::GetNumVideoDisplays()
{
	return SDL_GetNumVideoDisplays();
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
idDeviceManagerLocal::GetModeListForDisplay
====================
*/
bool idDeviceManagerLocal::GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList, const int minHeight )
{
	assert( requestedDisplayNum >= 0 );
	modeList.Clear();

	if( requestedDisplayNum >= SDL_GetNumVideoDisplays() )
	{
		common->Warning( "idDeviceManagerLocal::GetModeListForDisplay: invalid display index %d (only %d displays)", requestedDisplayNum, SDL_GetNumVideoDisplays() );
		return false;
	}

	int numModes = SDL_GetNumDisplayModes( requestedDisplayNum );
	if( numModes <= 0 )
	{
		common->Warning( "idDeviceManagerLocal::GetModeListForDisplay: no modes for display %d", requestedDisplayNum );
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
		common->Warning( "idDeviceManagerLocal::GetModeListForDisplay: no modes passed filtering (minHeight=%d)", minHeight );
		return false;
	}

	modeList.SortWithTemplate( idSort_VidMode() );
	return true;
}

/*
====================
idDeviceManagerLocal::GetDefaultDisplayMode
====================
*/
bool idDeviceManagerLocal::GetDefaultDisplayMode( int& defaultDisplayNum, vidMode_t& defaultMode )
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
====================
idDeviceManagerLocal::GetRefreshRatesForDisplay

Returns all distinct refresh rates available for a given resolution on
the given display.
====================
*/
bool idDeviceManagerLocal::GetRefreshRatesForDisplay( const int displayNum, const int width, const int height, idList<int>& rateList )
{
	rateList.Clear();

	if( displayNum < 0 || displayNum >= SDL_GetNumVideoDisplays() )
	{
		return false;
	}

	if( width <= 0 || height <= 0 )
	{
		return false;
	}

	const int numModes = SDL_GetNumDisplayModes( displayNum );
	for( int i = 0; i < numModes; i++ )
	{
		SDL_DisplayMode mode = {};
		if( SDL_GetDisplayMode( displayNum, i, &mode ) != 0 )
		{
			continue;
		}
		if( mode.w != width || mode.h != height )
		{
			continue;
		}

		const int rate = ( mode.refresh_rate > 0 ) ? mode.refresh_rate : 60;
		rateList.AddUnique( rate );
	}

	for( int i = 1; i < rateList.Num(); i++ )
	{
		const int key = rateList[i];
		int j = i - 1;
		while( j >= 0 && rateList[j] > key )
		{
			rateList[j + 1] = rateList[j];
			j--;
		}
		rateList[j + 1] = key;
	}

	return rateList.Num() > 0;
}

/*
===================
idDeviceManagerLocal::ScreenParmsHandleDisplayIndex

Helper functions for SetScreenParms()
===================
*/
int idDeviceManagerLocal::ScreenParmsHandleDisplayIndex( vidParms_t parms )
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
idDeviceManagerLocal::SetScreenParmsFullscreen
===================
*/
bool idDeviceManagerLocal::SetScreenParmsFullscreen( vidParms_t parms )
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
idDeviceManagerLocal::SetScreenParmsWindowed
===================
*/
bool idDeviceManagerLocal::SetScreenParmsWindowed( vidParms_t parms )
{
	// if we're currently in fullscreen mode, we need to disable that
	if( SDL_GetWindowFlags( sdl.window ) & SDL_WINDOW_FULLSCREEN )
	{
		if( SDL_SetWindowFullscreen( sdl.window, SDL_FALSE ) < 0 )
		{
			common->Warning( "Couldn't switch to windowed mode, reason: %s!", SDL_GetError() );
			return false;
		}
	}

	SDL_SetWindowBordered( sdl.window, SDL_TRUE );
	SDL_SetWindowSize( sdl.window, parms.width, parms.height );
	SDL_SetWindowPosition( sdl.window, parms.x, parms.y );

	return true;
}

/*
===================
idDeviceManagerLocal::SetScreenParms
===================
*/
bool idDeviceManagerLocal::SetScreenParms( vidParms_t parms )
{
	if( parms.fullScreen == -1 )
	{
		// If currently fullscreen, exit fullscreen mode first
		if( SDL_GetWindowFlags( sdl.window ) & SDL_WINDOW_FULLSCREEN )
		{
			SDL_SetWindowFullscreen( sdl.window, SDL_FALSE );
		}

		// Switch to borderless window at given position/size
		SDL_SetWindowBordered( sdl.window, SDL_FALSE );
		SDL_SetWindowPosition( sdl.window, parms.x, parms.y );
		SDL_SetWindowSize( sdl.window, parms.width, parms.height );
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
		common->Warning( "idDeviceManagerLocal::SetScreenParms: unsupported fullScreen value %d", parms.fullScreen );
		return false;
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.displayFrequency = parms.displayHz;
	glConfig.multisamples = parms.multiSamples;

	DM_SetMultisamples( parms.multiSamples );

	// Re-applies our icon windows icon
	SetWindowsIcon( sdl.window );

	return true;
}

/*
===================
idDeviceManagerLocal::Shutdown
===================
*/
void idDeviceManagerLocal::Shutdown( bool shutdownSDL )
{
	common->Printf( "Shutting down %s subsystem\n", DM_APIName() );

	RestoreGamma();

	if( SDL_GetWindowFlags( sdl.window ) & SDL_WINDOW_FULLSCREEN )
	{
		SDL_SetWindowFullscreen( sdl.window, SDL_FALSE );
		SDL_Delay( 100 );
	}

	DM_DestroyContext();

	if( sdl.window )
	{
		SDL_DestroyWindow( sdl.window );
		sdl.window = NULL;
	}

	DM_ShutdownSDL( shutdownSDL );
}

/*
===================
idDeviceManagerLocal::SwapBuffers
===================
*/
void idDeviceManagerLocal::SwapBuffers()
{
	DM_SwapBuffers( sdl.window );
}

/*
===================
idDeviceManagerLocal::GetWindow
===================
*/
void* idDeviceManagerLocal::GetWindow()
{
	return sdl.window;
}

/*
===================
idDeviceManagerLocal::IsFullscreen
===================
*/
bool idDeviceManagerLocal::IsFullscreen()
{
	return glConfig.isFullscreen > 0;
}

/*
===================
idDeviceManagerLocal::NotifyWindowResized

Called by the executable's event pump when SDL reports a resize.
===================
*/
void idDeviceManagerLocal::NotifyWindowResized( int width, int height )
{
	glConfig.nativeScreenWidth = width;
	glConfig.nativeScreenHeight = height;

	if( glConfig.isFullscreen <= 0 )
	{
		r_windowWidth.SetInteger( width );
		r_windowHeight.SetInteger( height );
	}
}

/*
===================
idDeviceManagerLocal::GrabInput
===================
*/
void idDeviceManagerLocal::GrabInput( int flags )
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

	if( cvarSystem->GetCVarBool( "in_nograb" ) )
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
idDeviceManagerLocal::SetWindowsIcon
========================
*/
void idDeviceManagerLocal::SetWindowsIcon( void* window )
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

#include "game_icon.h" // contains the struct game_icon

	SDL_Surface* icon = SDL_CreateRGBSurfaceFrom( ( void* )game_icon.pixel_data, game_icon.width, game_icon.height,
						game_icon.bytes_per_pixel * 8, game_icon.bytes_per_pixel * game_icon.width,
						rmask, gmask, bmask, amask );

	SDL_SetWindowIcon( ( SDL_Window* )window, icon );

	SDL_FreeSurface( icon );
}

/*
========================
idDeviceManagerLocal::SetWindowTitle

Updates the OS window title. Passing NULL or an empty string reverts to
the default title.
========================
*/
void idDeviceManagerLocal::SetWindowTitle( const char* modTitle )
{
	if( !sdl.window )
	{
		return;
	}

	idStr title;
	if( modTitle && modTitle[0] )
	{
		title = va( "%s (x%u Build)", modTitle, ( unsigned )( sizeof( void* ) * 8 ) );
	}
	else
	{
		title = va( "%s - %s %s (x%u Build)", GAME_NAME, ID__DATE__, ID__TIME__, ( unsigned int )( sizeof( void* ) * 8 ) );
	}

	SDL_SetWindowTitle( sdl.window, title.c_str() );
}

/*
========================
idDeviceManagerLocal::SetTitleIcon
========================
*/
bool idDeviceManagerLocal::SetTitleIcon( const char* OSPath )
{
	if( !sdl.window || !OSPath || !OSPath[0] )
	{
		return false;
	}

	SDL_Surface* icon = IMG_Load( OSPath );
	if( !icon )
	{
		common->DPrintf( "SetTitleIcon: couldn't load '%s': %s\n", OSPath, IMG_GetError() );
		return false;
	}

	SDL_SetWindowIcon( sdl.window, icon );
	SDL_FreeSurface( icon );
	return true;
}
