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

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

#include "sdl_local.h"

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
==============
Sys_Sleep
==============
*/
void Sys_Sleep( int msec )
{
	SDL_Delay( msec );
}

/*
==============
Sys_ShowWindow
==============
*/
void Sys_ShowWindow( bool show )
{
	if( show )
	{
		SDL_ShowWindow( sdl.window );
	}
	else
	{
		SDL_HideWindow( sdl.window );
	}
}

/*
==============
Sys_IsWindowVisible
==============
*/
bool Sys_IsWindowVisible()
{
	Uint32 flags = SDL_GetWindowFlags( sdl.window );
	return ( flags & SDL_WINDOW_SHOWN ) != 0;
}

/*
========================================================================

DLL Loading

========================================================================
*/


/*
=====================
Sys_DLL_Load
=====================
*/
uintptr_t Sys_DLL_Load( const char* dllName )
{
	void* handle = SDL_LoadObject( dllName );
	if( !handle )
	{
		const char* err = SDL_GetError();
		if( err && *err )
		{
			common->Warning( "SDL_LoadObject(\"%s\") failed: %s", dllName, err );
		}
		else
		{
			common->Warning( "SDL_LoadObject(\"%s\") failed without additional info.", dllName );
		}
		return 0;
	}

	return ( uintptr_t )handle;
}

/*
=====================
Sys_DLL_GetProcAddress
=====================
*/
void* Sys_DLL_GetProcAddress( uintptr_t dllHandle, const char* procName )
{
	void* adr = SDL_LoadFunction( ( void* )dllHandle, procName );
	if( !adr )
	{
		const char* err = SDL_GetError();
		if( err && *err )
		{
			common->Warning( "SDL_LoadFunction(%p, \"%s\") failed: %s", ( void* )dllHandle, procName, err );
		}
		else
		{
			common->Warning( "SDL_LoadFunction(%p, \"%s\") failed.", ( void* )dllHandle, procName );
		}
	}
	return adr;
}

/*
=====================
Sys_DLL_Unload
=====================
*/
void Sys_DLL_Unload( uintptr_t dllHandle )
{
	if( !dllHandle )
	{
		return;
	}

	SDL_UnloadObject( ( void* )dllHandle );
}