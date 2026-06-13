/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

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

#include "renderer/RenderCommon.h"

#include <SDL2/SDL.h>

extern SDL_Window* window;

/*
====================
DumpAllDisplayDevices
====================
*/
static void DumpAllDisplayDevices()
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
R_GetModeListForDisplay
====================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList, const int minHeight )
{
	assert( requestedDisplayNum >= 0 );
	modeList.Clear();

	if( requestedDisplayNum >= SDL_GetNumVideoDisplays() )
	{
		common->Warning( "R_GetModeListForDisplay: invalid display index %d (only %d displays)", requestedDisplayNum, SDL_GetNumVideoDisplays() );
		return false;
	}

	int numModes = SDL_GetNumDisplayModes( requestedDisplayNum );
	if( numModes <= 0 )
	{
		common->Warning( "R_GetModeListForDisplay: no modes for display %d", requestedDisplayNum );
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
		common->Warning( "R_GetModeListForDisplay: no modes passed filtering (minHeight=%d)", minHeight );
		return false;
	}

	modeList.SortWithTemplate( idSort_VidMode() );
	return true;
}

/*
====================
R_GetDefaultDisplayMode
====================
*/
bool R_GetDefaultDisplayMode( int& defaultDisplayNum, vidMode_t& defaultMode )
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
	if( parms.fullScreen == -1 )
	{
		// Switch to borderless window at given position/size
		SDL_SetWindowBordered( window, SDL_FALSE );
		SDL_SetWindowPosition( window, parms.x, parms.y );
		SDL_SetWindowSize( window, parms.width, parms.height );

		// If currently fullscreen, exit fullscreen mode first
		if( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN )
		{
			SDL_SetWindowFullscreen( window, SDL_FALSE );
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
		common->Warning( "GLimp_SetScreenParms: unsupported fullScreen value %d", parms.fullScreen );
		return false;
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.displayFrequency = parms.displayHz;
	glConfig.multisamples = parms.multiSamples;

	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples );

	return true;
}