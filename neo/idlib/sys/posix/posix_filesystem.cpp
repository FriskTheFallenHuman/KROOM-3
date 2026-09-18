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

#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <cstring>
#include <strings.h>
#include <stdio.h> // needed for sysconf()
#include <cstdio>

#if defined( __linux__ )
	#include <sys/sysmacros.h>
	#include <limits.h>
#elif defined( __APPLE__ )
	#include <sys/param.h>
	#include <sys/mount.h>
	#include <sys/sysctl.h>

	#include <mach/clock.h>
	#include <mach/clock_types.h>
	#include <mach/mach.h>
	#include <mach-o/dyld.h>
	#include <CoreFoundation/CoreFoundation.h>
	#include <IOKit/IOKitLib.h>
	#include <IOKit/storage/IOMedia.h>
	#include <IOKit/storage/IOBlockStorageDevice.h>
#endif

/*
==============
Sys_GetPath
==============
*/
bool Sys_GetPath( sysPath_t type, idStr& path )
{
	const char* s;
	char buf[MAX_OSPATH];
	struct stat st;

	path.Clear();

#if defined( __linux__ )
	switch( type )
	{
		case PATH_BASE:
			// try <path to executable>/base first
			if( Sys_GetPath( PATH_EXE, path ) )
			{
				path = path.StripFilename();

				// the path should have a base dir in it, otherwise it probably just contains the executable
				idStr testPath = path + "/" BASE_GAMEDIR;
				if( stat( testPath.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
				{
					idLib::Warning( "using path of executable: %s", path.c_str() );
					return true;
				}
			}

			// FIXME: Doom 3 BFG Edition never had a linux release.
			// fallback to vanilla doom3 install
			//if( stat( LINUX_DEFAULT_PATH, &st ) != -1 && S_ISDIR( st.st_mode ) )
			//{
			//	common->Warning( "using hardcoded default base path: " LINUX_DEFAULT_PATH );
			//
			//	path = LINUX_DEFAULT_PATH;
			//	return true;
			//}

			return false;

		case PATH_SAVE:
			s = getenv( "XDG_DATA_HOME" );
			if( s )
			{
				idStr::snPrintf( buf, sizeof( buf ), "%s/" SAVE_PATH, s );
			}
			else
			{
				s = getenv( "HOME" );
				if( !s )
				{
					idLib::Error( "Couldn't get HOME environment variable" );
					return false;
				}
				idStr::snPrintf( buf, sizeof( buf ), "%s/.local/share/" SAVE_PATH, s );
			}
			path = buf;
			return true;

		case PATH_EXE:
			ssize_t len = readlink( "/proc/self/exe", buf, sizeof( buf ) - 1 );

			if( len == -1 )
			{
				idStr linkpath;
				linkpath.Format( "/proc/%d/exe", getpid() );
				len = readlink( linkpath.c_str(), buf, sizeof( buf ) - 1 );
			}

			if( len == -1 )
			{
				idLib::Error( "Could not determine executable path via /proc." );
				return false;
			}

			buf[len] = '\0';
			path = buf;
			return true;
	}
#elif defined( __APPLE__ )
	switch( type )
	{
		case PATH_BASE:
			// try <path to executable>/base first
			if( Sys_GetPath( PATH_EXE, path ) )
			{
				path = path.StripFilename();

				// the path should have a base dir in it, otherwise it probably just contains the executable
				idStr testPath = path + "/" BASE_GAMEDIR;
				if( stat( testPath.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
				{
					idLib::Warning( "using path of executable: %s", path.c_str() );
					return true;
				}
			}

			// FIXME: Doom 3 BFG Edition never had a mac release.
			// fallback to vanilla doom3 install
			//if( stat( LINUX_DEFAULT_PATH, &st ) != -1 && S_ISDIR( st.st_mode ) )
			//{
			//	common->Warning( "using hardcoded default base path: " LINUX_DEFAULT_PATH );
			//
			//	path = LINUX_DEFAULT_PATH;
			//	return true;
			//}

			return false;

		case PATH_SAVE:
			s = getenv( "HOME" );
			if( !s )
			{
				idLib::Error( "Couldn't get HOME environment variable" );
				return false;
			}
			idStr::snPrintf( buf, sizeof( buf ), "%s/Library/Application Support/" SAVE_PATH, s );
			path = buf;
			return true;

		case PATH_EXE:
			char exePath[PATH_MAX];
			uint32_t size = sizeof( exePath );
			if( _NSGetExecutablePath( exePath, &size ) == 0 )
			{
				char resolved[PATH_MAX];
				if( realpath( exePath, resolved ) )
				{
					path = resolved;
					return true;
				}
			}
			return false;
	}
#endif

	return false;
}

/*
==============
Sys_ListFiles
==============
*/
int Sys_ListFiles( const char* directory, const char* extension, idStrList& list )
{
	list.Clear();

	if( !extension )
	{
		extension = "";
	}

	bool dirOnly = false;
	if( extension[0] == '/' && extension[1] == 0 )
	{
		extension = "";
		dirOnly = true;
	}

	DIR* dir = opendir( directory );
	if( dir == NULL )
	{
		return -1;
	}

	const size_t extLen = strlen( extension );

	struct dirent* entry;
	while( ( entry = readdir( dir ) ) != NULL )
	{
		if( extLen > 0 )
		{
			const size_t nameLen = strlen( entry->d_name );
			if( nameLen < extLen || strcasecmp( entry->d_name + nameLen - extLen, extension ) != 0 )
			{
				continue;
			}
		}

		char fullPath[MAX_OSPATH];
		idStr::snPrintf( fullPath, sizeof( fullPath ), "%s/%s", directory, entry->d_name );

		struct stat st;
		if( stat( fullPath, &st ) == -1 )
		{
			continue;
		}

		if( S_ISDIR( st.st_mode ) != dirOnly )
		{
			continue;
		}

		list.Append( entry->d_name );
	}

	closedir( dir );

	return list.Num();
}

/*
========================
Sys_IsFolder
========================
*/
sysFolder_t Sys_IsFolder( const char* path )
{
	struct stat buffer;
	if( stat( path, &buffer ) < 0 )
	{
		return FOLDER_ERROR;
	}
	return S_ISDIR( buffer.st_mode ) ? FOLDER_YES : FOLDER_NO;
}

/*
==============
Sys_Cwd
==============
*/
const char* Sys_Cwd()
{
	static char cwd[MAX_OSPATH];

	if( getcwd( cwd, sizeof( cwd ) - 1 ) == NULL )
	{
		cwd[0] = 0;
	}
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
	mkdir( path, 0777 );
}

/*
=================
Sys_FileTimeStamp
=================
*/
ID_TIME_T Sys_FileTimeStamp( const char* path )
{
	struct stat st;
	if( stat( path, &st ) == -1 )
	{
		return 0;
	}
	return st.st_mtime;
}

/*
========================
Sys_Rmdir
========================
*/
bool Sys_Rmdir( const char* path )
{
	return rmdir( path ) == 0;
}

/*
========================
Sys_IsFileWritable
========================
*/
bool Sys_IsFileWritable( const char* path )
{
	if( access( path, F_OK ) != 0 )
	{
		return true;
	}
	return access( path, W_OK ) == 0;
}

/*
================
Sys_IsFileOnHdd

Checks whether the disk containing the file incurs seeking penalty.
================
*/
bool Sys_IsFileOnHdd( const char* filePath )
{
#if defined( __linux__ )
	struct stat st;
	if( stat( filePath, &st ) == -1 )
	{
		return false;
	}

	const unsigned int devMajor = major( st.st_dev );
	const unsigned int devMinor = minor( st.st_dev );

	static const char* const formats[] =
	{
		"/sys/dev/block/%u:%u/queue/rotational",
		"/sys/dev/block/%u:%u/../queue/rotational",
	};

	for( int i = 0; i < 2; i++ )
	{
		char path[128];
		snprintf( path, sizeof( path ), formats[i], devMajor, devMinor );

		FILE* f = fopen( path, "r" );
		if( f == NULL )
		{
			continue;
		}

		const int c = fgetc( f );
		fclose( f );

		if( c == '0' || c == '1' )
		{
			return c == '1';
		}
	}

	return false;
#elif defined( __APPLE__ )
	struct statfs sfs;
	if( statfs( filePath, &sfs ) != 0 )
	{
		return false;
	}

	// network / virtual filesystems have no /dev node
	if( strncmp( sfs.f_mntfromname, "/dev/", 5 ) != 0 )
	{
		return false;
	}

	CFMutableDictionaryRef match = IOBSDNameMatching( 0, 0, sfs.f_mntfromname + 5 );
	if( match == NULL )
	{
		return false;
	}

	// consumes 'match'
	io_service_t service = IOServiceGetMatchingService( 0, match );
	if( service == IO_OBJECT_NULL )
	{
		return false;
	}

	bool rotational = false;

	CFTypeRef prop = IORegistryEntrySearchCFProperty( service, kIOServicePlane,
					 CFSTR( "Device Characteristics" ), kCFAllocatorDefault,
					 kIORegistryIterateRecursively | kIORegistryIterateParents );
	if( prop != NULL )
	{
		if( CFGetTypeID( prop ) == CFDictionaryGetTypeID() )
		{
			CFTypeRef medium = CFDictionaryGetValue( ( CFDictionaryRef )prop, CFSTR( "Medium Type" ) );
			if( medium != NULL && CFGetTypeID( medium ) == CFStringGetTypeID() )
			{
				rotational = ( CFStringCompare( ( CFStringRef )medium, CFSTR( "Rotational" ), 0 ) == kCFCompareEqualTo );
			}
		}
		CFRelease( prop );
	}

	IOObjectRelease( service );

	return rotational;
#endif
}