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

#include "sys_local.h"

#include <reproc++/reproc.hpp>
#include <reproc++/drain.hpp>

char	sys_cmdline[MAX_STRING_CHARS];

idStrList sysLanguageNames;

idCVar sys_lang( "sys_lang", ID_LANG_ENGLISH, CVAR_SYSTEM | CVAR_INIT | CVAR_ARCHIVE, "" );

idSysLocal			sysLocal;
idSys* 				sys = &sysLocal;

/*
==================
SplitArgs

Splits exePath + a raw args string into an argv vector.
Handles double-quoted tokens with spaces inside them.
==================
*/
static std::vector<std::string> SplitArgs( const char* exePath, const char* args )
{
	std::vector<std::string> argv;

	if( exePath != nullptr )
	{
		argv.push_back( exePath );
	}

	if( args == nullptr || *args == '\0' )
	{
		return argv;
	}

	const char* p = args;
	while( *p )
	{
		while( *p == ' ' )
		{
			++p;
		}
		if( *p == '\0' )
		{
			break;
		}

		std::string token;
		if( *p == '"' )
		{
			++p;
			while( *p && *p != '"' )
			{
				token += *p++;
			}
			if( *p == '"' )
			{
				++p;
			}
		}
		else
		{
			while( *p && *p != ' ' )
			{
				token += *p++;
			}
		}

		if( !token.empty() )
		{
			argv.push_back( token );
		}
	}

	return argv;
}

/*
==================
MakeDetachOptions
==================
*/
static reproc::options MakeDetachOptions()
{
	reproc::options opts;
	opts.stop =
	{
		{ reproc::stop::noop, reproc::milliseconds( 0 ) },
		{ reproc::stop::noop, reproc::milliseconds( 0 ) },
		{ reproc::stop::noop, reproc::milliseconds( 0 ) }
	};
	return opts;
}

/*
==================
idSysLocal::DebugPrintf
==================
*/
void idSysLocal::DebugPrintf( const char* fmt, ... )
{
	va_list argptr;

	va_start( argptr, fmt );
	Sys_DebugVPrintf( fmt, argptr );
	va_end( argptr );
}

/*
==================
idSysLocal::DebugVPrintf
==================
*/
void idSysLocal::DebugVPrintf( const char* fmt, va_list arg )
{
	Sys_DebugVPrintf( fmt, arg );
}

/*
==================
idSysLocal::GetClockTicks
==================
*/
double idSysLocal::GetClockTicks()
{
	return Sys_GetClockTicks();
}

/*
==================
idSysLocal::ClockTicksPerSecond
==================
*/
double idSysLocal::ClockTicksPerSecond()
{
	return Sys_ClockTicksPerSecond();
}

/*
==================
idSysLocal::GetMilliseconds
==================
*/
unsigned int idSysLocal::GetMilliseconds()
{
	return Sys_Milliseconds();
}

/*
==================
idSysLocal::GetProcessorId
==================
*/
int idSysLocal::GetProcessorId()
{
	return Sys_GetProcessorId();
}

/*
==================
idSysLocal::GetProcessorString
==================
*/
const char* idSysLocal::GetProcessorString()
{
	return Sys_GetProcessorString();
}

/*
==================
idSysLocal::FPU_SetFTZ
==================
*/
void idSysLocal::FPU_SetFTZ( bool enable )
{
	Sys_FPU_SetFTZ( enable );
}

/*
==================
idSysLocal::FPU_SetDAZ
==================
*/
void idSysLocal::FPU_SetDAZ( bool enable )
{
	Sys_FPU_SetDAZ( enable );
}

/*
==================
idSysLocal::DLL_Load
==================
*/
int idSysLocal::DLL_Load( const char* dllName )
{
	return Sys_DLL_Load( dllName );
}

/*
==================
idSysLocal::DLL_GetProcAddress
==================
*/
void* idSysLocal::DLL_GetProcAddress( int dllHandle, const char* procName )
{
	return Sys_DLL_GetProcAddress( dllHandle, procName );
}

/*
==================
idSysLocal::DLL_Unload
==================
*/
void idSysLocal::DLL_Unload( int dllHandle )
{
	Sys_DLL_Unload( dllHandle );
}

/*
==================
idSysLocal::DLL_GetFileName
==================
*/
void idSysLocal::DLL_GetFileName( const char* baseName, char* dllName, int maxLength )
{
	idStr::snPrintf( dllName, maxLength, "%s" CPUSTRING ".dll", baseName );
}

/*
==================
idSysLocal::GenerateMouseButtonEvent
==================
*/
sysEvent_t idSysLocal::GenerateMouseButtonEvent( int button, bool down )
{
	sysEvent_t ev;
	ev.evType = SE_KEY;
	ev.evValue = K_MOUSE1 + button - 1;
	ev.evValue2 = down;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;
	return ev;
}

/*
==================
idSysLocal::GenerateMouseMoveEvent
==================
*/
sysEvent_t idSysLocal::GenerateMouseMoveEvent( int deltax, int deltay )
{
	sysEvent_t ev;
	ev.evType = SE_MOUSE;
	ev.evValue = deltax;
	ev.evValue2 = deltay;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;
	return ev;
}

/*
==================
idSysLocal::OpenURL
==================
*/
void idSysLocal::OpenURL( const char* url, bool quit )
{
	// Not sure if this is even used
#if 0
	static bool doexit_spamguard = false;
	HWND wnd;

	if( doexit_spamguard )
	{
		common->DPrintf( "OpenURL: already in an exit sequence, ignoring %s\n", url );
		return;
	}

	common->Printf( "Open URL: %s\n", url );

	if( !ShellExecute( NULL, "open", url, NULL, NULL, SW_RESTORE ) )
	{
		common->Error( "Could not open url: '%s' ", url );
		return;
	}

	wnd = GetForegroundWindow();
	if( wnd )
	{
		ShowWindow( wnd, SW_MAXIMIZE );
	}

	if( quit )
	{
		doexit_spamguard = true;
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
#endif
}

/*
==================
idSysLocal::StartProcess
==================
*/
void idSysLocal::StartProcess( const char* exePath, bool quit )
{
	std::vector<std::string> argv = SplitArgs( exePath, nullptr );

	reproc::process proc;
	std::error_code ec = proc.start( argv, MakeDetachOptions() );
	if( ec )
	{
		common->Error( "Could not start process: '%s' (%s)", exePath, ec.message().c_str() );
		return;
	}

	if( quit )
	{
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}

/*
========================
idSysLocal::StartProcess
========================
*/
void idSysLocal::StartProcess( idCmdArgs& args, void* data, bool quit )
{
	const char* extraArgs = static_cast<const char*>( data );
	idStr exePath;
	if( !Sys_GetPath( PATH_EXE, exePath ) )
	{
		idLib::Error( "Could not get executable path" );
	}
	std::vector<std::string> argv = SplitArgs( exePath.c_str(), extraArgs );

	reproc::process proc;
	std::error_code ec = proc.start( argv, MakeDetachOptions() );
	if( ec )
	{
		idLib::Error( "Could not start process: '%s' (%s)", exePath.c_str(), ec.message().c_str() );
		return;
	}

	if( quit )
	{
		cmdSystem->AppendCommandText( "quit\n" );
	}
}

/*
========================
idSysLocal::ReLaunch
========================
*/
void idSysLocal::ReLaunch( void* data )
{
	const char* args = static_cast<const char*>( data );
	idStr exePath;
	if( !Sys_GetPath( PATH_EXE, exePath ) )
	{
		idLib::Error( "Could not get executable path" );
	}
	std::vector<std::string> argv = SplitArgs( exePath.c_str(), args );

#if defined(_WIN32)
	static HANDLE hProcessMutex;
	CloseHandle( hProcessMutex );
#endif

	reproc::process proc;
	std::error_code ec = proc.start( argv, MakeDetachOptions() );
	if( ec )
	{
		idLib::Error( "Could not start process: '%s' (%s)", exePath.c_str(), ec.message().c_str() );
		return;
	}

	cmdSystem->AppendCommandText( "quit\n" );
}

/*
========================
idSysLocal::Exec

if waitMsec is INFINITE, completely block until the process exits
If waitMsec is -1, don't wait for the process to exit
Other waitMsec values will allow the workFn to be called at those intervals.
========================
*/
bool idSysLocal::Exec( const char* appPath, const char* workingPath, const char* args,
					   execProcessWorkFunction_t workFn, execOutputFunction_t outputFn,
					   const int waitMS, unsigned int& exitCode )
{
	exitCode = 0;

	auto ExecOutputFn = []( const char* text )
	{
		idLib::Printf( text );
	};

	if( outputFn == nullptr )
	{
		outputFn = ExecOutputFn;
	}

	outputFn( va( "^2Executing Process: ^7%s\n^2working path: ^7%s\n^2args: ^7%s\n",
				  appPath, workingPath, args ) );

	std::vector<std::string> argv = SplitArgs( appPath, args );

	reproc::options options;
	options.working_directory = workingPath;
	options.redirect.out.type = reproc::redirect::pipe;
	options.redirect.err.type = reproc::redirect::pipe;

	if( waitMS < 0 )
	{
		options.stop =
		{
			{ reproc::stop::noop, reproc::milliseconds( 0 ) },
			{ reproc::stop::noop, reproc::milliseconds( 0 ) },
			{ reproc::stop::noop, reproc::milliseconds( 0 ) }
		};
	}

	reproc::process proc;
	std::error_code ec = proc.start( argv, options );
	if( ec )
	{
		outputFn( va( "idSysLocal::Exec: failed to start '%s': %s\n", appPath, ec.message().c_str() ) );
		return false;
	}

	if( waitMS < 0 )
	{
		return true;
	}

	auto readAndOutput = [&]( reproc::stream s )
	{
		uint8_t buffer[4096];
		size_t bytes = 0;
		std::tie( bytes, std::ignore ) = proc.read( s, buffer, sizeof( buffer ) - 1 );
		if( bytes == 0 )
		{
			return;
		}
		buffer[bytes] = '\0';
		int len = 0;
		for( int i = 0; buffer[i] != '\0'; i++ )
		{
			if( buffer[i] != '\r' )
			{
				buffer[len++] = buffer[i];
			}
		}
		buffer[len] = '\0';
		outputFn( reinterpret_cast<const char*>( buffer ) );
	};

	// waitMS == 0 means block per poll iteration; positive means workFn interval.
	reproc::milliseconds pollTimeout = ( waitMS == 0 )
									   ? reproc::infinite
									   : reproc::milliseconds( waitMS );

	for( ;; )
	{
		int events = 0;
		std::tie( events, ec ) = proc.poll( reproc::event::out | reproc::event::err, pollTimeout );

		// broken_pipe means the child closed.
		if( ec )
		{
			if( ec != reproc::error::broken_pipe )
			{
				outputFn( va( "idSysLocal::Exec: poll error: %s\n", ec.message().c_str() ) );
			}
			break;
		}

		if( events == 0 || ( events & reproc::event::deadline ) )
		{
			if( workFn != nullptr && !workFn() )
			{
				proc.terminate();
				break;
			}
			continue;
		}

		if( events & reproc::event::out )
		{
			readAndOutput( reproc::stream::out );
		}
		if( events & reproc::event::err )
		{
			readAndOutput( reproc::stream::err );
		}
	}

	int status = 0;
	std::tie( status, ec ) = proc.stop(
	{
		{ reproc::stop::wait,      reproc::milliseconds( 5000 ) },
		{ reproc::stop::terminate, reproc::milliseconds( 2000 ) },
		{ reproc::stop::kill,      reproc::milliseconds( 2000 ) }
	} );

	exitCode = static_cast<unsigned int>( status );
	return true;
}

/*
========================
idSysLocal::GetCmdLine
========================
*/
const char* idSysLocal::GetCmdLine()
{
	return sys_cmdline;
}

/*
========================
idSysLocal::ShowCrashDialog
========================
*/
void idSysLocal::ShowCrashDialog( const char* summaryText )
{
	Sys_ShowCrashDialog( summaryText );
}

/*
=================
Sys_TimeStampToStr
=================
*/
const char* Sys_TimeStampToStr( ID_TIME_T timeStamp )
{
	static char timeString[MAX_STRING_CHARS];
	timeString[0] = '\0';

	time_t ts = ( time_t )timeStamp;
	tm*	time = localtime( &ts );
	if( time == NULL )
	{
		// String separated to prevent detection of trigraphs
		return "??" "/" "??" "/" "???? ??:??";
	}

	idStr out;

	idStr lang = cvarSystem->GetCVarString( "sys_lang" );
	if( lang.Icmp( ID_LANG_ENGLISH ) == 0 )
	{
		// english gets "month/day/year  hour:min" + "am" or "pm"
		out = va( "%02d", time->tm_mon + 1 );
		out += "/";
		out += va( "%02d", time->tm_mday );
		out += "/";
		out += va( "%d", time->tm_year + 1900 );
		out += " ";	// changed to spaces since flash doesn't recognize \t
		if( time->tm_hour > 12 )
		{
			out += va( "%02d", time->tm_hour - 12 );
		}
		else if( time->tm_hour == 0 )
		{
			out += "12";
		}
		else
		{
			out += va( "%02d", time->tm_hour );
		}
		out += ":";
		out += va( "%02d", time->tm_min );
		if( time->tm_hour >= 12 )
		{
			out += "pm";
		}
		else
		{
			out += "am";
		}
	}
	else
	{
		// europeans get "day/month/year  24hour:min"
		out = va( "%02d", time->tm_mday );
		out += "/";
		out += va( "%02d", time->tm_mon + 1 );
		out += "/";
		out += va( "%d", time->tm_year + 1900 );
		out += " ";	// changed to spaces since flash doesn't recognize \t
		out += va( "%02d", time->tm_hour );
		out += ":";
		out += va( "%02d", time->tm_min );
	}
	idStr::Copynz( timeString, out, sizeof( timeString ) );

	return timeString;
}

/*
========================
Sys_SecToStr
========================
*/
const char* Sys_SecToStr( int sec )
{
	static char timeString[MAX_STRING_CHARS];

	int weeks = sec / ( 3600 * 24 * 7 );
	sec -= weeks * ( 3600 * 24 * 7 );

	int days = sec / ( 3600 * 24 );
	sec -= days * ( 3600 * 24 );

	int hours = sec / 3600;
	sec -= hours * 3600;

	int min = sec / 60;
	sec -= min * 60;

	if( weeks > 0 )
	{
		idStr::snPrintf( timeString, sizeof( timeString ), "%dw, %dd, %d:%02d:%02d", weeks, days, hours, min, sec );
	}
	else if( days > 0 )
	{
		idStr::snPrintf( timeString, sizeof( timeString ), "%dd, %d:%02d:%02d", days, hours, min, sec );
	}
	else
	{
		idStr::snPrintf( timeString, sizeof( timeString ), "%d:%02d:%02d", hours, min, sec );
	}

	return timeString;
}

/*
================
Sys_Lang

return number of supported languages
================
*/
int Sys_NumLangs()
{
	return sysLanguageNames.Num();
}

/*
================
Sys_Lang

get language name by index
================
*/
const char* Sys_Lang( int idx )
{
	if( idx >= 0 && idx < sysLanguageNames.Num() )
	{
		return sysLanguageNames[ idx ];
	}
	return "";
}

/*
================
Sys_LangIndex
================
*/
int Sys_LangIndex( const char* lang )
{
	for( int i = 0; i < sysLanguageNames.Num(); i++ )
	{
		if( idStr::Icmp( lang, sysLanguageNames[i] ) == 0 )
		{
			return i;
		}
	}
	return -1;
}

/*
================
Sys_DefaultLanguage
================
*/
const char* Sys_DefaultLanguage()
{
	// sku breakdowns are as follows
	//  EFIGS	Digital
	//  EF  S	North America
	//   FIGS	EU
	//  E		UK
	// JE    	Japan

	// If japanese exists, default to japanese
	// else if english exists, defaults to english
	// otherwise, french

	if( !fileSystem->UsingResourceFiles() )
	{
		return ID_LANG_ENGLISH;
	}

	// Prevent sys_lang to revert to english if is set manually
	if( idStr::Icmp( ID_LANG_ENGLISH, sys_lang.GetString() ) != 0 )
	{
		return sys_lang.GetString();
	}

	idStr fileName;

	//D3XP: Instead of just loading a single lang file for each language
	//we are going to load all files that begin with the language name
	//similar to the way pak files work. So you can place english001.lang
	//to add new strings to the english language dictionary
	idFileList* langFiles;
	langFiles = fileSystem->ListFilesTree( "strings", ".lang", true );

	idStrList langList = langFiles->GetList();

	// Loop through the list and filter
	idStrList currentLangList = langList;

	idStr temp;
	for( int i = 0; i < currentLangList.Num(); i++ )
	{
		temp = currentLangList[i];
		temp = temp.Right( temp.Length() - strlen( "strings/" ) );
		temp = temp.Left( temp.Length() - strlen( ".lang" ) );
		currentLangList[i] = temp;
		// Update available lang list with potentianly new languages
		if( temp.Find( "_" ) >= 0 )
		{
			sysLanguageNames.AddUnique( temp.SubStr( 0, temp.Find( "_" ) ) );
		}
	}

	if( currentLangList.Num() <= 0 )
	{
		// call it English if no lang files exist
		sys_lang.SetString( ID_LANG_ENGLISH );
	}
	else if( currentLangList.Num() == 1 )
	{
		sys_lang.SetString( currentLangList[0] );
	}
	else
	{
		if( currentLangList.Find( ID_LANG_ENGLISH ) )
		{
			sys_lang.SetString( ID_LANG_ENGLISH );
		}
		else if( currentLangList.Find( ID_LANG_JAPANESE ) )
		{
			sys_lang.SetString( ID_LANG_JAPANESE );
		}
		else if( currentLangList.Find( ID_LANG_FRENCH ) )
		{
			sys_lang.SetString( ID_LANG_FRENCH );
		}
		else if( currentLangList.Find( ID_LANG_GERMAN ) )
		{
			sys_lang.SetString( ID_LANG_GERMAN );
		}
		else if( currentLangList.Find( ID_LANG_ITALIAN ) )
		{
			sys_lang.SetString( ID_LANG_ITALIAN );
		}
		else if( currentLangList.Find( ID_LANG_SPANISH ) )
		{
			sys_lang.SetString( ID_LANG_SPANISH );
		}
		else
		{
			sys_lang.SetString( currentLangList[0] );
		}
	}

	fileSystem->FreeFileList( langFiles );

	return sys_lang.GetString();
}

/*
================
Sys_SetLanguageFromSystem
================
*/
void Sys_SetLanguageFromSystem()
{
	sys_lang.SetString( Sys_DefaultLanguage() );
}