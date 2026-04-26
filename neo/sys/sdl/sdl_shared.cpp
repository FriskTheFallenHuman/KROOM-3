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

#include <SDL.h>
#include <SDL_timer.h>

/*
================
Sys_Milliseconds
================
*/
unsigned int Sys_Milliseconds()
{
	return SDL_GetTicks();
}

/*
========================
Sys_Microseconds
========================
*/
uint64_t Sys_Microseconds()
{
	static uint64_t frequency = SDL_GetPerformanceFrequency(); // Frequency of the high-resolution counter
	uint64_t counter = SDL_GetPerformanceCounter();           // Current counter value
	return ( counter * 1000000 ) / frequency;                 // Convert to microseconds
}

/*
========================
Sys_SDLIcon
========================
*/
void Sys_SDLIcon( SDL_Window* window )
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

	SDL_SetWindowIcon( window, icon );

	SDL_FreeSurface( icon );
}
