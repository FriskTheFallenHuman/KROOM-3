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

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#elif defined(__linux__)
	#include <sys/utsname.h>
	#include <fstream>
#elif defined(__APPLE__)
	#include <sys/types.h>
	#include <sys/sysctl.h>
#endif

#include "../sys_local.h"
#include "DeviceSDL.h"
#include "DeviceManagerLocal.h"
#include "DeviceConsole.h"

namespace fs = std::filesystem;

#ifndef _WIN32
	idCVar SDLVars_t::com_pid( "com_pid", "0", CVAR_INTEGER | CVAR_INIT | CVAR_SYSTEM, "process id" );
#endif
idCVar SDLVars_t::in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing" );
idCVar SDLVars_t::sdl_outputEditString( "sdl_outputEditString", "1", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar SDLVars_t::sdl_viewlog( "sdl_viewlog", "0", CVAR_SYSTEM | CVAR_INTEGER, "" );

SDLVars_t sdl = {};

/*
=============
Sys_Mkdir
=============
*/
void Sys_Mkdir( const char* path )
{
	std::error_code ec;
	fs::create_directory( path, ec );
}

/*
=================
Sys_FileTimeStamp
=================
*/
ID_TIME_T Sys_FileTimeStamp( const char* path )
{
	std::error_code ec;
	auto ftime = fs::last_write_time( path, ec );
	if( ec )
	{
		return 0;
	}
	using namespace std::chrono;
	auto sysTime = time_point_cast<system_clock::duration>(
					   ftime - fs::file_time_type::clock::now() + system_clock::now() );
	return static_cast<ID_TIME_T>( system_clock::to_time_t( sysTime ) );
}

/*
=============
Sys_Rmdir
=============
*/
bool Sys_Rmdir( const char* path )
{
	std::error_code ec;
	bool removed = fs::remove( path, ec );
	return removed && !ec;
}

/*
========================
Sys_IsFileWritable
========================
*/
bool Sys_IsFileWritable( const char* path )
{
	std::error_code ec;
	auto perms = fs::status( path, ec ).permissions();
	if( ec )
	{
		return true;
	}
	return ( perms & fs::perms::owner_write ) != fs::perms::none;
}

/*
========================
Sys_IsFolder
========================
*/
sysFolder_t Sys_IsFolder( const char* path )
{
	std::error_code ec;
	bool isDir = fs::is_directory( path, ec );
	if( ec )
	{
		return FOLDER_ERROR;
	}
	return isDir ? FOLDER_YES : FOLDER_NO;
}

/*
==============
Sys_Cwd
==============
*/
const char* Sys_Cwd()
{
	static std::string cwd;
	std::error_code ec;
	cwd = fs::current_path( ec ).string();
	if( ec )
	{
		cwd.clear();
	}
	if( cwd.size() >= MAX_OSPATH )
	{
		cwd.resize( MAX_OSPATH - 1 );
	}
	return cwd.c_str();
}

/*
==============
Sys_ListFiles
==============
*/
int Sys_ListFiles( const char* directory, const char* extension, idStrList& list )
{
	if( !extension )
	{
		extension = "";
	}

	bool dirsOnly = false;
	if( extension[0] == '/' && extension[1] == 0 )
	{
		extension = "";
		dirsOnly = true;
	}

	list.Clear();

	std::error_code ec;
	fs::directory_iterator it( directory, ec );
	if( ec )
	{
		return -1;
	}

	const size_t extLen = strlen( extension );

	for( const auto& entry : it )
	{
		std::error_code statEc;
		const bool isDir = entry.is_directory( statEc );
		if( statEc )
		{
			continue;
		}

		if( dirsOnly != isDir )
		{
			continue;
		}

		const std::string name = entry.path().filename().string();

		if( !dirsOnly && extLen > 0 )
		{
			if( name.size() < extLen ||
					idStr::Icmp( name.c_str() + ( name.size() - extLen ), extension ) != 0 )
			{
				continue;
			}
		}

		list.Append( name.c_str() );
	}

	return list.Num();
}


/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void Sys_Error( const char* error, ... )
{
	va_list		argptr;
	char		text[4096];

	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr );

	Sys_Printf( "%s\n", text );
	Sys_ShowConsole( 1, true );

	timerHiRes.Shutdown();

	Sys_ShutdownInput();

	if( idDeviceManager::GetInstance() )
	{
		idDeviceManager::GetInstance()->Shutdown();
	}

	extern idCVar com_productionMode;
	if( com_productionMode.GetInteger() == 0 )
	{
		// wait for the user to quit
		while( 1 )
		{
			SDL_Event ev;
			while( SDL_PollEvent( &ev ) )
			{
				if( deviceConsole.g_active )
				{
					Uint32 conWinID = SDL_GetWindowID( deviceConsole.g_window );
					if( deviceConsole.IsConsoleEvent( ev, conWinID ) )
					{
						SDL_PushEvent( &ev );
					}
				}
			}

			// HACK HACK
			if( deviceConsole.IsQuitRequested() )
			{
				common->Quit();
				break;
			}

			Sys_Sleep( 10 );
		}
	}

	Sys_DestroyConsole();

	exit( 1 );
}

/*
==============
Sys_Printf
==============
*/
#define MAXPRINTMSG 4096
void Sys_Printf( const char* fmt, ... )
{
	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, MAXPRINTMSG - 1, fmt, argptr );
	va_end( argptr );
	msg[sizeof( msg ) - 1] = '\0';

#ifdef _WIN32
	if( IsDebuggerPresent() )
	{
		OutputDebugString( msg );
	}
#else
	printf( "%s", msg );
#endif

	if( sdl.sdl_outputEditString.GetBool() && idLib::IsMainThread() )
	{
		deviceConsole.Print( msg );
	}
}

/*
==============
Sys_DebugPrintf
==============
*/
#define MAXPRINTMSG 4096
void Sys_DebugPrintf( const char* fmt, ... )
{
	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, MAXPRINTMSG - 1, fmt, argptr );
	msg[ sizeof( msg ) - 1 ] = '\0';
	va_end( argptr );

#ifdef _WIN32
	if( IsDebuggerPresent() )
	{
		OutputDebugString( msg );
	}
#else
	printf( "%s", msg );
#endif
}

/*
==============
Sys_DebugVPrintf
==============
*/
void Sys_DebugVPrintf( const char* fmt, va_list arg )
{
	char msg[MAXPRINTMSG];

	idStr::vsnPrintf( msg, MAXPRINTMSG - 1, fmt, arg );
	msg[ sizeof( msg ) - 1 ] = '\0';

#ifdef _WIN32
	if( IsDebuggerPresent() )
	{
		OutputDebugString( msg );
	}
#else
	printf( "%s", msg );
#endif
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit()
{
	timerHiRes.Shutdown();

	Sys_ShutdownInput();
	Sys_DestroyConsole();

#ifdef _WIN32
	ExitProcess( 0 );
#else
	exit( 0 );
#endif
}

/*
==============
Sys_Sleep
==============
*/
void Sys_Sleep( int msec )
{
	SDL_Delay( msec );
}

/*
==============
Sys_ShowWindow
==============
*/
void Sys_ShowWindow( bool show )
{
	if( show )
	{
		SDL_ShowWindow( sdl.window );
	}
	else
	{
		SDL_HideWindow( sdl.window );
	}
}

/*
==============
Sys_IsWindowVisible
==============
*/
bool Sys_IsWindowVisible()
{
	Uint32 flags = SDL_GetWindowFlags( sdl.window );
	return ( flags & SDL_WINDOW_SHOWN ) != 0;
}

/*
========================================================================

DLL Loading

========================================================================
*/


/*
=====================
Sys_DLL_Load
=====================
*/
uintptr_t Sys_DLL_Load( const char* dllName )
{
	void* handle = SDL_LoadObject( dllName );
	if( !handle )
	{
		const char* err = SDL_GetError();
		if( err && *err )
		{
			common->Warning( "SDL_LoadObject(\"%s\") failed: %s", dllName, err );
		}
		else
		{
			common->Warning( "SDL_LoadObject(\"%s\") failed without additional info.", dllName );
		}
		return 0;
	}

	return ( uintptr_t )handle;
}

/*
=====================
Sys_DLL_GetProcAddress
=====================
*/
void* Sys_DLL_GetProcAddress( uintptr_t dllHandle, const char* procName )
{
	void* adr = SDL_LoadFunction( ( void* )dllHandle, procName );
	if( !adr )
	{
		const char* err = SDL_GetError();
		if( err && *err )
		{
			common->Warning( "SDL_LoadFunction(%p, \"%s\") failed: %s", ( void* )dllHandle, procName, err );
		}
		else
		{
			common->Warning( "SDL_LoadFunction(%p, \"%s\") failed.", ( void* )dllHandle, procName );
		}
	}
	return adr;
}

/*
=====================
Sys_DLL_Unload
=====================
*/
void Sys_DLL_Unload( uintptr_t dllHandle )
{
	if( !dllHandle )
	{
		return;
	}

	SDL_UnloadObject( ( void* )dllHandle );
}

/*
=====================
Sys_GetClipboardData
=====================
*/
char* Sys_GetClipboardData()
{
	char* txt = SDL_GetClipboardText();

	if( txt == NULL )
	{
		return NULL;
	}
	else if( txt[0] == '\0' )
	{
		SDL_free( txt );
		return NULL;
	}

	char* ret = Mem_CopyString( txt );
	SDL_free( txt );
	return ret;
}

/*
=====================
Sys_SetClipboardData
=====================
*/
void Sys_SetClipboardData( const char* string )
{
	SDL_SetClipboardText( string );
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
static void Sys_In_Restart_f( const idCmdArgs& args )
{
	Sys_ShutdownInput();
	Sys_InitInput();
}

/*
=================
Sys_PrintOSInfo
=================
*/
static void Sys_PrintOSInfo()
{
	const char* platform = SDL_GetPlatform();
	idStr osString = platform ? platform : "Unknown";

#if defined(_WIN32)
	OSVERSIONINFOEX osvi = { sizeof( osvi ) };
	if( GetVersionEx( ( LPOSVERSIONINFO )&osvi ) )
	{
		osString = va( "Windows %u.%u (Build %u)", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber );
	}
#elif defined(__linux__)
	//  uname for kernel version
	struct utsname unameData;
	if( uname( &unameData ) == 0 )
	{
		osString = va( "Linux (kernel %s)", unameData.release );
	}

	// /etc/os-release for distro name
	std::ifstream osRelease( "/etc/os-release" );
	if( osRelease.is_open() )
	{
		std::string line;
		while( std::getline( osRelease, line ) )
		{
			if( line.find( "PRETTY_NAME=" ) == 0 )
			{
				size_t start = line.find( '"' );
				size_t end = line.rfind( '"' );
				if( start != std::string::npos && end != std::string::npos && start < end )
				{
					osString = va( "%s (kernel %s)", line.substr( start + 1, end - start - 1 ).c_str(), unameData.release );
				}
				break;
			}
		}
	}
#elif defined(__APPLE__)
	// use sysctl for version
	char version[256];
	size_t size = sizeof( version );
	if( sysctlbyname( "kern.osproductversion", version, &size, NULL, 0 ) == 0 )
	{
		osString = va( "macOS %s", version );
	}
	else
	{
		// fallback to Darwin kernel version
		if( sysctlbyname( "kern.version", version, &size, NULL, 0 ) == 0 )
		{
			osString = va( "Darwin (%s)", version );
		}
	}
#endif

	common->Printf( "Detected: %s\n", osString.c_str() );
}

/*
================
Sys_Init

The cvar system must already be setup
================
*/
void Sys_Init()
{
	int cpuFlags = Sys_GetProcessorId();

#ifdef _WIN32
	CoInitialize( NULL );
#endif

	cmdSystem->AddCommand( "in_restart", Sys_In_Restart_f, CMD_FL_SYSTEM, "restarts the input system" );

	//
	// OS Info
	//
	Sys_PrintOSInfo();

	//
	// CPU type
	//
	common->Printf( "CPU Name: %s\n", Sys_GetProcessorString() );
	common->Printf( "%1.0f CPU MHz ", Sys_ClockTicksPerSecond() / 1000000.0f );

	common->Printf( "%d MB System Memory\n", Sys_GetSystemRam() );
	if( ( cpuFlags & CPUID_SSE ) == 0 )
	{
		common->Error( "SSE not supported!" );
	}

	sdl.g_Joystick.Init();
}

/*
================
Sys_Shutdown
================
*/
void Sys_Shutdown()
{
#ifdef _WIN32
	CoUninitialize();
#else
	Sys_ClearSigs();
#endif
}

/*
====================
Sys_Frame
====================
*/
static void Sys_Frame()
{
	// if "viewlog" has been modified, show or hide the log console
	if( sdl.sdl_viewlog.IsModified() )
	{
		sdl.sdl_viewlog.ClearModified();
		Sys_ShowConsole( sdl.sdl_viewlog.GetInteger() ? 1 : 0, true );
	}

	if( deviceConsole.IsQuitRequested() )
	{
		common->Quit();
	}
}

/*
==================
main
==================
*/
int main( int argc, char* argv[] )
{
#ifdef _WIN32
	::SetCursor( NULL );

	Sys_SetPhysicalWorkMemory( 192 << 20, 1024 << 20 );
#endif

	// combine the args into a windows-style command line
	sys_cmdline[0] = 0;
	for( int i = 1 ; i < argc ; i++ )
	{
		strcat( sys_cmdline, argv[i] );
		if( i < argc - 1 )
		{
			strcat( sys_cmdline, " " );
		}
	}

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

#ifdef _WIN32
	// Register the unhandled exception
	LONG WINAPI Sys_UnhandledExceptionFilter( EXCEPTION_POINTERS * exceptionInfo );
	SetUnhandledExceptionFilter( Sys_UnhandledExceptionFilter );

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );
#else
	Sys_InitSigs();
#endif

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution, may be different in other
	// platforms but it should roughly the same.
	timerHiRes.Init();

	// get the initial time base
	Sys_Milliseconds();

	static idDeviceManagerSDL deviceMgr;
	idDeviceManager::s_instance = &deviceMgr;

	common->Init( 0, NULL, sys_cmdline );

#ifndef _WIN32
	SDLVars_t::com_pid.SetInteger( getpid() );
	idLib::Printf( "pid: %d\n", SDLVars_t::com_pid.GetInteger() );
#endif

	// hide or show the early console as necessary
	if( sdl.sdl_viewlog.GetInteger() )
	{
		Sys_ShowConsole( 1, true );
	}
	else
	{
		Sys_ShowConsole( 0, false );
	}

	// main game loop
	while( 1 )
	{
		Sys_Frame();

		// run the game
		common->Frame();
	}

	// never gets here
	return 0;
}
