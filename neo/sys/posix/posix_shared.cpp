/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#if defined( __linux__ )
	#include <sys/stat.h>
	#include <sys/sysmacros.h>
	#include <unistd.h>
	#include <limits.h>
	#include <cstdio>
	#include <cstring>
#elif defined( __APPLE__ )
	#include <sys/mount.h>
	#include <cstring>

	#include <CoreFoundation/CoreFoundation.h>
	#include <IOKit/IOKitLib.h>
	#include <IOKit/storage/IOMedia.h>
	#include <IOKit/storage/IOBlockStorageDevice.h>
#endif

/*
================
Sys_IsFileOnHdd
================
*/
bool Sys_IsFileOnHdd( const char* filePath )
{
#if defined( __linux__ )
	struct stat st;
	if( stat( filePath, &st ) != 0 )
	{
		return false;
	}

	char sysPath[256];
	snprintf( sysPath, sizeof( sysPath ), "/sys/dev/block/%u:%u/queue/rotational",
			  major( st.st_dev ), minor( st.st_dev ) );

	FILE* f = fopen( sysPath, "r" );
	if( !f )
	{
		char linkPath[256];
		snprintf( linkPath, sizeof( linkPath ), "/sys/dev/block/%u:%u",
				  major( st.st_dev ), minor( st.st_dev ) );

		char resolved[PATH_MAX];
		ssize_t len = readlink( linkPath, resolved, sizeof( resolved ) - 1 );
		if( len <= 0 )
		{
			return true;
		}
		resolved[len] = '\0';

		char* lastSlash = strrchr( resolved, '/' );
		if( !lastSlash )
		{
			return true;
		}
		*lastSlash = '\0';

		lastSlash = strrchr( resolved, '/' );
		if( !lastSlash )
		{
			return true;
		}
		*lastSlash = '\0';

		char parentPath[300];
		snprintf( parentPath, sizeof( parentPath ), "/sys%s/queue/rotational", resolved );

		f = fopen( parentPath, "r" );
		if( !f )
		{
			return true;
		}
	}

	int rotational = 1;
	if( fscanf( f, "%d", &rotational ) != 1 )
	{
		rotational = 1;
	}
	fclose( f );

	return rotational != 0;
#elif defined( __APPLE__ )
	struct statfs fs;
	if( statfs( filePath, &fs ) != 0 )
	{
		return false;
	}

	const char* bsdName = fs.f_mntfromname;
	if( strncmp( bsdName, "/dev/", 5 ) == 0 )
	{
		bsdName += 5;
	}

	CFMutableDictionaryRef matching = IOBSDNameMatching( kIOMasterPortDefault, 0, bsdName );
	if( !matching )
	{
		return true;
	}

	io_service_t service = IOServiceGetMatchingService( kIOMasterPortDefault, matching );
	if( !service )
	{
		return true;
	}

	bool isHdd = true;	// default to rotational unless proven solid-state
	io_object_t current = service;
	IOObjectRetain( current );

	while( current )
	{
		CFTypeRef charDict = IORegistryEntryCreateCFProperty(
								 current, CFSTR( kIOPropertyDeviceCharacteristicsKey ),
								 kCFAllocatorDefault, 0 );

		if( charDict )
		{
			CFStringRef medium = ( CFStringRef )CFDictionaryGetValue(
									 ( CFDictionaryRef )charDict, CFSTR( kIOPropertyMediumTypeKey ) );

			if( medium && CFStringCompare( medium, CFSTR( kIOPropertyMediumTypeSolidStateKey ), 0 ) == kCFCompareEqualTo )
			{
				isHdd = false;
			}

			CFRelease( charDict );
			IOObjectRelease( current );
			break;
		}

		io_registry_entry_t parent = 0;
		if( IORegistryEntryGetParentEntry( current, kIOServicePlane, &parent ) != KERN_SUCCESS )
		{
			IOObjectRelease( current );
			break;
		}
		IOObjectRelease( current );
		current = parent;
	}

	IOObjectRelease( service );
	return isHdd;
#endif
}

/*
================
Sys_SetPhysicalWorkMemory
================
*/
void Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes )
{
}