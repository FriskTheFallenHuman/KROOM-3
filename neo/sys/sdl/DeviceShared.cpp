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

#include "DeviceSDL.h"

namespace fs = std::filesystem;

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
uint64 Sys_Microseconds()
{
	static uint64_t frequency = SDL_GetPerformanceFrequency();
	uint64_t counter = SDL_GetPerformanceCounter();
	return ( counter * 1000000 ) / frequency;
}

/*
==============
Sys_GetSystemRam
==============
*/
int Sys_GetSystemRam()
{
	int mb = SDL_GetSystemRAM();
	return ( mb + 8 ) & ~15;
}

/*
================
Sys_GetDriveFreeSpace
returns in megabytes
================
*/
int Sys_GetDriveFreeSpace( const char* path )
{
	try
	{
		fs::space_info si = fs::space( path );
		int64_t freeBytes = static_cast<int64_t>( si.available );
		int ret = static_cast<int>( freeBytes / ( 1024 * 1024 ) );

		return ( ret >= 0 ) ? ret : -ret;
	}
	catch( const fs::filesystem_error& )
	{
		return 26;
	}
}

/*
========================
Sys_GetDriveFreeSpaceInBytes
========================
*/
int64 Sys_GetDriveFreeSpaceInBytes( const char* path )
{
	try
	{
		fs::space_info si = fs::space( path );
		int64_t freeBytes = static_cast<int64_t>( si.available );
		return ( freeBytes >= 0 ) ? freeBytes : -freeBytes;
	}
	catch( const fs::filesystem_error& )
	{
		return 1;
	}
}

/*
================
Sys_Wcstrtombstr

wcstombs is unreliable on Windows so instead it has to be done the Windows way in order to include the Sytem's ANSI code page
================
*/
int Sys_Wcstrtombstr( char* Dest, const wchar_t* Source, size_t size )
{
#ifdef WIN32
	int wstrlen = size;
	int mbstrlen = WideCharToMultiByte( CP_ACP, NULL, Source, wstrlen, NULL, 0, NULL, 0 );
	if( WideCharToMultiByte( CP_ACP, NULL, Source, wstrlen, Dest, mbstrlen, NULL, 0 ) > 0 )
	{
		Dest[mbstrlen] = '\0';
		return mbstrlen + 1;
	}
	return -1;
#else
	return wcstombs( Dest, Source, size );
#endif
}

/*
================
Sys_Wcstrtombstr

wcstombs is unreliable on Windows so instead it has to be done the Windows way in order to include the Sytem's ANSI code page
================
*/
int Sys_Mbstrtowcstr( wchar_t* Dest, const char* Source, size_t size )
{
#ifdef WIN32
	int strlen = size;
	int wstrlen = MultiByteToWideChar( CP_ACP, NULL, Source, strlen, NULL, 0 );
	if( MultiByteToWideChar( CP_ACP, NULL, Source, strlen, Dest, wstrlen ) > 0 )
	{
		Dest[wstrlen] = '\0';
		return wstrlen + 1;
	}
	return -1;
#else
	return mbstowcs( Dest, Source, size );
#endif
}