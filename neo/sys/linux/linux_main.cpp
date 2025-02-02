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
#include "../posix/posix_public.h"
#include "../sys_local.h"
//#include "local.h"

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

// DG: needed for Sys_ReLaunch()
#include <dirent.h>

static const char** cmdargv = NULL;
static int cmdargc = 0;
// DG end

/*
=================
Sys_AsyncThread
=================
*/
void Sys_AsyncThread()
{
// RB: disabled multi tick compensate because it feels very laggy on Linux 3.x kernels
#if 0
	int now;
	int next;
	int	want_sleep;

	// multi tick compensate for poor schedulers (Linux 2.4)
	int ticked, to_ticked;
	now = Sys_Milliseconds();
	ticked = now / USERCMD_MSEC;
	while( 1 )
	{
		// sleep
		now = Sys_Milliseconds();
		next = now + USERCMD_MSEC;
		want_sleep = ( next - now - 1 ) * 1000;
		if( want_sleep > 0 )
		{
			usleep( want_sleep ); // sleep 1ms less than true target
		}

		// compensate if we slept too long
		now = Sys_Milliseconds();
		to_ticked = now / USERCMD_MSEC;

		// show ticking statistics - every 100 ticks, print a summary
#if 1
#define STAT_BUF 100
		static int stats[STAT_BUF];
		static int counter = 0;
		// how many ticks to play
		stats[counter] = to_ticked - ticked;
		counter++;
		if( counter == STAT_BUF )
		{
			Sys_DebugPrintf( "\n" );
			for( int i = 0; i < STAT_BUF; i++ )
			{
				if( !( i & 0xf ) )
				{
					Sys_DebugPrintf( "\n" );
				}
				Sys_DebugPrintf( "%d ", stats[i] );
			}
			Sys_DebugPrintf( "\n" );
			counter = 0;
		}
#endif

		while( ticked < to_ticked )
		{
			common->Async();
			ticked++;
			Sys_TriggerEvent( TRIGGER_EVENT_ONE );
		}
		// thread exit
		pthread_testcancel();
	}
#elif 0
	int ticked;
	int to_ticked;
	int start;
	int elapsed;

	start = Sys_Milliseconds();
	ticked = start / USERCMD_MSEC;
	while( 1 )
	{
		start = Sys_Milliseconds();

		to_ticked = start / USERCMD_MSEC;
		while( ticked < to_ticked )
		{
			common->Async();
			ticked++;
			Sys_TriggerEvent( TRIGGER_EVENT_ONE );
		}

		// thread exit
		pthread_testcancel();

		elapsed = Sys_Milliseconds() - start;

		Sys_DebugPrintf( "elapsed = %d\n", elapsed );

		if( elapsed < USERCMD_MSEC )
		{
			usleep( ( USERCMD_MSEC - elapsed ) * 1000 );
		}
	}
#else
	int now;
	int next;
	int want_sleep;
	int ticked;

	now = Sys_Milliseconds();
	ticked = 0;
	while( 1 )
	{
		now = Sys_Milliseconds();
		next = ( now & 0xFFFFFFF0 ) + USERCMD_MSEC;

		// sleep 1ms less than true target
		want_sleep = ( next - now - 1 ) * 1000;

		if( want_sleep > 0 )
		{
			usleep( want_sleep );
		}

		common->Async();
		ticked++;

		Sys_TriggerEvent( TRIGGER_EVENT_ONE );

		// thread exit
		pthread_testcancel();
	}
#endif
// RB end
}

/*
 ==============
 Sys_GetPath
 ==============
*/
bool Sys_GetPath( sysPath_t type, idStr& path )
{
	const char* s;
	char buf[MAX_OSPATH];
	char buf2[MAX_OSPATH];
	struct stat st;
	size_t len;

	path.Clear();

	switch( type )
	{
		case PATH_BASE:
			if( stat( BUILD_DATADIR, &st ) != -1 && S_ISDIR( st.st_mode ) )
			{
				path = BUILD_DATADIR;
				return true;
			}

			common->Warning( "base path '" BUILD_DATADIR "' does not exist" );

			// try next to the executable..
			if( Sys_GetPath( PATH_EXE, path ) )
			{
				path = path.StripFilename();
				// the path should have a base dir in it, otherwise it probably just contains the executable
				idStr testPath = path + "/" BASE_GAMEDIR;
				if( stat( testPath.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
				{
					common->Warning( "using path of executable: %s", path.c_str() );
					return true;
				}
				else
				{
					idStr testPath = path + "/demo/demo00.pk4";
					if( stat( testPath.c_str(), &st ) != -1 && S_ISREG( st.st_mode ) )
					{
						common->Warning( "using path of executable (seems to contain demo game data): %s", path.c_str() );
						return true;
					}
					else
					{
						path.Clear();
					}
				}
			}

			// fallback to vanilla doom3 install
			if( stat( LINUX_DEFAULT_PATH, &st ) != -1 && S_ISDIR( st.st_mode ) )
			{
				common->Warning( "using hardcoded default base path: " LINUX_DEFAULT_PATH );

				path = LINUX_DEFAULT_PATH;
				return true;
			}

			return false;

		case PATH_CONFIG:
			s = getenv( "XDG_CONFIG_HOME" );
			if( s )
			{
				idStr::snPrintf( buf, sizeof( buf ), "%s/dhewm3", s );
			}
			else
			{
				idStr::snPrintf( buf, sizeof( buf ), "%s/.config/dhewm3", getenv( "HOME" ) );
			}

			path = buf;
			return true;

		case PATH_SAVE:
			s = getenv( "XDG_DATA_HOME" );
			if( s )
			{
				idStr::snPrintf( buf, sizeof( buf ), "%s/dhewm3", s );
			}
			else
			{
				idStr::snPrintf( buf, sizeof( buf ), "%s/.local/share/dhewm3", getenv( "HOME" ) );
			}

			path = buf;
			return true;

		case PATH_EXE:
			idStr::snPrintf( buf, sizeof( buf ), "/proc/%d/exe", getpid() );
			len = readlink( buf, buf2, sizeof( buf2 ) );
			if( len != -1 )
			{
				if( len < MAX_OSPATH )
				{
					buf2[len] = '\0';
				}
				else
				{
					buf2[MAX_OSPATH - 1] = '\0';
				}
				path = buf2;
				return true;
			}

			if( path_argv[0] != 0 )
			{
				path = path_argv;
				return true;
			}

			return false;
	}

	return false;
}

/*
===============
Sys_Shutdown
===============
*/
void Sys_Shutdown()
{
	Posix_Shutdown();
}

/*
===============
Sys_GetProcessorId
===============
*/
cpuid_t Sys_GetProcessorId()
{
	return CPUID_GENERIC;
}

/*
===============
Sys_FPE_handler
===============
*/
void Sys_FPE_handler( int signum, siginfo_t* info, void* context )
{
	assert( signum == SIGFPE );
	Sys_Printf( "FPE\n" );
}

/*
===============
Sys_GetClockticks
===============
*/
double Sys_GetClockTicks()
{
#if defined( __i386__ )
	unsigned long lo, hi;

	__asm__ __volatile__(
		"push %%ebx\n"			\
		"xor %%eax,%%eax\n"		\
		"cpuid\n"					\
		"rdtsc\n"					\
		"mov %%eax,%0\n"			\
		"mov %%edx,%1\n"			\
		"pop %%ebx\n"
		: "=r"( lo ), "=r"( hi ) );
	return ( double ) lo + ( double ) 0xFFFFFFFF * hi;
#else
//#error unsupported CPU
// RB begin
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return now.tv_sec * 1000000000LL + now.tv_nsec;
// RB end
#endif
}

/*
===============
MeasureClockTicks
===============
*/
double MeasureClockTicks()
{
	double t0, t1;

	t0 = Sys_GetClockTicks( );
	Sys_Sleep( 1000 );
	t1 = Sys_GetClockTicks( );
	return t1 - t0;
}

/*
===============
Sys_ClockTicksPerSecond
===============
*/
double Sys_ClockTicksPerSecond()
{
	static bool		init = false;
	static double	ret;

	int		fd, len, pos, end;
	char	buf[ 4096 ];

	if( init )
	{
		return ret;
	}

	fd = open( "/proc/cpuinfo", O_RDONLY );
	if( fd == -1 )
	{
		common->Printf( "couldn't read /proc/cpuinfo\n" );
		ret = MeasureClockTicks();
		init = true;
		common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
		return ret;
	}
	len = read( fd, buf, 4096 );
	close( fd );
	pos = 0;
	while( pos < len )
	{
		if( !idStr::Cmpn( buf + pos, "cpu MHz", 7 ) )
		{
			pos = strchr( buf + pos, ':' ) - buf + 2;
			end = strchr( buf + pos, '\n' ) - buf;
			if( pos < len && end < len )
			{
				buf[end] = '\0';
				ret = atof( buf + pos );
			}
			else
			{
				common->Printf( "failed parsing /proc/cpuinfo\n" );
				ret = MeasureClockTicks();
				init = true;
				common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
				return ret;
			}
			common->Printf( "/proc/cpuinfo CPU frequency: %g MHz\n", ret );
			ret *= 1000000;
			init = true;
			return ret;
		}
		pos = strchr( buf + pos, '\n' ) - buf + 1;
	}
	common->Printf( "failed parsing /proc/cpuinfo\n" );
	ret = MeasureClockTicks();
	init = true;
	common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
	return ret;
}

/*
========================
Sys_CPUCount

numLogicalCPUCores	- the number of logical CPU per core
numPhysicalCPUCores	- the total number of cores per package
numCPUPackages		- the total number of packages (physical processors)
========================
*/
// RB begin
void Sys_CPUCount( int& numLogicalCPUCores, int& numPhysicalCPUCores, int& numCPUPackages )
{
	static bool		init = false;
	static double	ret;

	static int		s_numLogicalCPUCores;
	static int		s_numPhysicalCPUCores;
	static int		s_numCPUPackages;

	int		fd, len, pos, end;
	char	buf[ 4096 ];
	char	number[100];

	if( init )
	{
		numPhysicalCPUCores = s_numPhysicalCPUCores;
		numLogicalCPUCores = s_numLogicalCPUCores;
		numCPUPackages = s_numCPUPackages;
	}

	s_numPhysicalCPUCores = 1;
	s_numLogicalCPUCores = 1;
	s_numCPUPackages = 1;

	fd = open( "/proc/cpuinfo", O_RDONLY );
	if( fd != -1 )
	{
		len = read( fd, buf, 4096 );
		close( fd );
		pos = 0;
		while( pos < len )
		{
			if( !idStr::Cmpn( buf + pos, "processor", 9 ) )
			{
				pos = strchr( buf + pos, ':' ) - buf + 2;
				end = strchr( buf + pos, '\n' ) - buf;
				if( pos < len && end < len )
				{
					idStr::Copynz( number, buf + pos, sizeof( number ) );
					assert( ( end - pos ) > 0 && ( end - pos ) < sizeof( number ) );
					number[ end - pos ] = '\0';

					int processor = atoi( number );

					if( ( processor + 1 ) > s_numPhysicalCPUCores )
					{
						s_numPhysicalCPUCores = processor + 1;
					}
				}
				else
				{
					common->Printf( "failed parsing /proc/cpuinfo\n" );
					break;
				}
			}
			else if( !idStr::Cmpn( buf + pos, "core id", 7 ) )
			{
				pos = strchr( buf + pos, ':' ) - buf + 2;
				end = strchr( buf + pos, '\n' ) - buf;
				if( pos < len && end < len )
				{
					idStr::Copynz( number, buf + pos, sizeof( number ) );
					assert( ( end - pos ) > 0 && ( end - pos ) < sizeof( number ) );
					number[ end - pos ] = '\0';

					int coreId = atoi( number );

					if( ( coreId + 1 ) > s_numLogicalCPUCores )
					{
						s_numLogicalCPUCores = coreId + 1;
					}
				}
				else
				{
					common->Printf( "failed parsing /proc/cpuinfo\n" );
					break;
				}
			}

			pos = strchr( buf + pos, '\n' ) - buf + 1;
		}
	}

	common->Printf( "/proc/cpuinfo CPU processors: %d\n", s_numPhysicalCPUCores );
	common->Printf( "/proc/cpuinfo CPU logical cores: %d\n", s_numLogicalCPUCores );

	numPhysicalCPUCores = s_numPhysicalCPUCores;
	numLogicalCPUCores = s_numLogicalCPUCores;
	numCPUPackages = s_numCPUPackages;
}
// RB end

/*
================
Sys_GetSystemRam
returns in megabytes
================
*/
int Sys_GetSystemRam()
{
	long	count, page_size;
	int		mb;

	count = sysconf( _SC_PHYS_PAGES );
	if( count == -1 )
	{
		common->Printf( "GetSystemRam: sysconf _SC_PHYS_PAGES failed\n" );
		return 512;
	}
	page_size = sysconf( _SC_PAGE_SIZE );
	if( page_size == -1 )
	{
		common->Printf( "GetSystemRam: sysconf _SC_PAGE_SIZE failed\n" );
		return 512;
	}
	mb = ( int )( ( double )count * ( double )page_size / ( 1024 * 1024 ) );
	// round to the nearest 16Mb
	mb = ( mb + 8 ) & ~15;
	return mb;
}



/*
==================
Sys_DoStartProcess
if we don't fork, this function never returns
the no-fork lets you keep the terminal when you're about to spawn an installer

if the command contains spaces, system() is used. Otherwise the more straightforward execl ( system() blows though )
==================
*/
void Sys_DoStartProcess( const char* exeName, bool dofork )
{
	bool use_system = false;
	if( strchr( exeName, ' ' ) )
	{
		use_system = true;
	}
	else
	{
		// set exec rights when it's about a single file to execute
		struct stat buf;
		if( stat( exeName, &buf ) == -1 )
		{
			printf( "stat %s failed: %s\n", exeName, strerror( errno ) );
		}
		else
		{
			if( chmod( exeName, buf.st_mode | S_IXUSR ) == -1 )
			{
				printf( "cmod +x %s failed: %s\n", exeName, strerror( errno ) );
			}
		}
	}
	if( dofork )
	{
		switch( fork() )
		{
			case -1:
				// main thread
				break;
			case 0:
				if( use_system )
				{
					printf( "system %s\n", exeName );
					system( exeName );
					_exit( 0 );
				}
				else
				{
					printf( "execl %s\n", exeName );
					execl( exeName, exeName, NULL );
					printf( "execl failed: %s\n", strerror( errno ) );
					_exit( -1 );
				}
				break;
		}
	}
	else
	{
		if( use_system )
		{
			printf( "system %s\n", exeName );
			system( exeName );
			sleep( 1 );	// on some systems I've seen that starting the new process and exiting this one should not be too close
		}
		else
		{
			printf( "execl %s\n", exeName );
			execl( exeName, exeName, NULL );
			printf( "execl failed: %s\n", strerror( errno ) );
		}
		// terminate
		_exit( 0 );
	}
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

/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ( bool enable )
{
	/*
	DWORD dwData;

	_asm {
		movzx	ecx, byte ptr enable
		and		ecx, 1
		shl		ecx, 6
		STMXCSR	dword ptr dwData
		mov		eax, dwData
		and		eax, ~(1<<6)	// clear DAX bit
		or		eax, ecx		// set the DAZ bit
		mov		dwData, eax
		LDMXCSR	dword ptr dwData
	}
	*/
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ( bool enable )
{
	/*
	DWORD dwData;

	_asm {
		movzx	ecx, byte ptr enable
		and		ecx, 1
		shl		ecx, 15
		STMXCSR	dword ptr dwData
		mov		eax, dwData
		and		eax, ~(1<<15)	// clear FTZ bit
		or		eax, ecx		// set the FTZ bit
		mov		dwData, eax
		LDMXCSR	dword ptr dwData
	}
	*/
}

/*
========================
Sys_GetCmdLine
========================
*/
const char* Sys_GetCmdLine()
{
	// DG: don't use this, use cmdargv and cmdargc instead!
	return "TODO Sys_GetCmdLine";
}

/*
========================
Sys_ReLaunch
========================
*/
#if 0
void Sys_ReLaunch()
{
	// DG: implementing this... basic old fork() exec() (+ setsid()) routine..
	// NOTE: this function used to have parameters: the commandline arguments, but as one string..
	//       for Linux/Unix we want one char* per argument so we'll just add the friggin'
	//       " +set com_skipIntroVideos 1" to the other commandline arguments in this function.

	int ret = fork();
	if( ret < 0 )
	{
		idLib::Error( "Sys_ReLaunch(): Couldn't fork(), reason: %s ", strerror( errno ) );
	}

	if( ret == 0 )
	{
		// child process

		// get our own session so we don't depend on the (soon to be killed)
		// parent process anymore - else we'll freeze
		pid_t sId = setsid();
		if( sId == ( pid_t ) - 1 )
		{
			idLib::Error( "Sys_ReLaunch(): setsid() failed! Reason: %s ", strerror( errno ) );
		}

		// close all FDs (except for stdin/out/err) so we don't leak FDs
		DIR* devfd = opendir( "/dev/fd" );
		if( devfd != NULL )
		{
			struct dirent entry;
			struct dirent* result;
			while( readdir_r( devfd, &entry, &result ) == 0 )
			{
				const char* filename = result->d_name;
				char* endptr = NULL;
				long int fd = strtol( filename, &endptr, 0 );
				if( endptr != filename && fd > STDERR_FILENO )
				{
					close( fd );
				}
			}
		}
		else
		{
			idLib::Warning( "Sys_ReLaunch(): Couldn't open /dev/fd/ - will leak file descriptors. Reason: %s", strerror( errno ) );
		}

		// + 3 because "+set" "com_skipIntroVideos" "1" - and note that while we'll skip
		// one (the first) cmdargv argument, we need one more pointer for NULL at the end.
		int argc = cmdargc + 3;
		const char** argv = ( const char** )calloc( argc, sizeof( char* ) );

		int i;
		for( i = 0; i < cmdargc - 1; ++i )
		{
			argv[i] = cmdargv[i + 1];    // ignore cmdargv[0] == executable name
		}

		// add +set com_skipIntroVideos 1
		argv[i++] = "+set";
		argv[i++] = "com_skipIntroVideos";
		argv[i++] = "1";
		// execv expects NULL terminated array
		argv[i] = NULL;

		const char* exepath = Sys_EXEPath();

		errno = 0;
		execv( exepath, ( char** )argv );
		// we only get here if execv() fails, else the executable is restarted
		idLib::Error( "Sys_ReLaunch(): WTF exec() failed! Reason: %s ", strerror( errno ) );

	}
	else
	{
		// original process
		// just do a clean shutdown
		cmdSystem->AppendCommandText( "quit\n" );
	}
	// DG end
}
#endif

/*
===============
main
===============
*/
int main( int argc, const char** argv )
{
	// DG: needed for Sys_ReLaunch()
	cmdargc = argc;
	cmdargv = argv;
	// DG end

	Posix_EarlyInit( );

	if( argc > 1 )
	{
		common->Init( argc - 1, &argv[1], NULL );
	}
	else
	{
		common->Init( 0, NULL, NULL );
	}

	Posix_LateInit( );

	while( 1 )
	{
		common->Frame();
	}
}
