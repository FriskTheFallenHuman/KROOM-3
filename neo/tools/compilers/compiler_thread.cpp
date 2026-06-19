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

#include "compiler_thread.h"
#include "compiler_public.h"

idCompilerThread g_compilerThread;

// Dmap needs a large stack because BuildFaceTree_r / OptimizeIsland recurse deeply.
static const int COMPILER_STACK_SIZE = 8 * 1024 * 1024;   // 8 MB

/*
========================
idCompilerThread::idCompilerThread
========================
*/
idCompilerThread::idCompilerThread()
	: func( nullptr )
	, running( false )
	, result( 0 )
{
}

/*
========================
idCompilerThread::~idCompilerThread
========================
*/
idCompilerThread::~idCompilerThread()
{
	WaitForCompletion();
	StopThread();
}

/*
========================
idCompilerThread::Dispatch
========================
*/
bool idCompilerThread::Dispatch( CompilerFunc_t f, const idCmdArgs& a, const char* threadName )
{
	if( running )
	{
		idLib::Warning( "CompilerThread: a compiler job is already running." );
		return false;
	}

	func = f;
	args = a;
	running = true;
	result  = 0;

	// StartThread fires Run() once and exits — unlike StartWorkerThread,
	// which loops waiting for SignalWork().
	StartThread( threadName, CORE_ANY, THREAD_NORMAL, COMPILER_STACK_SIZE );
	return true;
}

/*
========================
idCompilerThread::IsRunning
========================
*/
bool idCompilerThread::IsRunning() const
{
	return running;
}

/*
========================
idCompilerThread::WaitForCompletion
========================
*/
void idCompilerThread::WaitForCompletion()
{
	WaitForThread();
}

/*
========================
idCompilerThread::Run
========================
*/
int idCompilerThread::Run()
{
	if( func )
	{
		// SetRefreshOnPrint is main-thread only; skip it here.
		common->ClearWarnings( "compiler thread" );

		func( args );

		common->PrintWarnings();
	}

	running = false;
	return 0;
}

/*
========================
Dmap_Threaded_f
========================
*/
static void Dmap_Threaded_f( const idCmdArgs& args )
{
	if( g_compilerThread.IsRunning() )
	{
		idLib::Printf( "Compiler already running. Use 'compiler_wait' to block.\n" );
		return;
	}
	g_compilerThread.Dispatch( Dmap_f, args, "DmapThread" );
}

/*
========================
RunAAS_Threaded_f
========================
*/
static void RunAAS_Threaded_f( const idCmdArgs& args )
{
	if( g_compilerThread.IsRunning() )
	{
		idLib::Printf( "Compiler already running.\n" );
		return;
	}
	g_compilerThread.Dispatch( RunAAS_f, args, "AASThread" );
}

/*
========================
RunAASDir_Threaded_f
========================
*/
static void RunAASDir_Threaded_f( const idCmdArgs& args )
{
	if( g_compilerThread.IsRunning() )
	{
		idLib::Printf( "Compiler already running.\n" );
		return;
	}
	g_compilerThread.Dispatch( RunAASDir_f, args, "AASDirThread" );
}

/*
========================
RunReach_Threaded_f
========================
*/
static void RunReach_Threaded_f( const idCmdArgs& args )
{
	if( g_compilerThread.IsRunning() )
	{
		idLib::Printf( "Compiler already running.\n" );
		return;
	}
	g_compilerThread.Dispatch( RunReach_f, args, "ReachThread" );
}

/*
========================
RoQFileEncode_Threaded_f
========================
*/
static void RoQFileEncode_Threaded_f( const idCmdArgs& args )
{
	if( g_compilerThread.IsRunning() )
	{
		idLib::Printf( "Compiler already running.\n" );
		return;
	}
	g_compilerThread.Dispatch( RoQFileEncode_f, args, "RoQThread" );
}

/*
========================
Amplitude_Threaded_f
========================
*/

static void Amplitude_Threaded_f( const idCmdArgs& args )
{
	if( g_compilerThread.IsRunning() )
	{
		idLib::Printf( "Compiler already running.\n" );
		return;
	}
	g_compilerThread.Dispatch( Amplitude_f, args, "AmplitudeThread" );
}
/*
========================
Compiler_Wait_f

Block the main thread until the background compiler finishes.
Useful in automation scripts.
========================
*/
static void Compiler_Wait_f( const idCmdArgs& args )
{
	if( !g_compilerThread.IsRunning() )
	{
		idLib::Printf( "No compiler job running.\n" );
		return;
	}
	idLib::Printf( "Waiting for compiler to finish...\n" );
	g_compilerThread.WaitForCompletion();
	idLib::Printf( "Compiler finished.\n" );
}

/*
========================
Compiler_Status_f
========================
*/
static void Compiler_Status_f( const idCmdArgs& args )
{
	idLib::Printf( "Compiler thread: %s\n", g_compilerThread.IsRunning() ? "RUNNING" : "idle" );
}

/*
========================
RegisterCompilerThreadCommands
========================
*/
void RegisterCompilerThreadCommands()
{
	cmdSystem->AddCommand( "dmap", Dmap_Threaded_f, CMD_FL_TOOL, "compiles a map", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runAAS", RunAAS_Threaded_f, CMD_FL_TOOL, "compiles an AAS file for a map", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runAASDir", RunAASDir_Threaded_f, CMD_FL_TOOL, "compiles AAS files for all maps in a folder", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runReach", RunReach_Threaded_f, CMD_FL_TOOL, "calculates reachability for an AAS file", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "roq", RoQFileEncode_Threaded_f, CMD_FL_TOOL, "encodes a roq file" );
	cmdSystem->AddCommand( "amplitude", Amplitude_Threaded_f, CMD_FL_TOOL, "encodes a wav file into a amp file", idCmdSystem::ArgCompletion_SoundName );
	cmdSystem->AddCommand( "compiler_wait", Compiler_Wait_f, CMD_FL_TOOL, "block until the background compiler finishes" );
	cmdSystem->AddCommand( "compiler_status", Compiler_Status_f, CMD_FL_TOOL, "print background compiler status" );
}
