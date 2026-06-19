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

#include <string.h>
#include <errno.h>

const int siglist[] =
{
	SIGHUP,
	SIGQUIT,
	SIGILL,
	SIGTRAP,
	SIGIOT,
	SIGBUS,
	SIGFPE,
	SIGSEGV,
	SIGPIPE,
	SIGABRT,
	//	SIGTTIN,
	//	SIGTTOU,
	-1
};

const char* signames[] =
{
	"SIGHUP",
	"SIGQUIT",
	"SIGILL",
	"SIGTRAP",
	"SIGIOT",
	"SIGBUS",
	"SIGFPE",
	"SIGSEGV",
	"SIGPIPE",
	"SIGABRT",
	//	"SIGTTIN",
	//	"SIGTTOUT"
};

static char fatalError[ 1024 ];

/*
================
Sys_ClearSigs
================
*/
void Sys_ClearSigs()
{
	struct sigaction action;
	int i;

	/* Set up the structure */
	action.sa_handler = SIG_DFL;
	sigemptyset( &action.sa_mask );
	action.sa_flags = 0;

	i = 0;
	while( siglist[ i ] != -1 )
	{
		if( sigaction( siglist[ i ], &action, NULL ) != 0 )
		{
			idLib::Printf( "Failed to reset %s handler: %s\n", signames[ i ], strerror( errno ) );
		}
		i++;
	}
}

/*
================
sig_handler
================
*/
static void sig_handler( int signum, siginfo_t* info, void* context )
{
	static bool double_fault = false;

	if( double_fault )
	{
		idLib::Printf( "double fault %s, bailing out\n", strsignal( signum ) );
		//Posix_Exit( signum );
	}

	double_fault = true;

	// NOTE: see sigaction man page, could verbose the whole siginfo_t and print human readable si_code
	idLib::Printf( "signal caught: %s\nsi_code %d\n", strsignal( signum ), info->si_code );

	idLib::Printf( "Trying to exit gracefully..\n" );

	Sys_CaptureExceptionStack( info, context );

	idList<debugStackFrame_t, TAG_CRAP> frames;
	idDebugSystem::DecodeStack( g_crashStackData, g_crashStackLen, frames );
	idDebugSystem::CleanStack( frames );

	char stackStr[4096];
	idDebugSystem::StringifyStack( g_crashStackHash, frames.Ptr(), frames.Num(), stackStr, sizeof( stackStr ) );

	char summary[4096];
	idStr::snPrintf( summary, sizeof( summary ),
					 "KROOM 3: BFG has crashed.\n\nSignal: %s\nsi_code: %d\n\n%s",
					 strsignal( signum ), info->si_code, stackStr );

	sys->ShowCrashDialog( summary );

	//Posix_SetExit( signum );

	common->Quit();
}

/*
================
Sys_InitSigs
================
*/
void Sys_InitSigs()
{
	struct sigaction action;
	int i;

	/* Set up the structure */
	action.sa_sigaction = sig_handler;
	sigemptyset( &action.sa_mask );
	action.sa_flags = SA_SIGINFO | SA_NODEFER;

	i = 0;
	while( siglist[ i ] != -1 )
	{
		if( siglist[ i ] == SIGFPE )
		{
			action.sa_sigaction = Sys_FPE_handler;
			if( sigaction( siglist[ i ], &action, NULL ) != 0 )
			{
				idLib::Printf( "Failed to set SIGFPE handler: %s\n", strerror( errno ) );
			}
			action.sa_sigaction = sig_handler;
		}
		else if( sigaction( siglist[ i ], &action, NULL ) != 0 )
		{
			idLib::Printf( "Failed to set %s handler: %s\n", signames[ i ], strerror( errno ) );
		}
		i++;
	}

	// if the process is backgrounded (running non interactively)
	// then SIGTTIN or SIGTOU could be emitted, if not caught, turns into a SIGSTP
	signal( SIGTTIN, SIG_IGN );
	signal( SIGTTOU, SIG_IGN );
}