/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

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
#include "../posix/posix_public.h"
#include "../sys_local.h"
//#include "local.h"

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

// DG: needed for Sys_ReLaunch()
#include <dirent.h>

static const char** cmdargv = NULL;
static int cmdargc = 0;
// DG end

// RB begin
#include <stdio.h> // needed for sysconf()
#include <cstring>
// RB end

/*
==============
Sys_EXEPath
==============
*/
const char* Sys_EXEPath()
{
	static char	buf[ 1024 ];
	idStr		linkpath;
	int			len;

	buf[ 0 ] = '\0';
	sprintf( linkpath, "/proc/%d/exe", getpid() );
	len = readlink( linkpath.c_str(), buf, sizeof( buf ) );
	if( len == -1 )
	{
		Sys_Printf( "couldn't stat exe path link %s\n", linkpath.c_str() );
		// RB: fixed array subscript is below array bounds
		buf[ 0 ] = '\0';
		// RB end
	}
	return buf;
}


/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ( bool enable )
{
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ( bool enable )
{
}

/*
===============
main
===============
*/
int main( int argc, const char** argv )
{
	// DG: needed for Sys_ReLaunch()
	cmdargc = argc;
	cmdargv = argv;
	// DG end

	Posix_EarlyInit( );

	if( argc > 1 )
	{
		common->Init( argc - 1, &argv[1], NULL );
	}
	else
	{
		common->Init( 0, NULL, NULL );
	}

	Posix_LateInit( );


	while( 1 )
	{
		common->Frame();
	}
}
