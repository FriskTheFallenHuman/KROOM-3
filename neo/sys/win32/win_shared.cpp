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

#undef StrCmpN
#undef StrCmpNI
#undef StrCmpI

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
Sys_SetPhysicalWorkMemory
================
*/
void Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes )
{
	::SetProcessWorkingSetSize( GetCurrentProcess(), minBytes, maxBytes );
}