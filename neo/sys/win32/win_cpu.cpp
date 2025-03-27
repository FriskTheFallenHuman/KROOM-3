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

/*
==============================================================

	CPU Flags

==============================================================
*/

/*
================
CPUID
================
*/
#if !defined(_WIN64)
static inline void CPUid( int index, int* a, int* b, int* c, int* d )
{
	int info[4] = {};

	// VS2005 and up
	__cpuid( info, index );

	*a = info[0];
	*b = info[1];
	*c = info[2];
	*d = info[3];
}
#endif


/*
================
HasMMX
================
*/
#if !defined(_WIN64)
static inline bool HasMMX()
{
	int a, b, c, d;

	CPUid( 0, &a, &b, &c, &d );
	if( a < 1 )
	{
		return false;
	}

	CPUid( 1, &a, &b, &c, &d );

	return ( c & ( 1 << 23 ) ) == ( 1 << 23 );
}
#endif

/*
================
HasSSE
================
*/
#if !defined(_WIN64)
static inline bool HasSSE()
{
	int a, b, c, d;

	CPUid( 0, &a, &b, &c, &d );
	if( a < 1 )
	{
		return false;
	}

	CPUid( 1, &a, &b, &c, &d );

	return ( c & ( 1 << 25 ) ) == ( 1 << 25 );
}
#endif

/*
================
HasDAZ
================
*/
#if !defined(_WIN64)
static inline bool HasDAZ()
{
	int a, b, c, d;

	CPUid( 0, &a, &b, &c, &d );
	if( a < 1 )
	{
		return false;
	}

	CPUid( 1, &a, &b, &c, &d );

	return ( d & ( 1 << 24 ) ) == ( 1 << 24 );
}
#endif

#ifndef USE_SDL
/*
================================================================================================

	CPU Count

================================================================================================
*/

/*
========================
CountSetBits
Helper function to count set bits in the processor mask.
========================
*/
DWORD CountSetBits( ULONG_PTR bitMask )
{
	DWORD LSHIFT = sizeof( ULONG_PTR ) * 8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = ( ULONG_PTR )1 << LSHIFT;

	for( DWORD i = 0; i <= LSHIFT; i++ )
	{
		bitSetCount += ( ( bitMask & bitTest ) ? 1 : 0 );
		bitTest /= 2;
	}

	return bitSetCount;
}

typedef BOOL ( WINAPI* LPFN_GLPI )( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD );

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
bool GetCPUInfo( cpuInfo_t& cpuInfo )
{
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	PCACHE_DESCRIPTOR Cache;
	LPFN_GLPI	glpi;
	BOOL		done = FALSE;
	DWORD		returnLength = 0;
	DWORD		byteOffset = 0;

	memset( & cpuInfo, 0, sizeof( cpuInfo ) );

	glpi = ( LPFN_GLPI )GetProcAddress( GetModuleHandle( TEXT( "kernel32" ) ), "GetLogicalProcessorInformation" );
	if( NULL == glpi )
	{
		idLib::Printf( "\nGetLogicalProcessorInformation is not supported.\n" );
		return 0;
	}

	while( !done )
	{
		DWORD rc = glpi( buffer, &returnLength );

		if( FALSE == rc )
		{
			if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
			{
				if( buffer )
				{
					free( buffer );
				}

				buffer = ( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION )malloc( returnLength );
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

	while( byteOffset + sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION ) <= returnLength )
	{
		switch( ( LOGICAL_PROCESSOR_RELATIONSHIP_LOCAL ) ptr->Relationship )
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
				if( Cache->Level >= 1 && Cache->Level <= 3 )
				{
					int level = Cache->Level - 1;
					if( cpuInfo.cacheLevel[level].count > 0 )
					{
						cpuInfo.cacheLevel[level].count++;
					}
					else
					{
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
Sys_CPUCount

numLogicalCPUCores	- the number of logical CPU per core
numPhysicalCPUCores	- the total number of cores per package
numCPUPackages		- the total number of packages (physical processors)
========================
*/
void Sys_CPUCount( int& numLogicalCPUCores, int& numPhysicalCPUCores, int& numCPUPackages )
{
	cpuInfo_t cpuInfo;
	GetCPUInfo( cpuInfo );

	numPhysicalCPUCores = cpuInfo.processorCoreCount;
	numLogicalCPUCores = cpuInfo.logicalProcessorCount;
	numCPUPackages = cpuInfo.processorPackageCount;
}
#endif /* USE_SDL */

/*
================
Sys_GetProcessorId
================
*/
int Sys_GetProcessorId()
{
#ifdef USE_SDL
	int SDL_GetProcessorId();
	return SDL_GetProcessorId();
#else
#if defined(_WIN64)
	int flags = CPUID_GENERIC;

	flags |= CPUID_MMX;
	flags |= CPUID_SSE;

	return flags;
#else
	int flags = CPUID_GENERIC;

	// check for Multi Media Extensions
	if( HasMMX() )
	{
		flags |= CPUID_MMX;
	}

	// check for Streaming SIMD Extensions
	if( HasSSE() )
	{
		flags |= CPUID_SSE;
	}

	return flags;
#endif
#endif
}


/*
===============================================================================

	FPU

===============================================================================
*/

#ifdef _MSC_VER
	#define STREFLOP_STMXCSR(cw) do { (cw) = static_cast<int>(_mm_getcsr()); } while (0)
	#define STREFLOP_LDMXCSR(cw) do { _mm_setcsr(static_cast<unsigned int>(cw)); } while (0)
#else
	#define STREFLOP_STMXCSR(cw)
	#define STREFLOP_LDMXCSR(cw)
#endif

/*
===============
EnableMXCSRFlag
===============
*/
static void EnableMXCSRFlag( int flag, bool enable, const char* name )
{
	int sse_mode;

	STREFLOP_STMXCSR( sse_mode );

	if( enable && ( sse_mode & flag ) == flag )
	{
		common->Printf( "%s mode is already enabled\n", name );
		return;
	}

	if( !enable && ( sse_mode & flag ) == 0 )
	{
		common->Printf( "%s mode is already disabled\n", name );
		return;
	}

	if( enable )
	{
		common->Printf( "enabling %s mode\n", name );
		sse_mode |= flag;
	}
	else
	{
		common->Printf( "disabling %s mode\n", name );
		sse_mode &= ~flag;
	}

	STREFLOP_LDMXCSR( sse_mode );
}


/*
===============
Sys_FPU_SetPrecision
===============
*/
void Sys_FPU_SetPrecision()
{
#if defined(_MSC_VER) && !defined(_WIN64)
	_controlfp( _PC_64, _MCW_PC );
#endif
}

/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ( bool enable )
{
#if !defined(_WIN64)
	if( !HasDAZ() )
	{
		common->Printf( "this CPU doesn't support Denormals-Are-Zero\n" );
		return;
	}

	EnableMXCSRFlag( ( 1 << 6 ), enable, "Denormals-Are-Zero" );
#endif
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ( bool enable )
{
#if !defined(_WIN64)
	EnableMXCSRFlag( ( 1 << 15 ), enable, "Flush-To-Zero" );
#endif
}
