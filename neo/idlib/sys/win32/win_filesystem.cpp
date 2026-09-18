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

#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <mapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

#ifndef __MRC__
	#include <sys/types.h>
	#include <sys/stat.h>
#endif

/*
==============
WPath2A
==============
*/
static int WPath2A( char* dst, size_t size, const WCHAR* src )
{
	int len;
	BOOL default_char = FALSE;

	// test if we can convert lossless
	len = WideCharToMultiByte( CP_ACP, 0, src, -1, dst, size, NULL, &default_char );

	if( default_char )
	{
		// The following lines implement a horrible
		// hack to connect the UTF-16 WinAPI to the
		// ASCII doom3 strings. While this should work in
		// most cases, it'll fail if the "Windows to
		// DOS filename translation" is switched off.
		// In that case the function will return NULL
		// and no homedir is used.
		WCHAR w[MAX_OSPATH];
		len = GetShortPathNameW( src, w, sizeof( w ) );

		if( len == 0 )
		{
			return 0;
		}

		/* Since the DOS path contains no UTF-16 characters, convert it to the system's default code page */
		len = WideCharToMultiByte( CP_ACP, 0, w, len, dst, size - 1, NULL, NULL );
	}

	if( len == 0 )
	{
		return 0;
	}

	dst[len] = 0;

	// Replace backslashes by slashes
	for( int i = 0; i < len; ++i )
	{
		if( dst[i] == '\\' )
		{
			dst[i] = '/';
		}
	}

	// cut trailing slash
	if( dst[len - 1] == '/' )
	{
		dst[len - 1] = 0;
		len--;
	}

	return len;
}

/*
==============
GetHomeDir
==============
*/
static int GetHomeDir( char* dst, size_t size )
{
	int len;
	WCHAR profile[MAX_OSPATH];

	// Get the path to "My Documents" directory
	SHGetFolderPathW( NULL, CSIDL_PERSONAL, NULL, 0, profile );

	len = WPath2A( dst, size, profile );
	if( len == 0 )
	{
		return 0;
	}

	idStr::Append( dst, size, SAVE_PATH );

	return len;
}

/*
==============
GetRegistryPath
==============
*/
static int GetRegistryPath( char* dst, size_t size, const WCHAR* subkey, const WCHAR* name )
{
	WCHAR w[MAX_OSPATH];
	DWORD len = sizeof( w );
	HKEY res;
	DWORD sam = KEY_QUERY_VALUE
#ifdef _WIN64
				| KEY_WOW64_32KEY
#endif
				;
	DWORD type;

	if( RegOpenKeyExW( HKEY_LOCAL_MACHINE, subkey, 0, sam, &res ) != ERROR_SUCCESS )
	{
		return 0;
	}

	if( RegQueryValueExW( res, name, NULL, &type, ( LPBYTE )w, &len ) != ERROR_SUCCESS )
	{
		RegCloseKey( res );
		return 0;
	}

	RegCloseKey( res );

	if( type != REG_SZ )
	{
		return 0;
	}

	return WPath2A( dst, size, w );
}

/*
==============
Sys_GetPath

Returns "My Documents"/My Games/dhewm3 directory (or equivalent - "CSIDL_PERSONAL").
To be used with Sys_GetPath(PATH_SAVE), so savegames, screenshots etc will be
saved to the users files instead of systemwide.

Based on (with kind permission) Yamagi Quake II's Sys_GetHomeDir()

Returns the number of characters written to dst
==============
*/
bool Sys_GetPath( sysPath_t type, idStr& path )
{
	char buf[MAX_OSPATH];
	struct _stat st;
	idStr s;

	switch( type )
	{
		case PATH_BASE:
			// try <path to exe>/base first
			if( Sys_GetPath( PATH_EXE, path ) )
			{
				path.StripFilename();

				s = path;
				s.AppendPath( BASE_GAMEDIR );
				if( _stat( s.c_str(), &st ) != -1 && ( st.st_mode & _S_IFDIR ) )
				{
					//idLib::Warning( "using path of executable: %s", path.c_str() );
					return true;
				}

				idLib::Warning( "base path '%s' does not exist", s.c_str() );
			}

			// fallback to steam doom3-bfg install
			if( GetRegistryPath( buf, sizeof( buf ), L"SOFTWARE\\Valve\\Steam", L"InstallPath" ) > 0 )
			{
				path = buf;
				path.AppendPath( "steamapps\\common\\DOOM 3 BFG Edition" );

				if( _stat( path.c_str(), &st ) != -1 && st.st_mode & _S_IFDIR )
				{
					return true;
				}
			}

			idLib::Warning( "vanilla doom3-bfg path not found either" );

			return false;

		case PATH_SAVE:
			if( GetHomeDir( buf, sizeof( buf ) ) < 1 )
			{
				idLib::Error( "Couldn't get dir to home path" );
				return false;
			}

			path = buf;
			return true;

		case PATH_EXE:
			GetModuleFileName( NULL, buf, sizeof( buf ) - 1 );
			path = buf;
			path.BackSlashesToSlashes();
			return true;
	}

	return false;
}

/*
==============
Sys_ListFiles
==============
*/
int Sys_ListFiles( const char* directory, const char* extension, idStrList& list )
{
	idStr		search;
	struct _finddata_t findinfo;
	intptr_t	findhandle;

	int			flag;

	if( !extension )
	{
		extension = "";
	}

	// passing a slash as extension will find directories
	if( extension[0] == '/' && extension[1] == 0 )
	{
		extension = "";
		flag = 0;
	}
	else
	{
		flag = _A_SUBDIR;
	}

	sprintf( search, "%s\\*%s", directory, extension );

	// search
	list.Clear();

	findhandle = _findfirst( search, &findinfo );
	if( findhandle == -1 )
	{
		return -1;
	}

	do
	{
		if( flag ^ ( findinfo.attrib & _A_SUBDIR ) )
		{
			list.Append( findinfo.name );
		}
	}
	while( _findnext( findhandle, &findinfo ) != -1 );

	_findclose( findhandle );

	return list.Num();
}

/*
========================
Sys_IsFolder
========================
*/
sysFolder_t Sys_IsFolder( const char* path )
{
	struct _stat buffer;
	if( _stat( path, &buffer ) < 0 )
	{
		return FOLDER_ERROR;
	}
	return ( buffer.st_mode & _S_IFDIR ) != 0 ? FOLDER_YES : FOLDER_NO;
}

/*
==============
Sys_Cwd
==============
*/
const char* Sys_Cwd()
{
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir( const char* path )
{
	_mkdir( path );
}

/*
=================
Sys_FileTimeStamp
=================
*/
static ID_TIME_T Sys_FileTimeStamp( idFileHandle fp )
{
	FILETIME writeTime;
	GetFileTime( fp, NULL, NULL, &writeTime );

	/*
		FILETIME = number of 100-nanosecond ticks since midnight
		1 Jan 1601 UTC. time_t = number of 1-second ticks since
		midnight 1 Jan 1970 UTC. To translate, we subtract a
		FILETIME representation of midnight, 1 Jan 1970 from the
		time in question and divide by the number of 100-ns ticks
		in one second.
	*/

	SYSTEMTIME base_st =
	{
		1970,   // wYear
		1,      // wMonth
		0,      // wDayOfWeek
		1,      // wDay
		0,      // wHour
		0,      // wMinute
		0,      // wSecond
		0       // wMilliseconds
	};

	FILETIME base_ft;
	SystemTimeToFileTime( &base_st, &base_ft );

	LARGE_INTEGER itime;
	itime.QuadPart = reinterpret_cast<LARGE_INTEGER&>( writeTime ).QuadPart;
	itime.QuadPart -= reinterpret_cast<LARGE_INTEGER&>( base_ft ).QuadPart;
	itime.QuadPart /= 10000000LL;
	return itime.QuadPart;
}

/*
=================
Sys_FileTimeStamp
=================
*/
ID_TIME_T Sys_FileTimeStamp( const char* path )
{
	idFileHandle fp = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
								   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( fp == INVALID_HANDLE_VALUE )
	{
		return 0;
	}

	const ID_TIME_T timestamp = Sys_FileTimeStamp( fp );
	CloseHandle( fp );
	return timestamp;
}

/*
========================
Sys_Rmdir
========================
*/
bool Sys_Rmdir( const char* path )
{
	return _rmdir( path ) == 0;
}

/*
========================
Sys_IsFileWritable
========================
*/
bool Sys_IsFileWritable( const char* path )
{
	struct _stat st;
	if( _stat( path, &st ) == -1 )
	{
		return true;
	}
	return ( st.st_mode & S_IWRITE ) != 0;
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