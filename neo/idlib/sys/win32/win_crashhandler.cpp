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

#pragma warning(push)
#pragma warning(disable: 4091)
#include "dbghelp.h"
#pragma warning(pop)

uint8 g_crashStackData[4096];
int g_crashStackLen = 0;
uint32 g_crashStackHash = 0;
EXCEPTION_POINTERS* g_exceptionPointers = NULL;
char g_crashLogPath[MAX_PATH] = {};

/*
====================
Sys_CaptureStackTrace
====================
*/
void Sys_CaptureStackTrace( int ignoreFrames, uint8* data, int& len )
{
	int cnt = CaptureStackBackTrace( ignoreFrames, len / sizeof( PVOID ), ( PVOID* )data, NULL );
	len = cnt * sizeof( PVOID );
}

/*
====================
Sys_GetStackTraceFramesCount
====================
*/
int Sys_GetStackTraceFramesCount( uint8* data, int len )
{
	return len / sizeof( PVOID );
}

/*
====================
Sys_GetStackTraceFramesCount
====================
*/
static bool AreSymbolsInitialized = false;
void Sys_DecodeStackTrace( uint8* data, int len, debugStackFrame_t* frames )
{
	//interpret input blob as array of addresses
	PVOID* addresses = ( PVOID* )data;
	int framesCount = Sys_GetStackTraceFramesCount( data, len );

	//fill output with zeros
	memset( frames, 0, framesCount * sizeof( frames[0] ) );

	HANDLE hProcess = GetCurrentProcess();
	if( !AreSymbolsInitialized )
	{
		AreSymbolsInitialized = true;
		SymInitialize( hProcess, NULL, TRUE );
	}

	//allocate symbol structures
	int buff[( sizeof( IMAGEHLP_SYMBOL64 ) + sizeof( frames[0].functionName ) ) / 4 + 1] = { 0 };
	IMAGEHLP_SYMBOL64* symbol = ( IMAGEHLP_SYMBOL64* )buff;
	symbol->SizeOfStruct = sizeof( IMAGEHLP_SYMBOL64 );
	symbol->MaxNameLength = sizeof( frames[0].functionName ) - 1;
	IMAGEHLP_LINE64 line = {0};
	line.SizeOfStruct = sizeof( IMAGEHLP_LINE64 );

	for( int i = 0; i < framesCount; i++ )
	{
		frames[i].pointer = addresses[i];
		sprintf( frames[i].functionName, "[%p]", frames[i].pointer );	//in case PDB not found

		if( !addresses[i] )
		{
			continue;	// null function?
		}

		if( !SymGetSymFromAddr64( hProcess, DWORD64( addresses[i] ), NULL, symbol ) )
		{
			continue;	// cannot get symbol
		}

		idStr::Copynz( frames[i].functionName, symbol->Name, sizeof( frames[0].functionName ) );

		DWORD displacement = DWORD( -1 );
		if( !SymGetLineFromAddr64( hProcess, DWORD64( addresses[i] ), &displacement, &line ) )
		{
			continue;	// no code line info
		}

		idStr::Copynz( frames[i].fileName, line.FileName, sizeof( frames[0].fileName ) );
		frames[i].lineNumber = line.LineNumber;
	}
}

/*
====================
GetExceptionCodeInfo

Returns a human-readable description of a Windows structured-exception code.
====================
*/
const char* GetExceptionCodeInfo( DWORD code )
{
	switch( code )
	{
		case EXCEPTION_ACCESS_VIOLATION:
			return "Access violation: the thread tried to read or write a "
				   "virtual address it does not have access to.";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			return "Array bounds exceeded: hardware detected an out-of-bounds "
				   "array access.";
		case EXCEPTION_BREAKPOINT:
			return "Breakpoint encountered.";
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return "Misaligned data: hardware does not support unaligned "
				   "reads/writes for this data type.";
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			return "FPU: denormal operand (value too small for a standard float).";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			return "FPU: floating-point divide by zero.";
		case EXCEPTION_FLT_INEXACT_RESULT:
			return "FPU: result cannot be represented exactly as a decimal.";
		case EXCEPTION_FLT_INVALID_OPERATION:
			return "FPU: invalid operation.";
		case EXCEPTION_FLT_OVERFLOW:
			return "FPU: overflow (exponent exceeds the type's max magnitude).";
		case EXCEPTION_FLT_STACK_CHECK:
			return "FPU: stack overflow or underflow.";
		case EXCEPTION_FLT_UNDERFLOW:
			return "FPU: underflow (exponent below the type's min magnitude).";
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			return "Illegal instruction: the thread tried to execute an invalid opcode.";
		case EXCEPTION_IN_PAGE_ERROR:
			return "In-page error: a needed page could not be loaded "
				   "(e.g. network disconnect while streaming).";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			return "Integer divide by zero.";
		case EXCEPTION_INT_OVERFLOW:
			return "Integer overflow: carry out of the most-significant bit.";
		case EXCEPTION_INVALID_DISPOSITION:
			return "Invalid exception handler disposition.";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			return "Execution attempted after a non-continuable exception.";
		case EXCEPTION_PRIV_INSTRUCTION:
			return "Privileged instruction: opcode not allowed in current CPU mode.";
		case EXCEPTION_SINGLE_STEP:
			return "Single-step: one instruction executed via trace trap.";
		case EXCEPTION_STACK_OVERFLOW:
			return "Stack overflow: the thread exhausted its stack.";
		default:
			return "Unknown exception code.";
	}
}

/*
====================
Sys_GetExceptionSummary
====================
*/
void Sys_GetExceptionSummary( char* buf, int bufSize )
{
	buf[0] = '\0';

	if( !g_exceptionPointers )
	{
		idStr::snPrintf( buf, bufSize,
						 "An unhandled exception has occurred.  Doom 3 BFG will now exit.\r\n\r\n"
						 "No exception information available." );
		return;
	}

	const EXCEPTION_RECORD* er = g_exceptionPointers->ExceptionRecord;

	idStr::snPrintf( buf, bufSize,
					 "An unhandled exception has occurred.  Doom 3 BFG will now exit.\r\n"
					 "\r\n"
					 "Exception : 0x%08X  at  0x%p\r\n"
					 "Info      : %s\r\n"
					 "\r\n"
					 "Crash log : %s",
					 ( unsigned int )er->ExceptionCode,
					 er->ExceptionAddress,
					 GetExceptionCodeInfo( er->ExceptionCode ),
					 g_crashLogPath[0] ? g_crashLogPath : "<check save folder>" );
}

/*
====================
Sys_CaptureExceptionStack
====================
*/
void Sys_CaptureExceptionStack( EXCEPTION_POINTERS* exceptionInfo )
{
	g_exceptionPointers = exceptionInfo;

	if( !exceptionInfo )
	{
		// Fallback: no context available, capture current stack as-is
		g_crashStackLen  = sizeof( g_crashStackData );
		g_crashStackHash = idDebugSystem::GetStack( g_crashStackData, g_crashStackLen );
		return;
	}

	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread  = GetCurrentThread();

	// Make a writable copy — StackWalk64 modifies the CONTEXT in place
	CONTEXT ctx = *exceptionInfo->ContextRecord;

	STACKFRAME64 sf = {};
#if defined( _WIN64 )
	const DWORD machineType   = IMAGE_FILE_MACHINE_AMD64;
	sf.AddrPC.Offset    = ctx.Rip;
	sf.AddrPC.Mode      = AddrModeFlat;
	sf.AddrFrame.Offset = ctx.Rbp;
	sf.AddrFrame.Mode   = AddrModeFlat;
	sf.AddrStack.Offset = ctx.Rsp;
	sf.AddrStack.Mode   = AddrModeFlat;
#else
	const DWORD machineType   = IMAGE_FILE_MACHINE_I386;
	sf.AddrPC.Offset    = ctx.Eip;
	sf.AddrPC.Mode      = AddrModeFlat;
	sf.AddrFrame.Offset = ctx.Ebp;
	sf.AddrFrame.Mode   = AddrModeFlat;
	sf.AddrStack.Offset = ctx.Esp;
	sf.AddrStack.Mode   = AddrModeFlat;
#endif

	// Ensure symbols are loaded before walking
	SymInitialize( hProcess, NULL, TRUE );

	// Walk the stack and store each frame PC as a PVOID (same layout that
	// Sys_DecodeStackTrace expects, since it also casts the buffer to PVOID[])
	PVOID* addresses  = reinterpret_cast<PVOID*>( g_crashStackData );
	int    maxFrames  = static_cast<int>( sizeof( g_crashStackData ) / sizeof( PVOID ) );
	int    frameCount = 0;

	while( frameCount < maxFrames )
	{
		if( !StackWalk64( machineType, hProcess, hThread,
						  &sf, &ctx,
						  NULL,                       // no custom memory reader
						  SymFunctionTableAccess64,
						  SymGetModuleBase64,
						  NULL ) )
		{
			break;
		}
		if( sf.AddrPC.Offset == 0 )
		{
			break;
		}
		addresses[frameCount++] = reinterpret_cast<PVOID>( sf.AddrPC.Offset );
	}

	g_crashStackLen = frameCount * static_cast<int>( sizeof( PVOID ) );

	// Compute a simple hash over the raw address data
	g_crashStackHash = idStr::Hash(
						   reinterpret_cast<const char*>( g_crashStackData ), g_crashStackLen );
	if( g_crashStackHash == 0 )
	{
		g_crashStackHash = static_cast<uint32>( -1 );
	}
}

/*
====================
Sys_WriteMiniDump
====================
*/
void Sys_WriteMiniDump( EXCEPTION_POINTERS* exceptionInfo )
{
	// Build the crashlogs directory path under the save path.
	// cvarSystem may not be ready during very early crashes, so guard it.
	char dumpDir[MAX_PATH] = {};
	if( cvarSystem && cvarSystem->IsInitialized() )
	{
		idStr::snPrintf( dumpDir, sizeof( dumpDir ), "%s\\crashlogs",
						 cvarSystem->GetCVarString( "fs_savepath" ) );
	}
	else
	{
		// Fallback: put crashlogs next to the executable
		GetModuleFileNameA( NULL, dumpDir, sizeof( dumpDir ) - 1 );
		char* lastSlash = strrchr( dumpDir, '\\' );
		if( lastSlash )
		{
			lastSlash[1] = '\0';
		}
		idStr::Append( dumpDir, sizeof( dumpDir ), "crashlogs" );
	}

	CreateDirectoryA( dumpDir, NULL );

	// Timestamp for the file names
	idStr::snPrintf( g_crashLogPath, sizeof( g_crashLogPath ), "%s", dumpDir );
	SYSTEMTIME st;
	GetLocalTime( &st );

	// Write minidump (.dmp)
	char dumpPath[MAX_PATH];
	idStr::snPrintf( dumpPath, sizeof( dumpPath ),
					 "%s\\crash_%04d-%02d-%02d_%02d-%02d-%02d.dmp",
					 dumpDir,
					 st.wYear, st.wMonth, st.wDay,
					 st.wHour, st.wMinute, st.wSecond );

	HANDLE hFile = CreateFileA( dumpPath, GENERIC_WRITE, 0, NULL,
								CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		MINIDUMP_EXCEPTION_INFORMATION exInfo = {};
		exInfo.ThreadId          = GetCurrentThreadId();
		exInfo.ExceptionPointers = exceptionInfo;
		exInfo.ClientPointers    = FALSE;

		MINIDUMP_TYPE dumpType = ( MINIDUMP_TYPE )(
									 MiniDumpWithDataSegs          |	// global/static variables
									 MiniDumpWithHandleData        |	// open handles
									 MiniDumpWithProcessThreadData |	// all thread stacks
									 MiniDumpWithUnloadedModules );	// recently unloaded DLLs

		MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hFile,
			dumpType,
			exceptionInfo ? &exInfo : NULL,
			NULL,
			NULL );
		CloseHandle( hFile );
	}

	char logPath[MAX_PATH];
	idStr::snPrintf( logPath, sizeof( logPath ),
					 "%s\\crash_%04d-%02d-%02d_%02d-%02d-%02d.txt",
					 dumpDir,
					 st.wYear, st.wMonth, st.wDay,
					 st.wHour, st.wMinute, st.wSecond );

	FILE* f = fopen( logPath, "w" );
	if( !f )
	{
		return;
	}

	fprintf( f, "\n--- Crash Summary :( ---\n" );

	// Engine version
	if( cvarSystem && cvarSystem->IsInitialized() )
	{
		fprintf( f, "Engine version : %s\n", com_version.GetString() );
	}
	fprintf( f, "Time           : %04d-%02d-%02d %02d:%02d:%02d\n\n",
			 st.wYear, st.wMonth, st.wDay,
			 st.wHour, st.wMinute, st.wSecond );

	if( exceptionInfo )
	{
		const EXCEPTION_RECORD* er  = exceptionInfo->ExceptionRecord;
		const CONTEXT* ctx = exceptionInfo->ContextRecord;

		fprintf( f, "Exception code    : 0x%08X\n", ( unsigned int )er->ExceptionCode );
		fprintf( f, "Exception address : 0x%p\n",    er->ExceptionAddress );
		fprintf( f, "Description       : %s\n\n",    GetExceptionCodeInfo( er->ExceptionCode ) );

#if defined( _WIN64 )
		fprintf( f,
				 "RAX=%016I64X  RBX=%016I64X  RCX=%016I64X  RDX=%016I64X\n"
				 "RSI=%016I64X  RDI=%016I64X  RBP=%016I64X  RSP=%016I64X\n"
				 "R8 =%016I64X  R9 =%016I64X  R10=%016I64X  R11=%016I64X\n"
				 "R12=%016I64X  R13=%016I64X  R14=%016I64X  R15=%016I64X\n"
				 "RIP=%016I64X  EFL=%08X\n\n",
				 ctx->Rax, ctx->Rbx, ctx->Rcx, ctx->Rdx,
				 ctx->Rsi, ctx->Rdi, ctx->Rbp, ctx->Rsp,
				 ctx->R8,  ctx->R9,  ctx->R10, ctx->R11,
				 ctx->R12, ctx->R13, ctx->R14, ctx->R15,
				 ctx->Rip, ctx->EFlags );
#else
		fprintf( f,
				 "EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X\n"
				 "ESI=%08X  EDI=%08X  EBP=%08X  ESP=%08X\n"
				 "EIP=%08X  EFL=%08X\n\n",
				 ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx,
				 ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp,
				 ctx->Eip, ctx->EFlags );
#endif
	}

	// Call stack
	if( g_crashStackLen > 0 )
	{
		idList<debugStackFrame_t, TAG_CRAP> frames;
		idDebugSystem::DecodeStack( g_crashStackData, g_crashStackLen, frames );
		idDebugSystem::CleanStack( frames );

		char stackStr[8192];
		idDebugSystem::StringifyStack( g_crashStackHash,
									   frames.Ptr(), frames.Num(), stackStr, sizeof( stackStr ) );
		fprintf( f, "%s\n", stackStr );
	}

	fprintf( f, "\n--- Report this crash ---\n"
			 "https://github.com/FriskTheFallenHuman/KROOM-3/issues\n" );

	fclose( f );
}
