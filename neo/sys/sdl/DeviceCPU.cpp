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

// DG: SDL_*.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strcasecmp
#undef strncmp
#undef vsnprintf
// DG end

#ifdef _MSC_VER
	extern bool HasDAZ();
	extern bool IsAMD();
#endif

#include <SDL2/SDL_cpuinfo.h>
#include <SDL2/SDL_timer.h>

/*
==============================================================

	Clock ticks

==============================================================
*/

/*
================
Sys_GetClockTicks
================
*/
double Sys_GetClockTicks()
{
	return ( double )SDL_GetPerformanceCounter();
}

/*
================
Sys_ClockTicksPerSecond
================
*/
double Sys_ClockTicksPerSecond()
{
	static double ticks = 0.0;
	if( !ticks )
	{
		ticks = ( double )SDL_GetPerformanceFrequency();
	}
	return ticks;
}

/*
==============================================================

	CPU Count

==============================================================
*/

/*
========================
Sys_CPUCount

numLogicalCPUCores	- the number of logical CPU per core
numPhysicalCPUCores	- the total number of cores per package
numCPUPackages		- the total number of packages (physical processors)
========================
*/
void Sys_CPUCount( int& numLogicalCPUCores, int& numPhysicalCPUCores, int& numCPUPackages )
{
	numPhysicalCPUCores = 1;
	numLogicalCPUCores = SDL_GetCPUCount();
	numCPUPackages = 1;
}

/*
==============================================================

	CPUID

==============================================================
*/

#ifdef _MSC_VER
/*
================
IsAMD
================
*/
bool IsAMD()
{
	int  info[4] = {0};
	char vendor[13];

	__cpuid( info, 0 );

	memcpy( vendor, &info[1], 4 );
	memcpy( vendor + 4, &info[2], 4 );
	memcpy( vendor + 8, &info[3], 4 );
	vendor[12] = '\0';

	if( idStr::Cmp( vendor, "AuthenticAMD" ) == 0 )
	{
		return true;
	}
	return false;
}

/*
================
HasDAZ
================
*/
bool HasDAZ()
{
	unsigned int mxcsr = _mm_getcsr();
	return ( mxcsr & 0x40 ) != 0;	// Bit 6 (0x40) of MXCSR indicates the DAZ state.
}
#endif

/*
================
Sys_GetProcessorId
================
*/
int Sys_GetProcessorId()
{
#ifdef _MSC_VER
	int flags;

	// check for an AMD
	if( IsAMD() )
	{
		flags = CPUID_AMD;
	}
	else
	{
		flags = CPUID_INTEL;
	}
#else
	int flags = CPUID_GENERIC;
#endif

	// check for Multi Media Extensions
	if( SDL_HasMMX() )
	{
		flags |= CPUID_MMX;
	}

	// check for Streaming SIMD Extensions
	if( SDL_HasSSE() )
	{
		flags |= CPUID_SSE | CPUID_FTZ;
	}

#ifdef _MSC_VER
	// check for Denormals-Are-Zero mode
	if( HasDAZ() )
	{
		flags |= CPUID_DAZ;
	}
#endif

	return flags;
}

/*
================
Sys_GetProcessorString
================
*/
const char* Sys_GetProcessorString()
{
	int flags = Sys_GetProcessorId();

	if( flags & CPUID_AMD )
	{
		return "AMD CPU";
	}
	else if( flags & CPUID_INTEL )
	{
		return "Intel CPU";
	}

	return "generic CPU";
}

/*
===============================================================================

	FPU

===============================================================================
*/

/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ( bool enable )
{
#ifdef _MSC_VER
	unsigned int mxcsr = _mm_getcsr();
	mxcsr &= ~( 1 << 6 );
	if( enable )
	{
		mxcsr |= ( 1 << 6 );
	}
	_mm_setcsr( mxcsr );
#endif
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ( bool enable )
{
#ifdef _MSC_VER
	unsigned int mxcsr = _mm_getcsr();
	mxcsr &= ~( 1u << 15 );
	if( enable )
	{
		mxcsr |= ( 1u << 15 );
	}
	_mm_setcsr( mxcsr );
#endif
}