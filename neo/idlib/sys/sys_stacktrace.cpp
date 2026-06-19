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

#include <cpptrace/cpptrace.hpp>

uint8 g_crashStackData[4096];
int g_crashStackLen = 0;
uint32 g_crashStackHash = 0;

static cpptrace::raw_trace g_lastRawTrace;

/*
====================
Sys_CaptureStackTrace
====================
*/
void Sys_CaptureStackTrace( int ignoreFrames, uint8* data, int& len )
{
	cpptrace::raw_trace trace = cpptrace::generate_raw_trace( ignoreFrames );

	const int maxFrames = len / sizeof( void* );
	const int count = idMath::ClampInt( 0, maxFrames, ( int )trace.frames.size() );

	void** addresses = ( void** )data;
	for( int i = 0; i < count; i++ )
	{
		addresses[i] = ( void* )trace.frames[i];
	}
	len = count * sizeof( void* );

	g_lastRawTrace = std::move( trace );
}

/*
====================
Sys_GetStackTraceFramesCount
====================
*/
int Sys_GetStackTraceFramesCount( uint8* data, int len )
{
	return len / sizeof( void* );
}

/*
====================
Sys_DecodeStackTrace
====================
*/
void Sys_DecodeStackTrace( uint8* data, int len, debugStackFrame_t* frames )
{
	int framesCount = Sys_GetStackTraceFramesCount( data, len );
	memset( frames, 0, framesCount * sizeof( frames[0] ) );

	cpptrace::stacktrace resolved = g_lastRawTrace.resolve();
	int count = idMath::ClampInt( 0, framesCount, ( int )resolved.frames.size() );

	for( int i = 0; i < count; i++ )
	{
		const cpptrace::stacktrace_frame& f = resolved.frames[i];
		frames[i].pointer = ( void* )f.raw_address;
		idStr::Copynz( frames[i].functionName, f.symbol.c_str(), sizeof( frames[0].functionName ) );
		idStr::Copynz( frames[i].fileName, f.filename.c_str(), sizeof( frames[0].fileName ) );
		frames[i].lineNumber = f.line.has_value() ? static_cast<int>( f.line.value() ) : 0;
	}
}