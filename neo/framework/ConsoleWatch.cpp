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

#include "ConsoleWatch.h"

idList< idConsoleWatch > consoleWatchList;
idList< idStr > consoleWatchResults;

/*
==================
Con_Watch_f
==================
*/
static void Con_Watch_f( const idCmdArgs& args )
{
	if( args.Argc() < 2 )
	{
		common->Printf( "usage: con_watch <command> [x y]\n" );
		return;
	}

	idConsoleWatch watch( args.Argv( 1 ), args.Argc() > 2 ? atoi( args.Argv( 2 ) ) : -1,
						  args.Argc() > 3 ? atoi( args.Argv( 3 ) ) : -1 );
	for( int i = 0; i < consoleWatchList.Num(); ++i )
	{
		if( consoleWatchList[i].watchString.Icmp( watch.watchString ) == 0 )
		{
			consoleWatchList[i] = watch;
			return;
		}
	}
	consoleWatchList.Append( watch );
}

/*
==================
Con_Unwatch_f
==================
*/
static void Con_Unwatch_f( const idCmdArgs& args )
{
	if( args.Argc() < 2 || idStr::Icmp( args.Argv( 1 ), "all" ) == 0 )
	{
		consoleWatchList.Clear();
		return;
	}
	for( int i = 0; i < consoleWatchList.Num(); ++i )
	{
		if( consoleWatchList[i].watchString.Icmp( args.Argv( 1 ) ) == 0 )
		{
			consoleWatchList.RemoveIndex( i );
			return;
		}
	}
}

/*
==================
ConsoleWatch_Init
==================
*/
void ConsoleWatch_Init()
{
	cmdSystem->AddCommand( "con_watch", Con_Watch_f, CMD_FL_SYSTEM, "displays the printed output of a command every frame" );
	cmdSystem->AddCommand( "con_unwatch", Con_Unwatch_f, CMD_FL_SYSTEM, "removes a console watch; use 'all' to remove every watch" );
}

/*
==================
ConsoleWatch_Shutdown
==================
*/
void ConsoleWatch_Shutdown()
{
	cmdSystem->RemoveCommand( "con_watch" );
	cmdSystem->RemoveCommand( "con_unwatch" );
	consoleWatchList.Clear();
	consoleWatchResults.Clear();
}
