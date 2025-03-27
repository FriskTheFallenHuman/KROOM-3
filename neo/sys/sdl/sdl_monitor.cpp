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

#include <SDL.h>

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
	modeList.AddUnique( vidMode_t( 1024,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1152,  864, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  720, 60 ) );
	modeList.AddUnique( vidMode_t( 1280, 1024, 60 ) );
	modeList.AddUnique( vidMode_t( 1366,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1440,  900, 60 ) );
	modeList.AddUnique( vidMode_t( 1400, 1050, 60 ) );
	modeList.AddUnique( vidMode_t( 1600,  900, 60 ) );
	modeList.AddUnique( vidMode_t( 1600, 1200, 60 ) );
	modeList.AddUnique( vidMode_t( 1680, 1050, 60 ) );
	modeList.AddUnique( vidMode_t( 1920, 1080, 60 ) );
	modeList.AddUnique( vidMode_t( 1920, 1200, 60 ) );
	modeList.AddUnique( vidMode_t( 2048, 1152, 60 ) );
	modeList.AddUnique( vidMode_t( 2560, 1600, 60 ) );
	modeList.AddUnique( vidMode_t( 3200, 2400, 60 ) );
	modeList.AddUnique( vidMode_t( 3840, 2160, 60 ) );
	modeList.AddUnique( vidMode_t( 4096, 2304, 60 ) );
	modeList.AddUnique( vidMode_t( 2880, 1800, 60 ) );
	modeList.AddUnique( vidMode_t( 2560, 1440, 60 ) );
	modeList.AddUnique( vidMode_t( 1440, 1080, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  800, 60 ) );
	modeList.AddUnique( vidMode_t( 2560, 1080, 60 ) );
	modeList.AddUnique( vidMode_t( 3440, 1440, 60 ) );
	modeList.AddUnique( vidMode_t( 3840, 1600, 60 ) );
	modeList.AddUnique( vidMode_t( 5120, 2160, 60 ) );
	modeList.AddUnique( vidMode_t( 3840, 1080, 60 ) );
	modeList.AddUnique( vidMode_t( 5120, 1440, 60 ) );
	modeList.AddUnique( vidMode_t( 7680, 2160, 60 ) );

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
}