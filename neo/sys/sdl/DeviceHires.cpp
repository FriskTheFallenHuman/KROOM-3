/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

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
#include "DeviceHires.h"

idTimeHiRes timerHiRes;

/*
========================================================================

	This class contains code adapted from Blat Blatnik's precise_sleep.c sample
	(retrieved 2024-04-10), which is under the Unlicense license:

	https://github.com/blat-blatnik/Snippets/blob/main/precise_sleep.c
	https://github.com/blat-blatnik/Snippets/blob/main/LICENSE

========================================================================
*/

/*
==============
idTimeHiRes::idTimeHiRes
==============
*/
idTimeHiRes::idTimeHiRes()
{
	m_frequency = 0;
	m_startCounter = 0;
}

/*
==============
idTimeHiRes::Init
==============
*/
void idTimeHiRes::Init()
{
	m_frequency = SDL_GetPerformanceFrequency();
	m_startCounter = SDL_GetPerformanceCounter();
}

/*
==============
idTimeHiRes::Shutdown
==============
*/
void idTimeHiRes::Shutdown()
{
}

/*
==============
idTimeHiRes::ClockCount
==============
*/
int64 idTimeHiRes::ClockCount()
{
	if( !idLib::IsMainThread() )
	{
		return 0;
	}

	return ( int64 )SDL_GetPerformanceCounter();
}

/*
==============
idTimeHiRes::ClockCountToMilliseconds
==============
*/
double idTimeHiRes::ClockCountToMilliseconds( int64 count )
{
	if( !idLib::IsMainThread() )
	{
		return 0.0;
	}

	return ( ( double )count / ( double )m_frequency ) * 1000.0;
}

/*
==============
idTimeHiRes::Sleep
==============
*/
void idTimeHiRes::Sleep( double milliseconds )
{
	if( !idLib::IsMainThread() )
	{
		return;
	}

	double seconds = milliseconds * 0.001;

	uint64 start = SDL_GetPerformanceCounter();
	uint64 target = start + ( uint64 )( seconds * ( double )m_frequency );

	// if we need to sleep more than ~2 ms, use SDL_Delay
	// to avoid burning CPU unnecessarily.
	if( milliseconds > 2.0 )
	{
		// Sleep for a bit less than the target, leaving some margin for the spin loop.
		double sleepMs = milliseconds - 1.0; // 1 ms its a breathing margin
		if( sleepMs > 0 )
		{
			SDL_Delay( ( Uint32 )sleepMs );
		}
	}

	// Busy‑spin for the remaining time to achieve high precision.
	while( SDL_GetPerformanceCounter() < target )
	{
		// Pause instruction to reduce power usage and improve hyper‑threading.
		SDL_CPUPauseInstruction();
	}
}

/*
==============
Sys_EnableThreadAffinity
==============
*/
void Sys_EnableThreadAffinity( bool enable )
{
#ifdef _WIN32
	static bool isEnabled = false;
	static DWORD_PTR previousAffinityMask = 0;

	if( !idLib::IsMainThread() )
	{
		return;
	}
	if( enable )
	{
		if( !isEnabled )
		{
			isEnabled = true;
			HANDLE hThread = GetCurrentThread();
			previousAffinityMask = SetThreadAffinityMask( hThread, 0x1 ); // previousAffinityMask will be 0 if this call fails
			Sleep( 0 );
		}
	}
	else
	{
		if( isEnabled )
		{
			isEnabled = false;
			if( previousAffinityMask )
			{
				HANDLE hThread = GetCurrentThread();
				SetThreadAffinityMask( hThread, previousAffinityMask );
				previousAffinityMask = 0;
				Sleep( 0 );
			}
		}
	}
#endif
}

/*
==============
Sys_HiResClockCount
==============
*/
int64 Sys_HiResClockCount()
{
	return timerHiRes.ClockCount();
}

/*
==============
Sys_HiResClockCountToMilliseconds
==============
*/
double Sys_HiResClockCountToMilliseconds( int64 count )
{
	return timerHiRes.ClockCountToMilliseconds( count );
}

/*
==============
Sys_SleepHiRes
==============
*/
void Sys_SleepHiRes( double milliseconds )
{
	timerHiRes.Sleep( milliseconds );
}