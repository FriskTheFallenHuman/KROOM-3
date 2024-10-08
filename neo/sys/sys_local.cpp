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
#pragma hdrstop
#include "sys_local.h"

const char* sysLanguageNames[] =
{
	"english", "spanish", "italian", "german", "french", "russian",
	"polish", "korean", "japanese", "chinese", NULL
};

idCVar sys_lang( "sys_lang", "english", CVAR_SYSTEM | CVAR_ARCHIVE,  "", sysLanguageNames, idCmdSystem::ArgCompletion_String<sysLanguageNames> );

idSysLocal			sysLocal;
idSys* 				sys = &sysLocal;

void idSysLocal::DebugPrintf( const char* fmt, ... )
{
	va_list argptr;

	va_start( argptr, fmt );
	Sys_DebugVPrintf( fmt, argptr );
	va_end( argptr );
}

void idSysLocal::DebugVPrintf( const char* fmt, va_list arg )
{
	Sys_DebugVPrintf( fmt, arg );
}

double idSysLocal::GetClockTicks()
{
	return Sys_GetClockTicks();
}

double idSysLocal::ClockTicksPerSecond()
{
	return Sys_ClockTicksPerSecond();
}

cpuid_t idSysLocal::GetProcessorId()
{
	return Sys_GetProcessorId();
}

void idSysLocal::FPU_SetFTZ( bool enable )
{
	Sys_FPU_SetFTZ( enable );
}

void idSysLocal::FPU_SetDAZ( bool enable )
{
	Sys_FPU_SetDAZ( enable );
}

bool idSysLocal::LockMemory( void* ptr, int bytes )
{
	return Sys_LockMemory( ptr, bytes );
}

bool idSysLocal::UnlockMemory( void* ptr, int bytes )
{
	return Sys_UnlockMemory( ptr, bytes );
}

uintptr_t idSysLocal::DLL_Load( const char* dllName )
{
	return Sys_DLL_Load( dllName );
}

void* idSysLocal::DLL_GetProcAddress( uintptr_t dllHandle, const char* procName )
{
	return Sys_DLL_GetProcAddress( dllHandle, procName );
}

void idSysLocal::DLL_Unload( uintptr_t dllHandle )
{
	Sys_DLL_Unload( dllHandle );
}

void idSysLocal::DLL_GetFileName( const char* baseName, char* dllName, int maxLength )
{
	idStr::snPrintf( dllName, maxLength, "%s" CPUSTRING LIBRARYSUFFIX, baseName );
}

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
=================
Sys_TimeStampToStr
=================
*/
const char* Sys_TimeStampToStr( ID_TIME_T timeStamp )
{
	static char timeString[MAX_STRING_CHARS];
	timeString[0] = '\0';

	tm*	time = localtime( &timeStamp );
	idStr out;

	idStr lang = cvarSystem->GetCVarString( "sys_lang" );
	if( lang.Icmp( "english" ) == 0 )
	{
		// english gets "month/day/year  hour:min" + "am" or "pm"
		out = va( "%02d", time->tm_mon + 1 );
		out += "/";
		out += va( "%02d", time->tm_mday );
		out += "/";
		out += va( "%d", time->tm_year + 1900 );
		out += "\t";
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
		out += "\t";
		out += va( "%02d", time->tm_hour );
		out += ":";
		out += va( "%02d", time->tm_min );
	}
	idStr::Copynz( timeString, out, sizeof( timeString ) );

	return timeString;
}
