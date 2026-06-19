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

uint8 g_crashStackData[4096];
int g_crashStackLen = 0;
uint32 g_crashStackHash = 0;

/*
====================
Sys_CaptureExceptionStack
====================
*/
void Sys_CaptureExceptionStack( siginfo_t* info, void* context )
{
	g_crashStackLen = sizeof( g_crashStackData );
	Sys_CaptureStackTrace( 2, g_crashStackData, g_crashStackLen );
	g_crashStackHash = idStr::Hash( ( char* )g_crashStackData, g_crashStackLen );
	if( g_crashStackHash == 0 )
	{
		g_crashStackHash = ( uint32 ) - 1;
	}
}