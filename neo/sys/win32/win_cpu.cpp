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

#include "win_local.h"

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
	int  cpureg[4] = {0};
	__cpuid( cpureg, 0 );

	unsigned __int64 ticks = __rdtsc();
	return (double)ticks;
}

/*
================
Sys_ClockTicksPerSecond
================
*/
double Sys_ClockTicksPerSecond()
{
	static double ticks = 0.0;
	if ( !ticks )
	{
		LARGE_INTEGER li;
		if ( QueryPerformanceFrequency( &li ) )
		{
			ticks = (double)li.QuadPart;
		}
	}
	return ticks;
}


/*
==============================================================

	CPUID

==============================================================
*/

#define _REG_EAX		0
#define _REG_EBX		1
#define _REG_ECX		2
#define _REG_EDX		3

/*
================
IsAMD
================
*/
static bool IsAMD()
{
	int  info[4] = {0};
	char vendor[13];

	__cpuid( info, 0 );

	memcpy(vendor, &info[1], 4);
	memcpy(vendor + 4, &info[2], 4);
	memcpy(vendor + 8, &info[3], 4);
	vendor[12] = '\0';

	if ( strcmp( vendor, "AuthenticAMD" ) == 0 ) {
		return true;
	}
	return false;
}

/*
================
HasMMX
================
*/
static bool HasMMX()
{
	int  regs[4] = {0};
	__cpuid( regs, 1 );

	// bit 23 of EDX denotes MMX existence
	return ( regs[_REG_EDX] & ( 1 << 23 ) ) != 0;
}

/*
================
HasSSE
================
*/
static bool HasSSE()
{
	int  regs[4] = { 0 };
	__cpuid( regs, 1 );

	// bit 25 of EDX denotes SSE existence
	return ( regs[_REG_EDX] & ( 1 << 25 ) ) != 0;
}

/*
================
HasDAZ
================
*/
static bool HasDAZ()
{
	unsigned int mxcsr = _mm_getcsr();
	return ( mxcsr & 0x40 ) != 0;	// Bit 6 (0x40) of MXCSR indicates the DAZ state.
}

/*
================================================================================================

	CPU

================================================================================================
*/

/*
================
CPUCount

	logicalNum is the number of logical CPU per physical CPU
	physicalNum is the total number of physical processor
	returns one of the HT_* flags
================
*/
static int CPUCount( int & logicalNum, int &physicalNum )
{
	logicalNum = std::thread::hardware_concurrency();

	SYSTEM_LOGICAL_PROCESSOR_INFORMATION infoBuffer[256];
	DWORD bufferSize = sizeof(infoBuffer);
	physicalNum = 0;

	if ( GetLogicalProcessorInformation( infoBuffer, &bufferSize ) )
	{
		for ( DWORD i = 0; i < bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION ); ++i)
		{
			if ( infoBuffer[i].Relationship == RelationProcessorCore ){
				physicalNum++;
			}
		}
	}
	else
	{
		physicalNum = 1;
	}

	return 0;
}

/*
========================
CountSetBits
Helper function to count set bits in the processor mask.
========================
*/
static DWORD CountSetBits( ULONG_PTR bitMask )
{
	DWORD LSHIFT = sizeof( ULONG_PTR ) * 8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;

	for ( DWORD i = 0; i <= LSHIFT; i++ ) {
		bitSetCount += ( ( bitMask & bitTest ) ? 1 : 0 );
		bitTest /= 2;
	}

	return bitSetCount;
}

typedef BOOL (WINAPI *LPFN_GLPI)( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD );

enum LOGICAL_PROCESSOR_RELATIONSHIP_LOCAL
{
	localRelationProcessorCore,
	localRelationNumaNode,
	localRelationCache,
	localRelationProcessorPackage
};

struct cpuInfo_t
{
	int processorPackageCount;
	int processorCoreCount;
	int logicalProcessorCount;
	int numaNodeCount;
	struct cacheInfo_t
	{
		int count;
		int associativity;
		int lineSize;
		int size;
	} cacheLevel[3];
};

/*
========================
GetCPUInfo
========================
*/
static bool GetCPUInfo( cpuInfo_t & cpuInfo )
{
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	PCACHE_DESCRIPTOR Cache;
	LPFN_GLPI	glpi;
	BOOL		done = FALSE;
	DWORD		returnLength = 0;
	DWORD		byteOffset = 0;

	memset( & cpuInfo, 0, sizeof( cpuInfo ) );

	glpi = (LPFN_GLPI)GetProcAddress( GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation" );
	if ( NULL == glpi )
	{
		idLib::Printf( "\nGetLogicalProcessorInformation is not supported.\n" );
		return 0;
	}

	while ( !done )
	{
		DWORD rc = glpi( buffer, &returnLength );

		if ( FALSE == rc )
		{
			if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
			{
				if ( buffer )
				{
					free( buffer );
				}

				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc( returnLength );
			}
			else
			{
				idLib::Printf( "Sys_CPUCount error: %d\n", GetLastError() );
				return false;
			}
		}
		else
		{
			done = TRUE;
		}
	}

	ptr = buffer;

	while ( byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength )
	{
		switch ( (LOGICAL_PROCESSOR_RELATIONSHIP_LOCAL) ptr->Relationship )
		{
			case localRelationProcessorCore:
				cpuInfo.processorCoreCount++;

				// A hyperthreaded core supplies more than one logical processor.
				cpuInfo.logicalProcessorCount += CountSetBits( ptr->ProcessorMask );
				break;

			case localRelationNumaNode:
				// Non-NUMA systems report a single record of this type.
				cpuInfo.numaNodeCount++;
				break;

			case localRelationCache:
				// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache.
				Cache = &ptr->Cache;
				if ( Cache->Level >= 1 && Cache->Level <= 3 ) {
					int level = Cache->Level - 1;
					if ( cpuInfo.cacheLevel[level].count > 0 ) {
						cpuInfo.cacheLevel[level].count++;
					} else {
						cpuInfo.cacheLevel[level].associativity = Cache->Associativity;
						cpuInfo.cacheLevel[level].lineSize = Cache->LineSize;
						cpuInfo.cacheLevel[level].size = Cache->Size;
					}
				}
				break;

			case localRelationProcessorPackage:
				// Logical processors share a physical package.
				cpuInfo.processorPackageCount++;
				break;

			default:
				idLib::Printf( "Error: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n" );
				break;
		}
		byteOffset += sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION );
		ptr++;
	}

	free( buffer );

	return true;
}

/*
========================
Sys_GetCPUCacheSize
========================
*/
static void Sys_GetCPUCacheSize( int level, int & count, int & size, int & lineSize )
{
	assert( level >= 1 && level <= 3 );
	cpuInfo_t cpuInfo;

	GetCPUInfo( cpuInfo );

	count = cpuInfo.cacheLevel[level - 1].count;
	size = cpuInfo.cacheLevel[level - 1].size;
	lineSize = cpuInfo.cacheLevel[level - 1].lineSize;
}

/*
========================
Sys_CPUCount

numLogicalCPUCores	- the number of logical CPU per core
numPhysicalCPUCores	- the total number of cores per package
numCPUPackages		- the total number of packages (physical processors)
========================
*/
void Sys_CPUCount( int & numLogicalCPUCores, int & numPhysicalCPUCores, int & numCPUPackages )
{
	cpuInfo_t cpuInfo;
	GetCPUInfo( cpuInfo );

	numPhysicalCPUCores = cpuInfo.processorCoreCount;
	numLogicalCPUCores = cpuInfo.logicalProcessorCount;
	numCPUPackages = cpuInfo.processorPackageCount;
}

/*
================
Sys_GetProcessorId
================
*/
int Sys_GetProcessorId()
{
	int flags;

	// check for an AMD
	if ( IsAMD() )
	{
		flags = CPUID_AMD;
	}
	else
	{
		flags = CPUID_INTEL;
	}

	// check for Multi Media Extensions
	if ( HasMMX() )
	{
		flags |= CPUID_MMX;
	}

	// check for Streaming SIMD Extensions
	if ( HasSSE() )
	{
		flags |= CPUID_SSE | CPUID_FTZ;
	}

	// check for Denormals-Are-Zero mode
	if ( HasDAZ() )
	{
		flags |= CPUID_DAZ;
	}

	return flags;
}

/*
================
Sys_GetProcessorString
================
*/
const char *Sys_GetProcessorString()
{
	int flags = Sys_GetProcessorId();

	if ( flags & CPUID_AMD )
	{
		return "AMD CPU";
	}
	else if ( flags & CPUID_INTEL )
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
	unsigned int mxcsr = _mm_getcsr();
	mxcsr &= ~( 1 << 6 );
	if ( enable )
	{
		mxcsr |= ( 1 << 6 );
	}
	_mm_setcsr( mxcsr );
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ( bool enable )
{
	unsigned int mxcsr = _mm_getcsr();
	mxcsr &= ~( 1u << 15 );
	if ( enable )
	{
		mxcsr |= ( 1u << 15 );
	}
	_mm_setcsr( mxcsr );
}