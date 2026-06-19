/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

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

#include "../../idlib/precompiled.h"
//#include "../sys_local.h"

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <sys/sysctl.h>
#include <mach/clock.h>
#include <mach/clock_types.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>

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

	return false;
}