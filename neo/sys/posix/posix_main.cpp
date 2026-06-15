/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

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
#include "../../idlib/precompiled.h"
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

#if defined(__APPLE__)
	#include <SDL.h>
#endif

#include <sys/statvfs.h>
// RB end

#include "posix_public.h"

#define					MAX_OSPATH 256
#define					COMMAND_HISTORY 64

static idStr			basepath;
static idStr			savepath;

static int				input_hide = 0;

idEditField				input_field;
static char				input_ret[256];

static idStr			history[ COMMAND_HISTORY ];	// cycle buffer
static int				history_count = 0;			// buffer fill up
static int				history_start = 0;			// current history start
static int				history_current = 0;		// goes back in history
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
 ==============
 Sys_DefaultSavePath
 ==============
 */
const char* Sys_DefaultSavePath()
{
#if defined(__APPLE__)
	char* base_path = SDL_GetPrefPath( "", "RBDOOM-3-BFG" );
	if( base_path )
	{
		savepath = base_path;
		savepath.StripTrailing( '/' );
		SDL_free( base_path );
	}
#else
	const char* xdg_data_home = getenv( "XDG_DATA_HOME" );
	if( xdg_data_home != NULL )
	{
		sprintf( savepath, "%s/rbdoom3bfg", xdg_data_home );
	}
	else
	{
		sprintf( savepath, "%s/.local/share/rbdoom3bfg", getenv( "HOME" ) );
	}
#endif

	return savepath.c_str();
}

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

	//if( asyncThread.threadHandle )
	//{
	//	Sys_DestroyThread( asyncThread );
	//}

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
================
Sys_Quit
================
*/
void Sys_Quit()
{
	Posix_Exit( EXIT_SUCCESS );
}

/*
===============
Sys_Shutdown
===============
*/
void Sys_Shutdown()
{
	basepath.Clear();
	savepath.Clear();
	Posix_Shutdown();
}

/*
================
Sys_DefaultBasePath

Get the default base path
- binary image path
- current directory
- macOS app bundle resources directory path			// SRS - added macOS app bundle resources path
- build directory path								// SRS - added build directory path
- hardcoded
Try to be intelligent: if there is no BASE_GAMEDIR, try the next path
================
*/
const char* Sys_DefaultBasePath()
{
	struct stat st;
	idStr testbase, exepath = {};
	basepath = Sys_EXEPath();
	if( basepath.Length() )
	{
		exepath = basepath.StripFilename();
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;
		if( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
		{
			return basepath.c_str();
		}
		else
		{
			common->Printf( "no '%s' directory in exe path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
	}
	if( basepath != Posix_Cwd() )
	{
		basepath = Posix_Cwd();
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;
		if( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
		{
			return basepath.c_str();
		}
		else
		{
			common->Printf( "no '%s' directory in cwd path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
	}
	if( exepath.Length() )
	{
#if defined(__APPLE__)
		// SRS - Check for macOS app bundle resources path (up one dir level and down to Resources dir)
		basepath = exepath;
		basepath = basepath.StripFilename() + "/Resources";
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;
		if( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
		{
			return basepath.c_str();
		}
		else
		{
			common->Printf( "no '%s' directory in macOS app bundle resources path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
#endif
		// SRS - Check for linux/macOS build path (directory structure with build dir and possible config suffix)
		basepath = exepath;
		basepath.StripFilename();						// up 1st dir level for single-config dev builds
#if !defined( NO_MULTI_CONFIG )
		basepath.StripFilename();						// up 2nd dir level for multi-config dev builds with Debug/Release/etc suffix
#endif
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;
		if( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
		{
			return basepath.c_str();
		}
		else
		{
			common->Printf( "no '%s' directory in build path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
	}
	common->Printf( "WARNING: using hardcoded default base path %s\n", DEFAULT_BASEPATH );
	return DEFAULT_BASEPATH;
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
================
Sys_Rmdir
================
*/
bool Sys_Rmdir( const char* path )
{
	return ( rmdir( path ) == 0 );
}

/*
========================
Sys_IsFileWritable
========================
*/
bool Sys_IsFileWritable( const char* path )
{
	struct stat st;
	if( stat( path, &st ) == -1 )
	{
		return true;
	}

	return ( st.st_mode & S_IWRITE ) != 0;
}

/*
========================
Sys_IsFolder
========================
*/
sysFolder_t	 Sys_IsFolder( const char* path )
{
	struct stat buffer;

	if( stat( path, &buffer ) < 0 )
	{
		return FOLDER_ERROR;
	}

	return ( buffer.st_mode & S_IFDIR ) != 0 ? FOLDER_YES : FOLDER_NO;
}

// RB end

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
================
Sys_ShowConsole
================
*/
void Sys_ShowConsole( int visLevel, bool quitOnClose ) { }

// ---------------------------------------------------------------------------

ID_TIME_T Sys_FileTimeStamp( idFileHandle fp )
{
	struct stat st;
	fstat( fileno( fp ), &st );
	return st.st_mtime;
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

// RB begin

/*
================
Sys_GetDriveFreeSpace
returns in megabytes
================
*/
int Sys_GetDriveFreeSpace( const char* path )
{
	int ret = 26;

	struct statvfs st;

	if( statvfs( path, &st ) == 0 )
	{
		unsigned long blocksize = st.f_bsize;
		unsigned long freeblocks = st.f_bfree;

		unsigned long free = blocksize * freeblocks;

		ret = ( double )( free ) / ( 1024.0 * 1024.0 );
	}

	return ret;
}

/*
========================
Sys_GetDriveFreeSpaceInBytes
========================
*/
int64 Sys_GetDriveFreeSpaceInBytes( const char* path )
{
	int64 ret = 1;

	struct statvfs st;

	if( statvfs( path, &st ) == 0 )
	{
		unsigned long blocksize = st.f_bsize;
		unsigned long freeblocks = st.f_bfree;

		unsigned long free = blocksize * freeblocks;

		ret = free;
	}

	return ret;
}

// RB end

/*
===============
Posix_EarlyInit
===============
*/
void Posix_EarlyInit()
{
	//memset( &asyncThread, 0, sizeof( asyncThread ) );

	exit_spawn[0] = '\0';
	Posix_InitSigs();

	// set the base time
	Sys_Milliseconds();

	//Posix_InitPThreads();
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
//	common->Printf( "%d MB System Memory\n", Sys_GetSystemRam() );

//#ifndef ID_DEDICATED
	//common->Printf( "%d MB Video Memory\n", Sys_GetVideoRam() );
//#endif

	//Posix_StartAsyncThread( );
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
			//__android_log_print(ANDROID_LOG_DEBUG, "RBDoom3_DEBUG", "%s", buf);
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
		// disabled on OSX. works fine from a terminal, but launching from Finder is causing trouble
		// I'm pretty sure it could be re-enabled if needed, and just handling the Finder failure case right (TTimo)
#ifndef __APPLE__
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
#endif
	}
	return NULL;
}

/*
called during frame loops, pacifier updates etc.
this is only for console input polling and misc mouse grab tasks
the actual mouse and keyboard input is in the Sys_Poll logic
*/
/*
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
*/

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

	__android_log_print( ANDROID_LOG_DEBUG, "RBDoom3_Debug", msg );
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
	__android_log_vprint( ANDROID_LOG_DEBUG, "RBDoom3_Debug", fmt, arg );
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

	__android_log_print( ANDROID_LOG_DEBUG, "RBDoom3", msg );
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
	__android_log_vprint( ANDROID_LOG_DEBUG, "RBDoom3", fmt, arg );
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

/*
================
Sys_SetLanguageFromSystem
================
*/
extern idCVar sys_lang;
void Sys_SetLanguageFromSystem()
{
	sys_lang.SetString( Sys_DefaultLanguage() );
}

/*
=================
Sys_OpenURL
=================
*/
void idSysLocal::OpenURL( const char* url, bool quit )
{
	const char*	script_path;
	idFile*		script_file;
	char		cmdline[ 1024 ];

	static bool	quit_spamguard = false;

	if( quit_spamguard )
	{
		common->DPrintf( "Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url );
		return;
	}

	common->Printf( "Open URL: %s\n", url );
	// opening an URL on *nix can mean a lot of things ..
	// just spawn a script instead of deciding for the user :-)

	// look in the savepath first, then in the basepath
	script_path = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_savepath" ), "", "openurl.sh" );
	script_file = fileSystem->OpenExplicitFileRead( script_path );
	if( !script_file )
	{
		script_path = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_basepath" ), "", "openurl.sh" );
		script_file = fileSystem->OpenExplicitFileRead( script_path );
	}
	if( !script_file )
	{
		common->Printf( "Can't find URL script 'openurl.sh' in either savepath or basepath\n" );
		common->Printf( "OpenURL '%s' failed\n", url );
		return;
	}
	fileSystem->CloseFile( script_file );

	// if we are going to quit, only accept a single URL before quitting and spawning the script
	if( quit )
	{
		quit_spamguard = true;
	}

	common->Printf( "URL script: %s\n", script_path );

	// StartProcess is going to execute a system() call with that - hence the &
	idStr::snPrintf( cmdline, 1024, "%s '%s' &",  script_path, url );
	sys->StartProcess( cmdline, quit );
}


