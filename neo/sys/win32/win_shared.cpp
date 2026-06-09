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
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#undef StrCmpN
#undef StrCmpNI
#undef StrCmpI

// RB begin
#if !defined(__MINGW32__)
	#include <comdef.h>
	#include <comutil.h>
	#include <Wbemidl.h>
#endif // #if !defined(__MINGW32__)
// RB end

#pragma comment (lib, "wbemuuid.lib")

#ifndef USE_SDL
#pragma warning(disable:4740)	// warning C4740: flow in or out of inline asm code suppresses global optimization

/*
================
Sys_Milliseconds
================
*/
unsigned int Sys_Milliseconds()
{
	int sys_curtime;
	static int sys_timeBase;
	static bool	initialized = false;

	if( !initialized )
	{
		sys_timeBase = timeGetTime();
		initialized = true;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

/*
========================
Sys_Microseconds
========================
*/
uint64 Sys_Microseconds()
{
	static HMODULE hKernel32Dll = LoadLibrary( "Kernel32.dll" );
	static auto GetSystemTimePreciseAsFileTime = ( void( WINAPI* )( LPFILETIME ) )GetProcAddress( hKernel32Dll, "GetSystemTimePreciseAsFileTime" );

	FILETIME ft = {0};
	// note: both functions return number of 100-nanosec intervals since January 1, 1601 (UTC)
	if( GetSystemTimePreciseAsFileTime )
	{
		// < 1 us precision, but requires Windows 8+
		GetSystemTimePreciseAsFileTime( &ft );
	}
	else
	{
		// seems to provide 1 ms precision only
		GetSystemTimeAsFileTime( &ft );
	}
	ULARGE_INTEGER num;
	num.HighPart = ft.dwHighDateTime;
	num.LowPart = ft.dwLowDateTime;

	// difference between 1970-Jan-01 & 1601-Jan-01 in 100-nanosecond intervals
	const uint64_t shift = 116444736000000000ULL; // (27111902 << 32) + 3577643008
	uint64_t res = ( num.QuadPart - shift ) / 10;
	return res;	//in microsecs now
}
#endif

/*
================
Sys_GetSystemRam

	returns amount of physical memory in MB
================
*/
int Sys_GetSystemRam()
{
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof( statex );
	GlobalMemoryStatusEx( &statex );
	int physRam = statex.ullTotalPhys / ( 1024 * 1024 );
	// HACK: For some reason, ullTotalPhys is sometimes off by a meg or two, so we round up to the nearest 16 megs
	physRam = ( physRam + 8 ) & ~15;
	return physRam;
}


/*
================
Sys_GetDriveFreeSpace
returns in megabytes
================
*/
int Sys_GetDriveFreeSpace( const char* path )
{
	DWORDLONG lpFreeBytesAvailable;
	DWORDLONG lpTotalNumberOfBytes;
	DWORDLONG lpTotalNumberOfFreeBytes;
	int ret = 26;
	//FIXME: see why this is failing on some machines
	//CANNOTFIX: uses reduntdant code needs total rewrite for modern windows archs, hack used to return positive numbers on win7 but breaks completely on any arch above that.
	if( ::GetDiskFreeSpaceEx( path, ( PULARGE_INTEGER )&lpFreeBytesAvailable, ( PULARGE_INTEGER )&lpTotalNumberOfBytes, ( PULARGE_INTEGER )&lpTotalNumberOfFreeBytes ) )
	{
		ret = int( lpFreeBytesAvailable / ( 1024.0 * 1024.0 ) );
	}
	// force it to output positive numbers
	if( ret < 0 )
	{
		ret = -ret;
	}
	return abs( ret );
}

/*
========================
Sys_GetDriveFreeSpaceInBytes
========================
*/
int64 Sys_GetDriveFreeSpaceInBytes( const char* path )
{
	DWORDLONG lpFreeBytesAvailable;
	DWORDLONG lpTotalNumberOfBytes;
	DWORDLONG lpTotalNumberOfFreeBytes;
	int64 ret = 1;
	//FIXME: see why this is failing on some machines
	//CANNOTFIX: uses reduntdant code needs total rewrite for modern windows archs, hack used to return positive numbers on win7 but breaks completely on any arch above that.
	if( ::GetDiskFreeSpaceEx( path, ( PULARGE_INTEGER )&lpFreeBytesAvailable, ( PULARGE_INTEGER )&lpTotalNumberOfBytes, ( PULARGE_INTEGER )&lpTotalNumberOfFreeBytes ) )
	{
		ret = lpFreeBytesAvailable;
	}
	// force it to output positive numbers
	if( ret < 0 )
	{
		ret = -ret;
	}
	return ret;
}

/*
================
GetVolumeHandleForFile
================
*/
static HANDLE GetVolumeHandleForFile( const char* filePath )
{
	char volumePath[MAX_PATH];
	if( !GetVolumePathName( filePath, volumePath, ARRAYSIZE( volumePath ) ) )
	{
		return INVALID_HANDLE_VALUE;
	}

	char volumeName[MAX_PATH];
	if( !GetVolumeNameForVolumeMountPoint( volumePath, volumeName, ARRAYSIZE( volumeName ) ) )
	{
		return INVALID_HANDLE_VALUE;
	}

	size_t length = strlen( volumeName );
	if( length && volumeName[length - 1] == '\\' )
	{
		volumeName[length - 1] = '\0';
	}

	return CreateFile(
			   volumeName, 0,
			   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			   nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr
		   );
}

/*
================
Sys_IsFileOnHdd

Checks whether the disk containing the file incurs seeking penalty.
Taken from Raymond Chen blog: https://devblogs.microsoft.com/oldnewthing/20201023-00/?p=104395
================
*/
bool Sys_IsFileOnHdd( const char* filePath )
{
	HANDLE volume = GetVolumeHandleForFile( filePath );
	if( volume == INVALID_HANDLE_VALUE )
	{
		return false;	// supposedly happens for files on network
	}

	// note: if you get compile error here, make sure windows.h is included without WIN32_LEAN_AND_MEAN
	// Note that MFC headers define and require this define, so we can't use PCH in this cpp file
	STORAGE_PROPERTY_QUERY query{};
	query.PropertyId = StorageDeviceSeekPenaltyProperty;
	query.QueryType = PropertyStandardQuery;
	DWORD bytesWritten;
	DEVICE_SEEK_PENALTY_DESCRIPTOR result{};

	BOOL ok = DeviceIoControl(
				  volume, IOCTL_STORAGE_QUERY_PROPERTY,
				  &query, sizeof( query ),
				  &result, sizeof( result ),
				  &bytesWritten, nullptr
			  );
	CloseHandle( volume );

	if( ok )
	{
		return result.IncursSeekPenalty;
	}
	return true;	// supposedly happens for multi-disk volumes
}

/*
================
Sys_LockMemory
================
*/
bool Sys_LockMemory( void* ptr, int bytes )
{
	return ( VirtualLock( ptr, ( SIZE_T )bytes ) != FALSE );
}

/*
================
Sys_UnlockMemory
================
*/
bool Sys_UnlockMemory( void* ptr, int bytes )
{
	return ( VirtualUnlock( ptr, ( SIZE_T )bytes ) != FALSE );
}

/*
================
Sys_SetPhysicalWorkMemory
================
*/
void Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes )
{
	::SetProcessWorkingSetSize( GetCurrentProcess(), minBytes, maxBytes );
}