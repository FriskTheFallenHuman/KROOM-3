/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/
#include "precompiled.h"
#include "../sys_local.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <termios.h>
#include <signal.h>
#include <fcntl.h>
#include <fnmatch.h>

// RB begin
#if defined(__ANDROID__)
	#include <android/log.h>
#endif
// RB end

#include "posix_public.h"

#define					COMMAND_HISTORY 64

static int				input_hide = 0;

idEditField				input_field;
static char				input_ret[256];

static idStr			history[ COMMAND_HISTORY ];	// cycle buffer
static int				history_count = 0;			// buffer fill up
static int				history_start = 0;			// current history start
static int				history_current = 0;			// goes back in history
idEditField				history_backup;				// the base edit line

// terminal support
idCVar in_tty( "in_tty", "1", CVAR_BOOL | CVAR_INIT | CVAR_SYSTEM, "terminal tab-completion and history" );

static bool				tty_enabled = false;
static struct termios	tty_tc;

// pid - useful when you attach to gdb..
idCVar com_pid( "com_pid", "0", CVAR_INTEGER | CVAR_INIT | CVAR_SYSTEM, "process id" );

// exit - quit - error --------------------------------------------------------

static int set_exit = 0;
static char exit_spawn[ 1024 ];

/*
================
Posix_Exit
================
*/
void Posix_Exit( int ret )
{
	if( tty_enabled )
	{
		Sys_Printf( "shutdown terminal support\n" );
		if( tcsetattr( 0, TCSADRAIN, &tty_tc ) == -1 )
		{
			Sys_Printf( "tcsetattr failed: %s\n", strerror( errno ) );
		}
	}
	// at this point, too late to catch signals
	Posix_ClearSigs();
	if( asyncThread.threadHandle )
	{
		Sys_DestroyThread( asyncThread );
	}
	// process spawning. it's best when it happens after everything has shut down
	if( exit_spawn[0] )
	{
		Sys_DoStartProcess( exit_spawn, false );
	}
	// in case of signal, handler tries a common->Quit
	// we use set_exit to maintain a correct exit code
	if( set_exit )
	{
		exit( set_exit );
	}
	exit( ret );
}

/*
================
Posix_SetExit
================
*/
void Posix_SetExit( int ret )
{
	set_exit = 0;
}

/*
===============
Posix_SetExitSpawn
set the process to be spawned when we quit
===============
*/
void Posix_SetExitSpawn( const char* exeName )
{
	idStr::Copynz( exit_spawn, exeName, 1024 );
}

/*
==================
idSysLocal::StartProcess
if !quit, start the process asap
otherwise, push it for execution at exit
(i.e. let complete shutdown of the game and freeing of resources happen)
NOTE: might even want to add a small delay?
==================
*/
void idSysLocal::StartProcess( const char* exeName, bool quit )
{
	if( quit )
	{
		common->DPrintf( "Sys_StartProcess %s (delaying until final exit)\n", exeName );
		Posix_SetExitSpawn( exeName );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
		return;
	}

	common->DPrintf( "Sys_StartProcess %s\n", exeName );
	Sys_DoStartProcess( exeName );
}

/*
================
Sys_Quit
================
*/
void Sys_Quit()
{
	Posix_Exit( EXIT_SUCCESS );
}

/*
================
Sys_Milliseconds
================
*/
/* base time in seconds, that's our origin
   timeval:tv_sec is an int:
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
   using unsigned long data type to work right with Sys_XTimeToSysTime */

#ifdef CLOCK_MONOTONIC_RAW
	// use RAW monotonic clock if available (=> not subject to NTP etc)
	#define D3_CLOCK_TO_USE CLOCK_MONOTONIC_RAW
#else
	#define D3_CLOCK_TO_USE CLOCK_MONOTONIC
#endif

// RB: changed long to int
unsigned int sys_timeBase = 0;
// RB end
/* current time in ms, using sys_timeBase as origin
   NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
	 0x7fffffff ms - ~24 days
		 or is it 48 days? the specs say int, but maybe it's casted from unsigned int?
*/
int Sys_Milliseconds()
{
	// DG: use clock_gettime on all platforms
#if 1
	int curtime;
	struct timespec ts;

	clock_gettime( D3_CLOCK_TO_USE, &ts );

	if( !sys_timeBase )
	{
		sys_timeBase = ts.tv_sec;
		return ts.tv_nsec / 1000000;
	}

	curtime = ( ts.tv_sec - sys_timeBase ) * 1000 + ts.tv_nsec / 1000000;

	return curtime;
#else
	// gettimeofday() implementation
	int curtime;
	struct timeval tp;

	gettimeofday( &tp, NULL );

	if( !sys_timeBase )
	{
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec / 1000;
	}

	curtime = ( tp.tv_sec - sys_timeBase ) * 1000 + tp.tv_usec / 1000;

	return curtime;
	* /
#endif
	// DG end
}

// RB: added for BFG

/*
================
Sys_Microseconds
================
*/
static uint64 sys_microTimeBase = 0;

uint64 Sys_Microseconds()
{
#if 0
	static uint64 ticksPerMicrosecondTimes1024 = 0;

	if( ticksPerMicrosecondTimes1024 == 0 )
	{
		ticksPerMicrosecondTimes1024 = ( ( uint64 )Sys_ClockTicksPerSecond() << 10 ) / 1000000;
		assert( ticksPerMicrosecondTimes1024 > 0 );
	}

	return ( ( uint64 )( ( int64 )Sys_GetClockTicks() << 10 ) ) / ticksPerMicrosecondTimes1024;
#elif 0
	uint64 curtime;
	struct timespec ts;

	clock_gettime( CLOCK_MONOTONIC, &ts );

	curtime = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

	return curtime;
#else
	uint64 curtime;
	struct timespec ts;

	clock_gettime( D3_CLOCK_TO_USE, &ts );

	if( !sys_microTimeBase )
	{
		sys_microTimeBase = ts.tv_sec;
		return ts.tv_nsec / 1000;
	}

	curtime = ( ts.tv_sec - sys_microTimeBase ) * 1000000 + ts.tv_nsec / 1000;

	return curtime;
#endif
}

/*
================
Sys_Mkdir
================
*/
void Sys_Mkdir( const char* path )
{
	mkdir( path, 0777 );
}

/*
==========
Sys_IsFile
==========
*/
bool Sys_IsFile( const char* path )
{
	assert( path );

	struct stat st;
	if( stat( path, &st ) != -1 && S_ISREG( st.st_mode ) )
	{
		return true;
	}

	return false;
}

/*
===============
Sys_IsDirectory
===============
*/
bool Sys_IsDirectory( const char* path )
{
	assert( path );

	struct stat st;
	if( stat( path, &st ) != -1 && S_ISDIR( st.st_mode ) )
	{
		return true;
	}

	return false;
}

/*
================
Sys_ListFiles
================
*/
int Sys_ListFiles( const char* directory, const char* extension, idStrList& list )
{
	struct dirent* d;
	DIR* fdir;
	bool dironly = false;
	char search[MAX_OSPATH];
	struct stat st;
	bool debug;

	list.Clear();

	debug = cvarSystem->GetCVarBool( "fs_debug" );
	// DG: we use fnmatch for shell-style pattern matching
	// so the pattern should at least contain "*" to match everything,
	// the extension will be added behind that (if !dironly)
	idStr pattern( "*" );

	// passing a slash as extension will find directories
	if( extension[0] == '/' && extension[1] == 0 )
	{
		dironly = true;
	}
	else
	{
		// so we have *<extension>, the same as in the windows code basically
		pattern += extension;
	}
	// DG end

	// NOTE: case sensitivity of directory path can screw us up here
	if( ( fdir = opendir( directory ) ) == NULL )
	{
		if( debug )
		{
			common->Printf( "Sys_ListFiles: opendir %s failed\n", directory );
		}
		return -1;
	}

	// DG: use readdir_r instead of readdir for thread safety
	// the following lines are from the readdir_r manpage.. fscking ugly.
	int nameMax = pathconf( directory, _PC_NAME_MAX );
	if( nameMax == -1 )
	{
		nameMax = 255;
	}
	int direntLen = offsetof( struct dirent, d_name ) + nameMax + 1;

	struct dirent* entry = ( struct dirent* )Mem_Alloc( direntLen, TAG_CRAP );

	if( entry == NULL )
	{
		common->Warning( "Sys_ListFiles: Mem_Alloc for entry failed!" );
		closedir( fdir );
		return 0;
	}

	while( readdir_r( fdir, entry, &d ) == 0 && d != NULL )
	{
		// DG end
		idStr::snPrintf( search, sizeof( search ), "%s/%s", directory, d->d_name );
		if( stat( search, &st ) == -1 )
		{
			continue;
		}
		if( !dironly )
		{
			// DG: the original code didn't work because d3 bfg abuses the extension
			// to match whole filenames and patterns in the savegame-code, not just file extensions...
			// so just use fnmatch() which supports matching shell wildcard patterns ("*.foo" etc)
			// if we should ever need case insensitivity, use FNM_CASEFOLD as third flag
			if( fnmatch( pattern.c_str(), d->d_name, 0 ) != 0 )
			{
				continue;
			}
			// DG end
		}
		if( ( dironly && !( st.st_mode & S_IFDIR ) ) ||
				( !dironly && ( st.st_mode & S_IFDIR ) ) )
		{
			continue;
		}

		list.Append( d->d_name );
	}

	closedir( fdir );
	Mem_Free( entry );

	if( debug )
	{
		common->Printf( "Sys_ListFiles: %d entries in %s\n", list.Num(), directory );
	}

	return list.Num();
}

/*
============================================================================
EVENT LOOP
============================================================================
*/

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

static sysEvent_t eventQue[MAX_QUED_EVENTS];
static int eventHead, eventTail;

/*
================
Posix_QueEvent

ptr should either be null, or point to a block of data that can be freed later
================
*/
void Posix_QueEvent( sysEventType_t type, int value, int value2,
					 int ptrLength, void* ptr )
{
	sysEvent_t* ev;

	ev = &eventQue[eventHead & MASK_QUED_EVENTS];
	if( eventHead - eventTail >= MAX_QUED_EVENTS )
	{
		common->Printf( "Posix_QueEvent: overflow\n" );
		// we are discarding an event, but don't leak memory
		// TTimo: verbose dropped event types?
		if( ev->evPtr )
		{
			Mem_Free( ev->evPtr );
			ev->evPtr = NULL;
		}
		eventTail++;
	}

	eventHead++;

	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;

#if 0
	common->Printf( "Event %d: %d %d\n", ev->evType, ev->evValue, ev->evValue2 );
#endif
}

/*
================
Sys_GetEvent
================
*/
#if !defined(USE_SDL)
sysEvent_t Sys_GetEvent()
{
	static sysEvent_t ev;

	// return if we have data
	if( eventHead > eventTail )
	{
		eventTail++;
		return eventQue[( eventTail - 1 ) & MASK_QUED_EVENTS];
	}
	// return the empty event with the current time
	memset( &ev, 0, sizeof( ev ) );

	return ev;
}
#endif

/*
================
Sys_ClearEvents
================
*/
#if !defined(USE_SDL)
void Sys_ClearEvents()
{
	eventHead = eventTail = 0;
}
#endif

/*
================
Posix_Cwd
================
*/
const char* Posix_Cwd()
{
	static char cwd[MAX_OSPATH];

	getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

/*
=================
Sys_Init
Posix_EarlyInit/Posix_LateInit is better
=================
*/
void Sys_Init() { }

/*
=================
Posix_Shutdown
=================
*/
void Posix_Shutdown()
{
	for( int i = 0; i < COMMAND_HISTORY; i++ )
	{
		history[ i ].Clear();
	}
}

/*
=================
Sys_DLL_Load
TODO: OSX - use the native API instead? NSModule
=================
*/
uintptr_t Sys_DLL_Load( const char* path )
{
	void* ret = dlopen( path, RTLD_NOW );
	if( ret == NULL )
	{
		// dlopen() failed - this might be ok (we tried one possible path and the next will work)
		// or it might be worth warning about (the lib existed but still couldn't be loaded,
		// maybe a missing symbol or permission problems)
		// unfortunately we only get a string from dlerror(), not some distinctive error code..
		// so use try to open() the file to get a better idea what went wrong

		int fd = open( path, O_RDONLY );
		if( fd < 0 )  // couldn't open file for reading either
		{
			int e = errno;
			if( e != ENOENT )
			{
				// it didn't fail because the file doesn't exist - log it, might be interesting (=> likely permission problem)
				common->Warning( "Failed to load lib '%s'! Reason: %s ( %s )\n", path, dlerror(), strerror( e ) );
			}
		}
		else
		{
			// file could be opened, so it exists => log just dlerror()
			close( fd );
			common->Warning( "Failed to load lib '%s' even though it exists and is readable! Reason: %s\n", path, dlerror() );
		}
	}
	return ( uintptr_t )ret;
}

/*
=================
Sys_DLL_GetProcAddress
=================
*/
// RB: 64 bit fixes, changed int to intptr_t
void* Sys_DLL_GetProcAddress( intptr_t handle, const char* sym )
{
// RB end
	const char* error;
	void* ret = dlsym( ( void* )handle, sym );
	if( ( error = dlerror() ) != NULL )
	{
		Sys_Printf( "dlsym '%s' failed: %s\n", sym, error );
	}
	return ret;
}

/*
=================
Sys_DLL_Unload
=================
*/
// RB: 64 bit fixes, changed int to intptr_t
void Sys_DLL_Unload( intptr_t handle )
{
// RB end
	dlclose( ( void* )handle );
}

/*
================
Sys_ShowConsole
================
*/
void Sys_ShowConsole( int visLevel, bool quitOnClose ) { }

// ---------------------------------------------------------------------------

ID_TIME_T Sys_FileTimeStamp( FILE* fp )
{
	struct stat st;
	fstat( fileno( fp ), &st );
	return st.st_mtime;
}

void Sys_Sleep( int msec )
{
#if 0 // DG: I don't really care, this spams the console (and on windows this case isn't handled either)
	// Furthermore, there are several Sys_Sleep( 10 ) calls throughout the code
	if( msec < 20 )
	{
		static int last = 0;
		int now = Sys_Milliseconds();
		if( now - last > 1000 )
		{
			Sys_Printf( "WARNING: Sys_Sleep - %d < 20 msec is not portable\n", msec );
			last = now;
		}
		// ignore that sleep call, keep going
		return;
	}
#endif // DG end
	// use nanosleep? keep sleeping if signal interrupt?

	// RB begin
#if defined(__ANDROID__)
	usleep( msec * 1000 );
#else
	if( usleep( msec * 1000 ) == -1 )
	{
		Sys_Printf( "usleep: %s\n", strerror( errno ) );
	}
#endif
}

char* Sys_GetClipboardData()
{
	Sys_Printf( "TODO: Sys_GetClipboardData\n" );
	return NULL;
}

void Sys_SetClipboardData( const char* string )
{
	Sys_Printf( "TODO: Sys_SetClipboardData\n" );
}

void Sys_FPU_SetPrecision()
{
}

/*
================
Sys_LockMemory
================
*/
bool Sys_LockMemory( void* ptr, int bytes )
{
	return true;
}

/*
================
Sys_UnlockMemory
================
*/
bool Sys_UnlockMemory( void* ptr, int bytes )
{
	return true;
}

/*
================
Sys_SetPhysicalWorkMemory
================
*/
void Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes )
{
	common->DPrintf( "TODO: Sys_SetPhysicalWorkMemory\n" );
}

/*
===========
Sys_GetDriveFreeSpace
return in MegaBytes
===========
*/
int Sys_GetDriveFreeSpace( const char* path )
{
	common->DPrintf( "TODO: Sys_GetDriveFreeSpace\n" );
	return 1000 * 1024;
}

/*
===============
Posix_EarlyInit
===============
*/
void Posix_EarlyInit()
{
	memset( &asyncThread, 0, sizeof( asyncThread ) );
	exit_spawn[0] = '\0';
	Posix_InitSigs();
	// set the base time
	Sys_Milliseconds();
	Posix_InitPThreads();
}

/*
===============
Posix_LateInit
===============
*/
void Posix_LateInit()
{
	Posix_InitConsoleInput();
	com_pid.SetInteger( getpid() );
	common->Printf( "pid: %d\n", com_pid.GetInteger() );
	common->Printf( "%d MB System Memory\n", Sys_GetSystemRam() );

	// RB begin
#if !defined(USE_SDL_ASYNC)
	Posix_StartAsyncThread( );
#endif
	// RB end
}

/*
===============
Posix_InitConsoleInput
===============
*/
void Posix_InitConsoleInput()
{
	struct termios tc;

	if( in_tty.GetBool() )
	{
		if( isatty( STDIN_FILENO ) != 1 )
		{
			Sys_Printf( "terminal support disabled: stdin is not a tty\n" );
			in_tty.SetBool( false );
			return;
		}
		if( tcgetattr( 0, &tty_tc ) == -1 )
		{
			Sys_Printf( "tcgetattr failed. disabling terminal support: %s\n", strerror( errno ) );
			in_tty.SetBool( false );
			return;
		}
		// make the input non blocking
		if( fcntl( STDIN_FILENO, F_SETFL, fcntl( STDIN_FILENO, F_GETFL, 0 ) | O_NONBLOCK ) == -1 )
		{
			Sys_Printf( "fcntl STDIN non blocking failed.  disabling terminal support: %s\n", strerror( errno ) );
			in_tty.SetBool( false );
			return;
		}
		tc = tty_tc;
		/*
		  ECHO: don't echo input characters
		  ICANON: enable canonical mode.  This  enables  the  special
			characters  EOF,  EOL,  EOL2, ERASE, KILL, REPRINT,
			STATUS, and WERASE, and buffers by lines.
		  ISIG: when any of the characters  INTR,  QUIT,  SUSP,  or
			DSUSP are received, generate the corresponding signal
		*/
		tc.c_lflag &= ~( ECHO | ICANON );
		/*
		  ISTRIP strip off bit 8
		  INPCK enable input parity checking
		*/
		tc.c_iflag &= ~( ISTRIP | INPCK );
		tc.c_cc[VMIN] = 1;
		tc.c_cc[VTIME] = 0;
		if( tcsetattr( 0, TCSADRAIN, &tc ) == -1 )
		{
			Sys_Printf( "tcsetattr failed: %s\n", strerror( errno ) );
			Sys_Printf( "terminal support may not work correctly. Use +set in_tty 0 to disable it\n" );
		}
#if 0
		// make the output non blocking
		if( fcntl( STDOUT_FILENO, F_SETFL, fcntl( STDOUT_FILENO, F_GETFL, 0 ) | O_NONBLOCK ) == -1 )
		{
			Sys_Printf( "fcntl STDOUT non blocking failed: %s\n", strerror( errno ) );
		}
#endif
		tty_enabled = true;
		// check the terminal type for the supported ones
		char* term = getenv( "TERM" );
		if( term )
		{
			if( strcmp( term, "linux" ) && strcmp( term, "xterm" ) && strcmp( term, "xterm-color" ) && strcmp( term, "screen" ) )
			{
				Sys_Printf( "WARNING: terminal type '%s' is unknown. terminal support may not work correctly\n", term );
			}
		}
		Sys_Printf( "terminal support enabled ( use +set in_tty 0 to disabled )\n" );
	}
	else
	{
		Sys_Printf( "terminal support disabled\n" );
	}
}

/*
================
terminal support utilities
================
*/

void tty_Del()
{
	char key;
	key = '\b';
	write( STDOUT_FILENO, &key, 1 );
	key = ' ';
	write( STDOUT_FILENO, &key, 1 );
	key = '\b';
	write( STDOUT_FILENO, &key, 1 );
}

void tty_Left()
{
	char key = '\b';
	write( STDOUT_FILENO, &key, 1 );
}

void tty_Right()
{
	char key = 27;
	write( STDOUT_FILENO, &key, 1 );
	write( STDOUT_FILENO, "[C", 2 );
}

// clear the display of the line currently edited
// bring cursor back to beginning of line
void tty_Hide()
{
	int len, buf_len;
	if( !tty_enabled )
	{
		return;
	}
	if( input_hide )
	{
		input_hide++;
		return;
	}
	// clear after cursor
	len = strlen( input_field.GetBuffer() ) - input_field.GetCursor();
	while( len > 0 )
	{
		tty_Right();
		len--;
	}
	buf_len = strlen( input_field.GetBuffer() );
	while( buf_len > 0 )
	{
		tty_Del();
		buf_len--;
	}
	input_hide++;
}

// show the current line
void tty_Show()
{
	//	int i;
	if( !tty_enabled )
	{
		return;
	}
	assert( input_hide > 0 );
	input_hide--;
	if( input_hide == 0 )
	{
		char* buf = input_field.GetBuffer();
		if( buf[0] )
		{
			write( STDOUT_FILENO, buf, strlen( buf ) );

			// RB begin
#if defined(__ANDROID__)
			//__android_log_print(ANDROID_LOG_DEBUG, "Tekuum_DEBUG", "%s", buf);
#endif
			// RB end

			int back = strlen( buf ) - input_field.GetCursor();
			while( back > 0 )
			{
				tty_Left();
				back--;
			}
		}
	}
}

void tty_FlushIn()
{
	char key;
	while( read( 0, &key, 1 ) != -1 )
	{
		Sys_Printf( "'%d' ", key );
	}
	Sys_Printf( "\n" );
}

/*
================
Posix_ConsoleInput
Checks for a complete line of text typed in at the console.
Return NULL if a complete line is not ready.
================
*/
char* Posix_ConsoleInput()
{
	if( tty_enabled )
	{
		int		ret;
		char	key;
		bool	hidden = false;
		while( ( ret = read( STDIN_FILENO, &key, 1 ) ) > 0 )
		{
			if( !hidden )
			{
				tty_Hide();
				hidden = true;
			}
			switch( key )
			{
				case 1:
					input_field.SetCursor( 0 );
					break;
				case 5:
					input_field.SetCursor( strlen( input_field.GetBuffer() ) );
					break;
				case 127:
				case 8:
					input_field.CharEvent( K_BACKSPACE );
					break;
				case '\n':
					idStr::Copynz( input_ret, input_field.GetBuffer(), sizeof( input_ret ) );
					assert( hidden );
					tty_Show();
					write( STDOUT_FILENO, &key, 1 );
					input_field.Clear();
					if( history_count < COMMAND_HISTORY )
					{
						history[ history_count ] = input_ret;
						history_count++;
					}
					else
					{
						history[ history_start ] = input_ret;
						history_start++;
						history_start %= COMMAND_HISTORY;
					}
					history_current = 0;
					return input_ret;
				case '\t':
					input_field.AutoComplete();
					break;
				case 27:
				{
					// enter escape sequence mode
					ret = read( STDIN_FILENO, &key, 1 );
					if( ret <= 0 )
					{
						Sys_Printf( "dropping sequence: '27' " );
						tty_FlushIn();
						assert( hidden );
						tty_Show();
						return NULL;
					}
					switch( key )
					{
						case 79:
							ret = read( STDIN_FILENO, &key, 1 );
							if( ret <= 0 )
							{
								Sys_Printf( "dropping sequence: '27' '79' " );
								tty_FlushIn();
								assert( hidden );
								tty_Show();
								return NULL;
							}
							switch( key )
							{
								case 72:
									// xterm only
									input_field.SetCursor( 0 );
									break;
								case 70:
									// xterm only
									input_field.SetCursor( strlen( input_field.GetBuffer() ) );
									break;
								default:
									Sys_Printf( "dropping sequence: '27' '79' '%d' ", key );
									tty_FlushIn();
									assert( hidden );
									tty_Show();
									return NULL;
							}
							break;
						case 91:
						{
							ret = read( STDIN_FILENO, &key, 1 );
							if( ret <= 0 )
							{
								Sys_Printf( "dropping sequence: '27' '91' " );
								tty_FlushIn();
								assert( hidden );
								tty_Show();
								return NULL;
							}
							switch( key )
							{
								case 49:
								{
									ret = read( STDIN_FILENO, &key, 1 );
									if( ret <= 0 || key != 126 )
									{
										Sys_Printf( "dropping sequence: '27' '91' '49' '%d' ", key );
										tty_FlushIn();
										assert( hidden );
										tty_Show();
										return NULL;
									}
									// only screen and linux terms
									input_field.SetCursor( 0 );
									break;
								}
								case 50:
								{
									ret = read( STDIN_FILENO, &key, 1 );
									if( ret <= 0 || key != 126 )
									{
										Sys_Printf( "dropping sequence: '27' '91' '50' '%d' ", key );
										tty_FlushIn();
										assert( hidden );
										tty_Show();
										return NULL;
									}
									// all terms
									input_field.KeyDownEvent( K_INS );
									break;
								}
								case 52:
								{
									ret = read( STDIN_FILENO, &key, 1 );
									if( ret <= 0 || key != 126 )
									{
										Sys_Printf( "dropping sequence: '27' '91' '52' '%d' ", key );
										tty_FlushIn();
										assert( hidden );
										tty_Show();
										return NULL;
									}
									// only screen and linux terms
									input_field.SetCursor( strlen( input_field.GetBuffer() ) );
									break;
								}
								case 51:
								{
									ret = read( STDIN_FILENO, &key, 1 );
									if( ret <= 0 )
									{
										Sys_Printf( "dropping sequence: '27' '91' '51' " );
										tty_FlushIn();
										assert( hidden );
										tty_Show();
										return NULL;
									}
									if( key == 126 )
									{
										input_field.KeyDownEvent( K_DEL );
										break;
									}
									Sys_Printf( "dropping sequence: '27' '91' '51' '%d'", key );
									tty_FlushIn();
									assert( hidden );
									tty_Show();
									return NULL;
								}
								case 65:
								case 66:
								{
									// history
									if( history_current == 0 )
									{
										history_backup = input_field;
									}
									if( key == 65 )
									{
										// up
										history_current++;
									}
									else
									{
										// down
										history_current--;
									}
									// history_current cycle:
									// 0: current edit
									// 1 .. Min( COMMAND_HISTORY, history_count ): back in history
									if( history_current < 0 )
									{
										history_current = Min( COMMAND_HISTORY, history_count );
									}
									else
									{
										history_current %= Min( COMMAND_HISTORY, history_count ) + 1;
									}
									int index = -1;
									if( history_current == 0 )
									{
										input_field = history_backup;
									}
									else
									{
										index = history_start + Min( COMMAND_HISTORY, history_count ) - history_current;
										index %= COMMAND_HISTORY;
										assert( index >= 0 && index < COMMAND_HISTORY );
										input_field.SetBuffer( history[ index ] );
									}
									assert( hidden );
									tty_Show();
									return NULL;
								}
								case 67:
									input_field.KeyDownEvent( K_RIGHTARROW );
									break;
								case 68:
									input_field.KeyDownEvent( K_LEFTARROW );
									break;
								default:
									Sys_Printf( "dropping sequence: '27' '91' '%d' ", key );
									tty_FlushIn();
									assert( hidden );
									tty_Show();
									return NULL;
							}
							break;
						}
						default:
							Sys_Printf( "dropping sequence: '27' '%d' ", key );
							tty_FlushIn();
							assert( hidden );
							tty_Show();
							return NULL;
					}
					break;
				}
				default:
					if( key >= ' ' )
					{
						input_field.CharEvent( key );
						break;
					}
					Sys_Printf( "dropping sequence: '%d' ", key );
					tty_FlushIn();
					assert( hidden );
					tty_Show();
					return NULL;
			}
		}
		if( hidden )
		{
			tty_Show();
		}
		return NULL;
	}
	else
	{
		// no terminal support - read only complete lines
		int				len;
		fd_set			fdset;
		struct timeval	timeout;

		FD_ZERO( &fdset );
		FD_SET( STDIN_FILENO, &fdset );
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if( select( 1, &fdset, NULL, NULL, &timeout ) == -1 || !FD_ISSET( 0, &fdset ) )
		{
			return NULL;
		}

		len = read( 0, input_ret, sizeof( input_ret ) );
		if( len == 0 )
		{
			// EOF
			return NULL;
		}

		if( len < 1 )
		{
			Sys_Printf( "read failed: %s\n", strerror( errno ) );	// something bad happened, cancel this line and print an error
			return NULL;
		}

		if( len == sizeof( input_ret ) )
		{
			Sys_Printf( "read overflow\n" );	// things are likely to break, as input will be cut into pieces
		}

		input_ret[ len - 1 ] = '\0';		// rip off the \n and terminate
		return input_ret;
	}
	return NULL;
}

/*
called during frame loops, pacifier updates etc.
this is only for console input polling and misc mouse grab tasks
the actual mouse and keyboard input is in the Sys_Poll logic
*/
#if !defined(USE_SDL)
void Sys_GenerateEvents()
{
	char* s;
	if( ( s = Posix_ConsoleInput() ) )
	{
		char* b;
		int len;

		len = strlen( s ) + 1;
		b = ( char* )Mem_Alloc( len );
		strcpy( b, s );
		Posix_QueEvent( SE_CONSOLE, 0, 0, len, b );
	}
}
#endif

/*
===============
low level output
===============
*/

void Sys_DebugPrintf( const char* fmt, ... )
{
#if defined(__ANDROID__)
	va_list		argptr;
	char		msg[4096];

	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );
	msg[sizeof( msg ) - 1] = '\0';

	__android_log_print( ANDROID_LOG_DEBUG, "Tekuum_Debug", msg );
#else
	va_list argptr;

	tty_Hide();
	va_start( argptr, fmt );
	vprintf( fmt, argptr );
	va_end( argptr );
	tty_Show();
#endif
}

void Sys_DebugVPrintf( const char* fmt, va_list arg )
{
#if defined(__ANDROID__)
	__android_log_vprint( ANDROID_LOG_DEBUG, "Tekuum_Debug", fmt, arg );
#else
	tty_Hide();
	vprintf( fmt, arg );
	tty_Show();
#endif
}

void Sys_Printf( const char* fmt, ... )
{
#if defined(__ANDROID__)
	va_list		argptr;
	char		msg[4096];

	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );
	msg[sizeof( msg ) - 1] = '\0';

	__android_log_print( ANDROID_LOG_DEBUG, "Tekuum", msg );
#else
	va_list argptr;

	tty_Hide();
	va_start( argptr, fmt );
	vprintf( fmt, argptr );
	va_end( argptr );
	tty_Show();
#endif
}

void Sys_VPrintf( const char* fmt, va_list arg )
{
#if defined(__ANDROID__)
	__android_log_vprint( ANDROID_LOG_DEBUG, "Tekuum", fmt, arg );
#else
	tty_Hide();
	vprintf( fmt, arg );
	tty_Show();
#endif
}

/*
================
Sys_Error
================
*/
void Sys_Error( const char* error, ... )
{
	va_list argptr;

	Sys_Printf( "Sys_Error: " );
	va_start( argptr, error );
	Sys_DebugVPrintf( error, argptr );
	va_end( argptr );
	Sys_Printf( "\n" );

	Posix_Exit( EXIT_FAILURE );
}