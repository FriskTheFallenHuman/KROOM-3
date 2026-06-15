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

#include "win_local.h"
#include "win_hirez.h"

idTimeHiRes timerHiRes;

/*
========================================================================

	This class contains code adapted from Blat Blatnik's precise_sleep.c sample
	(retrieved 2024-04-10), which is under the Unlicense license:

	https://github.com/blat-blatnik/Snippets/blob/main/precise_sleep.c
	https://github.com/blat-blatnik/Snippets/blob/main/LICENSE

========================================================================
*/
extern "C"
{
	__declspec( dllexport ) DWORD NvOptimusEnablement = 0x00000001;
	__declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1;
}

/*
===================
idTimeHiRes::idTimeHiRes
===================
*/
idTimeHiRes::idTimeHiRes()
{
	Timer = NULL;
	SchedulerPeriodMs = 0;
	QpcPerSecond = 0;
}

/*
===================
idTimeHiRes::Init
===================
*/
void idTimeHiRes::Init()
{
	if( !Timer )
	{
		Timer = CreateWaitableTimerEx( NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS );
	}

	TIMECAPS timeCaps;
	bool capsObtained = false;
	if( timeGetDevCaps( &timeCaps, sizeof( timeCaps ) ) == MMSYSERR_NOERROR )
	{
		SchedulerPeriodMs = ( int )timeCaps.wPeriodMin;
		if( SchedulerPeriodMs > 0 )
		{
			timeBeginPeriod( ( UINT )SchedulerPeriodMs );
			capsObtained = true;
		}
	}
	if( !capsObtained )
	{
		SchedulerPeriodMs = 0;
	}

	LARGE_INTEGER qpf;
	QueryPerformanceFrequency( &qpf );
	QpcPerSecond = qpf.QuadPart;
}

/*
===================
idTimeHiRes::Shutdown
===================
*/
void idTimeHiRes::Shutdown()
{
	if( Timer )
	{
		CloseHandle( Timer );
		Timer = NULL;
	}
	if( SchedulerPeriodMs > 0 )
	{
		timeEndPeriod( ( uint )SchedulerPeriodMs );
	}
	SchedulerPeriodMs = 0;
}

/*
===================
idTimeHiRes::ClockCount
===================
*/
int64 idTimeHiRes::ClockCount()
{
	if( !idLib::IsMainThread() )
	{
		return 0;
	}
	LARGE_INTEGER qpc;
	QueryPerformanceCounter( &qpc );
	int64 count = qpc.QuadPart;
	return count;
}

/*
===================
idTimeHiRes::ClockCountToMilliseconds
===================
*/
double idTimeHiRes::ClockCountToMilliseconds( int64 count )
{
	if( !idLib::IsMainThread() )
	{
		return 0.0;
	}
	double msec = ( ( double )count / ( double )QpcPerSecond ) * 1000.0;
	return msec;
}

/*
===================
idTimeHiRes::Sleep
===================
*/
void idTimeHiRes::Sleep( double milliseconds )
{
	if( !idLib::IsMainThread() )
	{
		return;
	}
	double seconds = milliseconds * 0.001;

	LARGE_INTEGER qpc;
	QueryPerformanceCounter( &qpc );
	int64 targetQpc = qpc.QuadPart + (int64)( seconds * ( double )QpcPerSecond );

	if( SchedulerPeriodMs > 0 )
	{
		// Try using a high resolution timer first.
		if( Timer )
		{
			const double TOLERANCE = 0.001'02;
			int64 maxTicks = (int64)SchedulerPeriodMs * 9'500;
			
			// Break sleep up into parts that are lower than scheduler period.
			for( ;; )
			{
				double remainingSeconds = ( double )( targetQpc - qpc.QuadPart ) / ( double )QpcPerSecond;
				int64 sleepTicks = (int64)( ( remainingSeconds - TOLERANCE ) * 10'000'000.0 ); // 100ns intervals
				if( sleepTicks <= 0 )
				{
					break;
				}

				LARGE_INTEGER due;
				due.QuadPart = -( sleepTicks > maxTicks ? maxTicks : sleepTicks );
				SetWaitableTimerEx( Timer, &due, 0, NULL, NULL, NULL, 0 );
				WaitForSingleObject( Timer, INFINITE );
				QueryPerformanceCounter( &qpc );
			}
		}
		else
		{
			// Fallback to Sleep.
			const double TOLERANCE = 0.000'02;
			double sleepMs = (seconds - TOLERANCE) * 1000.0 - (double)SchedulerPeriodMs; // Sleep for 1 scheduler period less than requested.
			int sleepSlices = (int)(sleepMs / (double)SchedulerPeriodMs);
			if( sleepSlices > 0 )
			{
				Sleep((DWORD)sleepSlices * (DWORD)SchedulerPeriodMs);
			}

			QueryPerformanceCounter(&qpc);
		}
	}

	// Spin for any remaining time.
	while( qpc.QuadPart < targetQpc )
	{
		YieldProcessor();
		QueryPerformanceCounter(&qpc);
	}
}

/*
==============
Sys_EnableThreadAffinity
==============
*/
void Sys_EnableThreadAffinity( bool enable )
{
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