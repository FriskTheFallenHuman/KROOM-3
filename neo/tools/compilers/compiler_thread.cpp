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
#include "compiler_resources.h"

idCompilerThread g_compilerThread;

// Dmap needs a large stack because BuildFaceTree_r / OptimizeIsland recurse deeply.
static const int COMPILER_STACK_SIZE = 8 * 1024 * 1024;   // 8 MB

/*
========================
EnsureCompilerThreadIdle
========================
*/
static bool EnsureCompilerThreadIdle()
{
	if( g_compilerThread.IsRunning() )
	{
		idLib::Printf( "Compiler already running.\n" );
		return false;
	}
	return true;
}

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
	bool alreadyRunning = false;
	{
		idScopedCriticalSection lock( stateMutex );
		if( running )
		{
			alreadyRunning = true;
		}
		else
		{
			WaitForThread();
			func = f;
			args = a;
			running = true;
			result = 0;
		}
	}

	if( alreadyRunning )
	{
		idLib::Warning( "CompilerThread: a compiler job is already running." );
		return false;
	}

	// StartThread fires Run() once and exits — unlike StartWorkerThread,
	// which loops waiting for SignalWork().
	if( !StartThread( threadName, CORE_ANY, THREAD_NORMAL, COMPILER_STACK_SIZE ) )
	{
		{
			idScopedCriticalSection lock( stateMutex );
			running = false;
			func = nullptr;
		}
		idLib::Warning( "CompilerThread: failed to start compiler job." );
		return false;
	}

	return true;
}

/*
========================
idCompilerThread::IsRunning
========================
*/
bool idCompilerThread::IsRunning() const
{
	idScopedCriticalSection lock( stateMutex );
	return running;
}

/*
========================
idCompilerThread::GetResult
========================
*/
int idCompilerThread::GetResult() const
{
	idScopedCriticalSection lock( stateMutex );
	return result;
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
	CompilerFunc_t job;
	idCmdArgs jobArgs;
	{
		idScopedCriticalSection lock( stateMutex );
		job = func;
		jobArgs = args;
	}

	if( job )
	{
		// SetRefreshOnPrint is main-thread only; skip it here.
		common->ClearWarnings( "compiler thread" );

		job( jobArgs );

		common->PrintWarnings();
	}

	{
		idScopedCriticalSection lock( stateMutex );
		running = false;
		func = nullptr;
	}
	return 0;
}

/*
========================
Dmap_Threaded_f
========================
*/
static void Dmap_Threaded_f( const idCmdArgs& args )
{
	if( !EnsureCompilerThreadIdle() )
	{
		return;
	}

	if( !DmapPreInit( args, true ) )
	{
		return;
	}

	if( !g_compilerThread.Dispatch( DmapInit_f, args, "DmapThread" ) )
	{
		DmapAborted();
	}
}

/*
========================
RunAAS_Threaded_f
========================
*/
static void RunAAS_Threaded_f( const idCmdArgs& args )
{
	if( !EnsureCompilerThreadIdle() )
	{
		return;
	}

	if( args.Argc() > 1 )
	{
		AASPreloadDefinitions();
		AASPreloadMapFile( args.Argv( args.Argc() - 1 ) );
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
	if( !EnsureCompilerThreadIdle() )
	{
		return;
	}

	if( args.Argc() > 1 )
	{
		AASPreloadDefinitions();
		AASPreloadMapDirectory( args.Argv( 1 ) );
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
	if( !EnsureCompilerThreadIdle() )
	{
		return;
	}

	if( args.Argc() > 1 )
	{
		AASPreloadDefinitions();
		AASPreloadMapFile( args.Argv( args.Argc() - 1 ) );
	}

	g_compilerThread.Dispatch( RunReach_f, args, "ReachThread" );
}

/*
========================
RunReachDir_Threaded_f
========================
*/
static void RunReachDir_Threaded_f( const idCmdArgs& args )
{
	if( !EnsureCompilerThreadIdle() )
	{
		return;
	}

	if( args.Argc() > 1 )
	{
		AASPreloadDefinitions();
		AASPreloadMapDirectory( args.Argv( 1 ) );
	}

	g_compilerThread.Dispatch( RunReachDir_f, args, "ReachDirThread" );
}

/*
========================
RoQFileEncode_Threaded_f
========================
*/
static void RoQFileEncode_Threaded_f( const idCmdArgs& args )
{
	if( !EnsureCompilerThreadIdle() )
	{
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
	if( !EnsureCompilerThreadIdle() )
	{
		return;
	}
	g_compilerThread.Dispatch( Amplitude_f, args, "AmplitudeThread" );
}

#if 0 // DEBUG stuff
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
#endif

/*
========================
RegisterCompilerThreadCommands
========================
*/
void RegisterCompilerThreadCommands()
{
	cmdSystem->AddCommand( "dmap", Dmap_Threaded_f, CMD_FL_TOOL, "compiles a map", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "dmapDir", DmapDir_f, CMD_FL_TOOL, "compiles maps in a folder", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runAAS", RunAAS_Threaded_f, CMD_FL_TOOL, "compiles an AAS file for a map", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runAASDir", RunAASDir_Threaded_f, CMD_FL_TOOL, "compiles AAS files for all maps in a folder", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runReach", RunReach_Threaded_f, CMD_FL_TOOL, "calculates reachability for an AAS file", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "runReachDir", RunReachDir_Threaded_f, CMD_FL_TOOL, "calculates reachability for maps in a folder", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "roq", RoQFileEncode_Threaded_f, CMD_FL_TOOL, "encodes a roq file" );
	cmdSystem->AddCommand( "amplitude", Amplitude_Threaded_f, CMD_FL_TOOL, "encodes a wav file into a amp file", idCmdSystem::ArgCompletion_SoundName );
#if 0 // DEBUG stuff
	cmdSystem->AddCommand( "compiler_wait", Compiler_Wait_f, CMD_FL_TOOL, "block until the background compiler finishes" );
	cmdSystem->AddCommand( "compiler_status", Compiler_Status_f, CMD_FL_TOOL, "print background compiler status" );
#endif
}
