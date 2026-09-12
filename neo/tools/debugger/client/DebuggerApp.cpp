/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software
Copyright (C) 2021 Harrie van Ginneken

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

#include "DebuggerApp.h"

#ifndef _MSC_VER
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	#if defined(__MINGW32__) || defined(__MINGW64__)
		// MingW
		#include <process.h>
		#include <direct.h>
		#include <errno.h>
	#else
		// POSIX (Linux, macOS, *nix)
		#include <unistd.h>
		#include <sys/types.h>
		#include <sys/wait.h>
		#include <limits.h>
		#include <errno.h>
	#endif
#endif

rvDebuggerApp	gDebuggerApp;

/*
============================================

rvDebuggerMainApp

============================================
*/

/*
================
rvDebuggerMainApp::OnInit
================
*/
bool rvDebuggerMainApp::OnInit()
{
	if( !wxApp::OnInit() )
	{
		return false;
	}

	return true;
}

wxIMPLEMENT_APP_NO_MAIN( rvDebuggerMainApp );

/*
============================================

rvDebuggerApp

============================================
*/

/*
================
rvDebuggerApp::rvDebuggerApp
================
*/
rvDebuggerApp::rvDebuggerApp()
{
	mOptions.Init( "debugger.cfg", "Debugger" );

	mDebuggerWindow = NULL;
	mIsInitialized  = false;
}

/*
================
rvDebuggerApp::~rvDebuggerApp
================
*/
rvDebuggerApp::~rvDebuggerApp()
{
	if( mIsInitialized )
	{
		wxEntryCleanup();
	}
}

/*
================
rvDebuggerApp::Initialize

Initializes the debugger application by creating the debugger window
================
*/
bool rvDebuggerApp::Initialize()
{
	int argc = 0;
	char** argv = NULL;

	if( !wxEntryStart( argc, argv ) )
	{
		return false;
	}

	if( !wxTheApp || !wxTheApp->CallOnInit() )
	{
		return false;
	}

	mIsInitialized = true;

	mOptions.Load();

	mDebuggerWindow = new rvDebuggerWindow();
	if( !mDebuggerWindow->Create() )
	{
		delete mDebuggerWindow;
		mDebuggerWindow = NULL;
		return false;
	}

	// Initialize the network connection for the debugger
	if( !mClient.Initialize() )
	{
		return false;
	}

	mNetworkPollTimer = new wxTimer();
	mNetworkPollTimer->Bind( wxEVT_TIMER, &rvDebuggerApp::OnNetworkPollTimer, this );
	mNetworkPollTimer->Start( 15 );

	return true;
}

/*
================
rvDebuggerApp::OnNetworkPollTimer
================
*/
void rvDebuggerApp::OnNetworkPollTimer( wxTimerEvent& event )
{
	mClient.ProcessMessages();
}

/*
================
rvDebuggerApp::ProcessWindowMessages

Process windows messages
================
*/
bool rvDebuggerApp::ProcessWindowMessages()
{
	if( !wxTheApp )
	{
		return false;
	}

	while( wxTheApp->Pending() )
	{
		wxTheApp->Dispatch();
	}

	return !wxTheApp->IsMainLoopRunning();
}

/*
================
rvDebuggerApp::NotifyWindowDestroyed
================
*/
void rvDebuggerApp::NotifyWindowDestroyed( rvDebuggerWindow* wnd )
{
	if( mDebuggerWindow == wnd )
	{
		mDebuggerWindow = NULL;
	}
}

/*
================
rvDebuggerApp::Run

Main Loop for the debugger application
================
*/
int rvDebuggerApp::Run()
{
	int result = wxTheApp->OnRun();

	mNetworkPollTimer->Stop();
	delete mNetworkPollTimer;

	mClient.Shutdown();
	mOptions.Save();

	if( mDebuggerWindow )
	{
		mDebuggerWindow->Destroy();
		mDebuggerWindow = NULL;
	}

	return result;
}

/*
================
DebuggerMain

Main entry point for the debugger application
================
*/
void DebuggerClientInit( const char* cmdline )
{
	// See if the debugger is already running
	if( rvDebuggerWindow::Activate() )
	{
		goto DebuggerClientInitDone;
	}

	if( !gDebuggerApp.Initialize() )
	{
		goto DebuggerClientInitDone;
	}

	// hide the doom window by default
	Sys_ShowWindow( false );

	gDebuggerApp.Run();

DebuggerClientInitDone:

	common->Quit();
}

/*
================
DebuggerLaunch

Launches another instance of the running executable with +debugger appended
to the end to indicate that the debugger should start up.
================
*/
void DebuggerClientLaunch()
{
	if( renderSystem->IsFullScreen() )
	{
		common->Printf( "Cannot run the script debugger in fullscreen mode.\n"
						"Set r_vidfullscreen to 0 and vid_restart.\n" );
		return;
	}

	// See if the debugger is already running
	if( rvDebuggerWindow::Activate() )
	{
		return;
	}

#ifdef _MSC_VER
	char exeFile[MAX_PATH];
	char curDir[MAX_PATH];
	char cmdLine[2048];

	STARTUPINFO         startup;
	PROCESS_INFORMATION process;

	ZeroMemory( &startup, sizeof( startup ) );
	startup.cb = sizeof( startup );

	GetCurrentDirectory( MAX_PATH, curDir );
	GetModuleFileName( NULL, exeFile, MAX_PATH );

	_snprintf_s( cmdLine, sizeof( cmdLine ), _TRUNCATE,
				 "\"%s\" +set com_skipIntroVideos 1 +set com_skipLegalScreens 1"
				 " +set fs_game %s +set com_smp 0 +debugger",
				 exeFile, cvarSystem->GetCVarString( "fs_game" ) );

	BOOL ok = CreateProcess( NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, curDir, &startup, &process );
	if( !ok )
	{
		DWORD err = GetLastError();
		common->Printf( "DebuggerClientLaunch: CreateProcess failed (err=%lu), cmd='%s'\n",
						err, cmdLine );
		return;
	}

	CloseHandle( process.hThread );
	CloseHandle( process.hProcess );
#else // MinGW

#if defined(__MINGW32__) || defined(__MINGW64__)
	{
		char exeFile[MAX_PATH];
		char curDir[MAX_PATH];

		// Get the current directory
		if( _getcwd( curDir, MAX_PATH ) == NULL )
		{
			fprintf( stderr, "getcwd failed: %d\n", errno );
			curDir[0] = '\0';
		}

#ifdef __MINGW32__
		if( __argc > 0 && __argv[0] )
		{
			strncpy( exeFile, __argv[0], MAX_PATH - 1 );
			exeFile[MAX_PATH - 1] = '\0';
		}
		else
		{
			assert( !"MinGW what the fuck??" );
		}
#else
		assert( !"MinGW what the fuck??" );
#endif

		// Build launch arguments
		const char* fs_game = cvarSystem->GetCVarString( "fs_game" );
		const char* argv_spawn[] =
		{
			exeFile,
			"+set", "com_skipIntroVideos", "1",
			"+set", "com_skipLegalScreens", "1",
			"+set", "fs_game", fs_game ? fs_game : "",
			"+set", "com_enableDebuggerServer", "1",
			NULL
		};

		int pid = _spawnv( _P_NOWAIT, exeFile, ( const char* const* )argv_spawn );
		if( pid == -1 )
		{
			perror( "_spawnv" );
		}
	}

#else // POSIX (Linux, macOS, *nix)
	{
		char exeFile[PATH_MAX];
		char curDir[PATH_MAX];
		ssize_t len = 0;
		int have_exe = 0;

		// Get the current directory
		if( getcwd( curDir, sizeof( curDir ) ) == NULL )
		{
			fprintf( stderr, "getcwd failed: %s\n", strerror( errno ) );
			curDir[0] = '\0';
		}

#if defined(__linux__)
		// probe /proc/self/exe
		len = readlink( "/proc/self/exe", exeFile, sizeof( exeFile ) - 1 );
		if( len > 0 )
		{
			exeFile[len] = '\0';
			have_exe = 1;
		}
#elif defined(__APPLE__)
		// use _NSGetExecutablePath
#include <mach-o/dyld.h>
		uint32_t size = sizeof( exeFile );
		if( _NSGetExecutablePath( exeFile, &size ) == 0 )
		{
			have_exe = 1;
		}
#endif

		if( !have_exe )
		{
			assert( !"This shouldn't happen" );
		}

		// Build launch arguments
		const char* fs_game = cvarSystem->GetCVarString( "fs_game" );
		char* argv_exec[12];
		int ai = 0;
		argv_exec[ai++] = exeFile;
		argv_exec[ai++] = ( char* )"+set";
		argv_exec[ai++] = ( char* )"com_skipIntroVideos";
		argv_exec[ai++] = ( char* )"1";
		argv_exec[ai++] = ( char* )"+set";
		argv_exec[ai++] = ( char* )"com_skipLegalScreens";
		argv_exec[ai++] = ( char* )"1";
		argv_exec[ai++] = ( char* )"+set";
		argv_exec[ai++] = ( char* )"fs_game";
		argv_exec[ai++] = ( char* )( fs_game ? fs_game : "" );
		argv_exec[ai++] = ( char* )"+set";
		argv_exec[ai++] = ( char* )"com_enableDebuggerServer";
		argv_exec[ai++] = ( char* )"1";
		argv_exec[ai] = NULL;

		pid_t pid = fork();
		if( pid < 0 )
		{
			fprintf( stderr, "fork failed: %s\n", strerror( errno ) );
		}
		else if( pid == 0 )
		{
			// Set the working directory then execute
			if( curDir[0] != '\0' )
			{
				if( chdir( curDir ) != 0 )
				{
					fprintf( stderr, "child chdir(%s) failed: %s\n", curDir, strerror( errno ) );
				}
			}

			execv( exeFile, argv_exec );

			fprintf( stderr, "execv(%s) failed: %s\n", exeFile, strerror( errno ) );
			_exit( 127 );
		}
	}

#endif // MinGW/POSIX/Linux

#endif // _MSC_VER
}
