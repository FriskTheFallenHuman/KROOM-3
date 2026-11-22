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

#ifdef _WIN32
	#include <array>
	#include <vector>
	#include <bitset>

	#ifdef _MSC_VER
		extern bool HasDAZ();
	#endif
#endif

#include <SDL2/SDL_cpuinfo.h>
#include <SDL2/SDL_timer.h>

#if defined(__x86_64__) || defined(__i386__)
	#include <cpuid.h>
#endif

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

// Direct copy from https://docs.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
// trimmed down to ONLY Retrieve Vendor ID and Brand Name
class idCPUInfo
{
public:
	idCPUInfo()
		:
		isIntel_{ false },
		isAMD_{ false }
	{
#ifdef _WIN32
		std::array<int, 4> cpui;
		std::vector<std::array<int, 4>> data_;
		std::vector<std::array<int, 4>> extdata_;
		int nIds_ = 0;
		int nExIds_ = 0;

		// Calling __cpuid with 0x0 as the function_id argument
		// gets the number of the highest valid function ID.
		__cpuid( cpui.data(), 0 );
		nIds_ = cpui[0];

		for( int i = 0; i <= nIds_; ++i )
		{
			__cpuidex( cpui.data(), i, 0 );
			data_.push_back( cpui );
		}

		// Capture vendor string
		char vendor[0x20];
		memset( vendor, 0, sizeof( vendor ) );
		*reinterpret_cast<int*>( vendor ) = data_[0][1];
		*reinterpret_cast<int*>( vendor + 4 ) = data_[0][3];
		*reinterpret_cast<int*>( vendor + 8 ) = data_[0][2];
		vendor_ = vendor;
		if( vendor_ == "GenuineIntel" )
		{
			isIntel_ = true;
		}
		else if( vendor_ == "AuthenticAMD" )
		{
			isAMD_ = true;
		}

		// Calling __cpuid with 0x80000000 as the function_id argument
		// gets the number of the highest valid extended ID.
		__cpuid( cpui.data(), 0x80000000 );
		nExIds_ = cpui[0];

		char brand[0x40];
		memset( brand, 0, sizeof( brand ) );

		for( int i = 0x80000000; i <= nExIds_; ++i )
		{
			__cpuidex( cpui.data(), i, 0 );
			extdata_.push_back( cpui );
		}

		// Interpret CPU brand string if reported
		if( nExIds_ >= 0x80000004 )
		{
			memcpy( brand, extdata_[2].data(), sizeof( cpui ) );
			memcpy( brand + 16, extdata_[3].data(), sizeof( cpui ) );
			memcpy( brand + 32, extdata_[4].data(), sizeof( cpui ) );
			brand_ = brand;
		}
#elif __linux__
		FILE* fd;

		fd = fopen( "/proc/cpuinfo", "r" );
		if( fd == NULL )
		{
			return;
		}
		int processor = 0;
		size_t size = 0;
		char* line = NULL;
		while( getline( &line, &size, fd ) > 0 )
		{
			char* propertyName =  strtok( line, ":" );
			char* propertyValue = strtok( NULL, ":" ) + 1;
			if( strstr( propertyName, "processor" ) )
			{
				processor = atoi( propertyValue );
			}
			if( processor > 0 )
			{
				break;
			}
			if( strstr( propertyName, "vendor_id" ) )
			{
				if( strstr( propertyValue, "AuthenticAMD" ) )
				{
					isAMD_ = true;
				}
				else if( strstr( propertyValue, "GenuineIntel" ) )
				{
					isIntel_ = true;
				}
			}
			if( strstr( propertyName, "model name" ) )
			{
				brand_ = propertyValue;
			}
		}
		free( line );
		fclose( fd );
#endif
	};

	std::string vendor_;
	std::string brand_;
	bool isIntel_;
	bool isAMD_;

};

const idCPUInfo CPU_Rep;

/*
================
HasFMA
================
*/
static bool HasFMA()
{
#if defined(_MSC_VER) && ( defined(_M_X64) || defined(_M_IX86) )
	int info[4] = { 0 };
	__cpuid( info, 1 );
	return ( info[2] & ( 1 << 12 ) ) != 0 && SDL_HasAVX();
#elif defined(__x86_64__) || defined(__i386__)
	unsigned int eax, ebx, ecx, edx;
	return __get_cpuid( 1, &eax, &ebx, &ecx, &edx ) && ( ecx & ( 1u << 12 ) ) != 0 && SDL_HasAVX();
#else
	return false;
#endif
}

#ifdef _MSC_VER
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
	int flags;

	// check for an AMD
	flags = CPUID_GENERIC;
	if( CPU_Rep.isAMD_ )
	{
		flags = CPUID_AMD;
	}
	else
	{
		flags = CPUID_INTEL;
	}

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

	if( SDL_HasSSE2() )
	{
		flags |= CPUID_SSE2;
	}
	if( SDL_HasSSE3() )
	{
		flags |= CPUID_SSE3;
	}
	if( SDL_HasSSE41() )
	{
		flags |= CPUID_SSE41;
	}
	if( SDL_HasSSE42() )
	{
		flags |= CPUID_SSE42;
	}
	if( SDL_HasAVX() )
	{
		flags |= CPUID_AVX;
	}
	if( SDL_HasAVX2() )
	{
		flags |= CPUID_AVX2;
	}
	if( HasFMA() )
	{
		flags |= CPUID_FMA;
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

	return "Generic CPU";
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