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

#if defined( ID_ALLOW_TOOLS )
	#include "DebuggerServer.h"
	#include "../client/DebuggerApp.h"
#else
	#include "DebuggerServer.h"
#endif

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL2/SDL.h>

static rvDebuggerServer*		gDebuggerServer			= NULL;
static SDL_Thread*				gDebuggerServerThread   = NULL;
static bool						gDebuggerServerQuit     = false;

/*
================
DebuggerServerThread

Thread proc for the debugger server
================
*/
static int SDLCALL DebuggerServerThread( void* param )
{
	assert( gDebuggerServer );

	while( !gDebuggerServerQuit )
	{
		gDebuggerServer->ProcessMessages();
		SDL_Delay( 1 );
	}

	return 0;
}

/*
================
DebuggerServerInit

Starts up the debugger server
================
*/
bool DebuggerServerInit()
{
	com_enableDebuggerServer.ClearModified();

	// Dont do this if we are in the debugger already
	if( gDebuggerServer != NULL || ( com_editors & EDITOR_DEBUGGER ) )
	{
		return false;
	}

	// Allocate the new debugger server
	gDebuggerServer = new rvDebuggerServer;
	if( !gDebuggerServer )
	{
		return false;
	}

	// Initialize the debugger server
	if( !gDebuggerServer->Initialize() )
	{
		delete gDebuggerServer;
		gDebuggerServer = NULL;
		return false;
	}

	// Start the debugger server thread
	gDebuggerServerThread = SDL_CreateThread( DebuggerServerThread, "DebuggerServer", NULL );

	return true;
}

/*
================
DebuggerServerShutdown

Shuts down the debugger server
================
*/
void DebuggerServerShutdown()
{
	if( gDebuggerServerThread != NULL )
	{
		// Signal the debugger server to quit
		gDebuggerServerQuit = true;

		// Wait for the thread to finish
		SDL_WaitThread( gDebuggerServerThread, NULL );
		gDebuggerServerThread = NULL;

		// Shutdown the server now
		gDebuggerServer->Shutdown();

		delete gDebuggerServer;
		gDebuggerServer = NULL;

		com_editors &= ~EDITOR_DEBUGGER;
	}

	com_enableDebuggerServer.ClearModified();
}

/*
================
DebuggerServerCheckBreakpoint

Check to see if there is a breakpoint associtated with this statement
================
*/
void DebuggerServerCheckBreakpoint( idInterpreter* interpreter, idProgram* program, int instructionPointer )
{
	if( !gDebuggerServer )
	{
		return;
	}

	gDebuggerServer->CheckBreakpoints( interpreter, program, instructionPointer );
}

/*
================
DebuggerServerPrint

Sends a print message to the debugger client
================
*/
void DebuggerServerPrint( const char* text )
{
	if( !gDebuggerServer )
	{
		return;
	}

	gDebuggerServer->Print( text );
}
