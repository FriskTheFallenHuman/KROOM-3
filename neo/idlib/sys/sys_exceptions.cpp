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

#include <string>
#include <map>

#ifdef _WIN32
	#include <CommCtrl.h>
	#include <psapi.h>
	#undef StrCmpI
	#undef StrCmpNI
	#undef StrCmpN
	#include <shlwapi.h>
#endif

#ifdef _WIN32
extern uint8 g_crashStackData[4096];
extern int g_crashStackLen;
extern uint32 g_crashStackHash;
extern EXCEPTION_POINTERS* g_exceptionPointers;
extern const char* GetExceptionCodeInfo( DWORD code );

extern void Sys_GetExceptionSummary( char* buf, int bufSize );

extern HWND FindParentWindow();
extern HINSTANCE GetApplicationInstance();

// Tab indices
enum CrashTab
{
	TAB_CALLSTACK = 0,
	TAB_EXCEPTION = 1,
	TAB_REGISTERS = 2,
	TAB_COUNT = 3
};

/*
========================
BuildGitHubURL
========================
*/
static void BuildGitHubURL( char* urlBuf, int urlBufSize )
{
	if( g_exceptionPointers )
	{
		DWORD code = g_exceptionPointers->ExceptionRecord->ExceptionCode;
		idStr::snPrintf( urlBuf, urlBufSize,
						 "https://github.com/FriskTheFallenHuman/KROOM-3/issues/new"
						 "?title=Crash%%3A+0x%08X&labels=crash",
						 ( unsigned int )code );
	}
	else
	{
		idStr::snPrintf( urlBuf, urlBufSize,
						 "https://github.com/FriskTheFallenHuman/KROOM-3/issues/new"
						 "?title=Crash+Report&labels=crash" );
	}
}

/*
========================
BuildRegisterText

Formats CPU register state as \r\n-delimited text for the EDITTEXT control.
========================
*/
static void BuildRegisterText( char* buf, int bufSize )
{
	buf[0] = '\0';

	if( !g_exceptionPointers )
	{
		idStr::snPrintf( buf, bufSize, "No register information available." );
		return;
	}

	const CONTEXT* ctx = g_exceptionPointers->ContextRecord;

#if defined( _WIN64 )
	idStr::snPrintf( buf, bufSize,
					 "General-Purpose Registers\r\n"
					 "------------------------------------------------------------\r\n"
					 "RAX = %016I64X    RBX = %016I64X\r\n"
					 "RCX = %016I64X    RDX = %016I64X\r\n"
					 "RSI = %016I64X    RDI = %016I64X\r\n"
					 "RBP = %016I64X    RSP = %016I64X\r\n"
					 "R8  = %016I64X    R9  = %016I64X\r\n"
					 "R10 = %016I64X    R11 = %016I64X\r\n"
					 "R12 = %016I64X    R13 = %016I64X\r\n"
					 "R14 = %016I64X    R15 = %016I64X\r\n"
					 "\r\n"
					 "Control Registers\r\n"
					 "------------------------------------------------------------\r\n"
					 "RIP = %016I64X    EFL = %08X\r\n",
					 ctx->Rax, ctx->Rbx, ctx->Rcx, ctx->Rdx,
					 ctx->Rsi, ctx->Rdi, ctx->Rbp, ctx->Rsp,
					 ctx->R8,  ctx->R9,  ctx->R10, ctx->R11,
					 ctx->R12, ctx->R13, ctx->R14, ctx->R15,
					 ctx->Rip, ctx->EFlags );
#else
	idStr::snPrintf( buf, bufSize,
					 "General-Purpose Registers\r\n"
					 "----------------------------------------\r\n"
					 "EAX = %08X    EBX = %08X\r\n"
					 "ECX = %08X    EDX = %08X\r\n"
					 "ESI = %08X    EDI = %08X\r\n"
					 "EBP = %08X    ESP = %08X\r\n"
					 "\r\n"
					 "Control Registers\r\n"
					 "----------------------------------------\r\n"
					 "EIP = %08X    EFL = %08X\r\n"
					 "\r\n"
					 "Segment Registers\r\n"
					 "----------------------------------------\r\n"
					 "CS  = %04X    SS  = %04X\r\n"
					 "DS  = %04X    ES  = %04X\r\n"
					 "FS  = %04X    GS  = %04X\r\n",
					 ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx,
					 ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp,
					 ctx->Eip, ctx->EFlags,
					 ctx->SegCs, ctx->SegSs,
					 ctx->SegDs, ctx->SegEs,
					 ctx->SegFs, ctx->SegGs );
#endif
}

/*
========================
// InsertExceptionInfoRow
========================
*/
static void InsertExceptionInfoRow( HWND hList, int row,
									const char* prop, const char* value )
{
	char buf[512];
	LVITEM lvi = {};
	lvi.mask     = LVIF_TEXT;
	lvi.iItem    = row;
	lvi.iSubItem = 0;
	idStr::Copynz( buf, prop, sizeof( buf ) );
	lvi.pszText  = buf;
	ListView_InsertItem( hList, &lvi );

	lvi.iSubItem = 1;
	idStr::Copynz( buf, value, sizeof( buf ) );
	lvi.pszText  = buf;
	ListView_SetItem( hList, &lvi );
}

// ---------------------------------------------------------------------------
// PopulateExceptionView
//
// Fills the Exception Info ListView (ID_EXCEPTION_VIEW) with a property /
// value table describing the exception, the engine version, the thread/
// process IDs, and — for access violations — the access type and target
// address.
// ---------------------------------------------------------------------------
static void PopulateExceptionView( HWND hList )
{
	// Two columns: Property and Value
	LVCOLUMN lvc = {};
	lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvc.cx   = 140;
	lvc.pszText = "Property";
	ListView_InsertColumn( hList, 0, &lvc );
	lvc.cx   = 200;
	lvc.pszText = "Value";
	ListView_InsertColumn( hList, 1, &lvc );

	ListView_SetExtendedListViewStyle( hList,
									   LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES );

	int row = 0;
	char val[512];

	// Engine version (if available)
	if( cvarSystem && cvarSystem->IsInitialized() )
	{
		InsertExceptionInfoRow( hList, row++, "Engine Version",
								com_version.GetString() );
	}

	// Process / thread
	idStr::snPrintf( val, sizeof( val ), "%lu", GetCurrentProcessId() );
	InsertExceptionInfoRow( hList, row++, "Process ID", val );

	idStr::snPrintf( val, sizeof( val ), "%lu", GetCurrentThreadId() );
	InsertExceptionInfoRow( hList, row++, "Thread ID", val );

	if( !g_exceptionPointers )
	{
		InsertExceptionInfoRow( hList, row++, "Exception", "No information available" );
		return;
	}

	const EXCEPTION_RECORD* er = g_exceptionPointers->ExceptionRecord;

	// Exception code (hex)
	idStr::snPrintf( val, sizeof( val ), "0x%08X", ( unsigned int )er->ExceptionCode );
	InsertExceptionInfoRow( hList, row++, "Exception Code", val );

	// Exception address
	idStr::snPrintf( val, sizeof( val ), "0x%p", er->ExceptionAddress );
	InsertExceptionInfoRow( hList, row++, "Exception Address", val );

	// Description
	InsertExceptionInfoRow( hList, row++, "Description",
							GetExceptionCodeInfo( er->ExceptionCode ) );

	// Access violations carry extra info in ExceptionInformation[]
	if( er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
			er->NumberParameters >= 2 )
	{
		InsertExceptionInfoRow( hList, row++, "Access Type",
								er->ExceptionInformation[0] == 0 ? "Read"
								: er->ExceptionInformation[0] == 1 ? "Write"
								:                                     "Execute (DEP)" );

		idStr::snPrintf( val, sizeof( val ), "0x%p",
						 reinterpret_cast<void*>( er->ExceptionInformation[1] ) );
		InsertExceptionInfoRow( hList, row++, "Bad Address", val );
	}

	// Crash log path
	extern char g_crashLogPath[MAX_PATH];
	if( g_crashLogPath[0] )
	{
		InsertExceptionInfoRow( hList, row++, "Crash Log Folder", g_crashLogPath );
	}
}

// ShowTab
static void ShowTab( HWND hDlg, int sel )
{
	ShowWindow( GetDlgItem( hDlg, /*ID_STACK_VIEW*/ 4005 ),      sel == TAB_CALLSTACK ? SW_SHOW : SW_HIDE );
	ShowWindow( GetDlgItem( hDlg, /*ID_EXCEPTION_VIEW*/ 4007 ), sel == TAB_EXCEPTION ? SW_SHOW : SW_HIDE );
	ShowWindow( GetDlgItem( hDlg, /*IDC_REGISTER_EDIT*/ 4021 ), sel == TAB_REGISTERS ? SW_SHOW : SW_HIDE );

	// Set keyboard focus to the newly visible control
	int focusId = ( sel == TAB_CALLSTACK ) ? /*ID_STACK_VIEW*/ 4005
				  : ( sel == TAB_EXCEPTION ) ? /*ID_EXCEPTION_VIEW*/ 4007
				  :                          /*IDC_REGISTER_EDIT*/ 4021;
	SetFocus( GetDlgItem( hDlg, focusId ) );
}

/*
========================
CrashHandlerDialogProc

Shamelessly copied from DoomEdit

@inputs: HWND hDlg - handle to dialog box
@inputs: UINT uMsg - message
@inputs: WPARAM wParam - first message parameter
@inputs: LPARAM lParam - second message parameter
========================
*/
INT_PTR CALLBACK CrashHandlerDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
		case WM_INITDIALOG:
		{
			RECT rcDlg, rcDesktop;
			GetWindowRect( hDlg, &rcDlg );
			GetWindowRect( GetDesktopWindow(), &rcDesktop );
			SetWindowPos( hDlg, HWND_TOPMOST,
						  ( rcDesktop.right  - ( rcDlg.right  - rcDlg.left ) ) / 2,
						  ( rcDesktop.bottom - ( rcDlg.bottom - rcDlg.top ) ) / 2,
						  0, 0, SWP_NOSIZE );
			SetForegroundWindow( hDlg );

			// summary text
			char summary[1024];
			Sys_GetExceptionSummary( summary, sizeof( summary ) );
			SetDlgItemTextA( hDlg, /*ID_CRASH_TEXT*/ 4012, summary );

			HWND hTab = GetDlgItem( hDlg, /*ID_CRASH_TABS*/ 4002 );
			TCITEM tie = {};
			tie.mask    = TCIF_TEXT;
			tie.pszText = "Call Stack";
			TabCtrl_InsertItem( hTab, TAB_CALLSTACK, &tie );
			tie.pszText = "Exception Info";
			TabCtrl_InsertItem( hTab, TAB_EXCEPTION, &tie );
			tie.pszText = "CPU Registers";
			TabCtrl_InsertItem( hTab, TAB_REGISTERS, &tie );

			// Call Stack
			HWND hStackList = GetDlgItem( hDlg, /*ID_STACK_VIEW*/ 4005 );
			ListView_SetExtendedListViewStyle( hStackList,
											   LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_AUTOSIZECOLUMNS );
			{
				LVCOLUMN lvc = {};
				lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
				lvc.cx = 240;
				lvc.pszText = "Function";
				ListView_InsertColumn( hStackList, 0, &lvc );
				lvc.cx =  50;
				lvc.pszText = "Line";
				ListView_InsertColumn( hStackList, 1, &lvc );
				lvc.cx = 200;
				lvc.pszText = "Source File";
				ListView_InsertColumn( hStackList, 2, &lvc );
				lvc.cx = 150;
				lvc.pszText = "Module";
				ListView_InsertColumn( hStackList, 3, &lvc );
			}

			// Decode the crash stack
			idList<debugStackFrame_t, TAG_CRAP> frames;
			if( g_crashStackLen > 0 )
			{
				idDebugSystem::DecodeStack( g_crashStackData, g_crashStackLen, frames );
				idDebugSystem::CleanStack( frames );
				if( frames.Num() > 0 )
				{
					frames.RemoveIndex( 0 );
				}
				if( frames.Num() > 0 )
				{
					frames.RemoveIndex( 0 );
				}
			}

			// Build module map
			std::map<void*, std::string> moduleMap;
			{
				HMODULE hMods[1024];
				DWORD   cbNeeded;
				if( EnumProcessModules( GetCurrentProcess(), hMods,
										sizeof( hMods ), &cbNeeded ) )
				{
					for( unsigned i = 0; i < cbNeeded / sizeof( HMODULE ); i++ )
					{
						char name[MAX_PATH];
						if( GetModuleFileNameExA( GetCurrentProcess(), hMods[i],
												  name, sizeof( name ) ) )
						{
							moduleMap[ reinterpret_cast<void*>( hMods[i] ) ] =
								PathFindFileName( name );
						}
					}
				}
			}

			for( int i = 0; i < frames.Num(); i++ )
			{
				const debugStackFrame_t& fr = frames[i];
				char buf[256];
				LVITEM lvi = {};
				lvi.mask  = LVIF_TEXT;
				lvi.iItem = i;

				// Function name
				lvi.iSubItem = 0;
				idStr::Copynz( buf,
							   ( idStr::Cmp( fr.functionName, "[UNNAMED]" ) == 0 )
							   ? "**UNKNOWN**" : fr.functionName,
							   sizeof( buf ) );
				lvi.pszText = buf;
				ListView_InsertItem( hStackList, &lvi );

				// Line number
				lvi.iSubItem = 1;
				idStr::snPrintf( buf, sizeof( buf ),
								 fr.lineNumber <= 0 ? "0" : "%d", fr.lineNumber );
				lvi.pszText = buf;
				ListView_SetItem( hStackList, &lvi );

				// Source file (or raw pointer if no debug info)
				lvi.iSubItem = 2;
				if( idStr::Cmp( fr.fileName, "[UNNAMED]" ) == 0 || !strlen( fr.fileName ) )
				{
					idStr::snPrintf( buf, sizeof( buf ), "0x%p", fr.pointer );
				}
				else
				{
					idStr::Copynz( buf, fr.fileName, sizeof( buf ) );
				}
				lvi.pszText = buf;
				ListView_SetItem( hStackList, &lvi );

				// Module: walk back from frame pointer to find enclosing module
				auto it = moduleMap.lower_bound( fr.pointer );
				if( it != moduleMap.begin() &&
						( it == moduleMap.end() || it->first != fr.pointer ) )
				{
					--it;
				}
				lvi.iSubItem = 3;
				lvi.pszText  = ( it != moduleMap.end() )
							   ? const_cast<char*>( it->second.c_str() )
							   : const_cast<char*>( "[UNKNOWN]" );
				ListView_SetItem( hStackList, &lvi );
			}

			PopulateExceptionView( GetDlgItem( hDlg, /*ID_EXCEPTION_VIEW*/ 4007 ) );

			{
				HWND hRegEdit = GetDlgItem( hDlg, /*IDC_REGISTER_EDIT*/ 4021 );

				HFONT hMono = CreateFontA(
								  -13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
								  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
								  DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New" );
				if( hMono )
				{
					SendMessage( hRegEdit, WM_SETFONT, ( WPARAM )hMono, TRUE );
				}

				char regText[2048];
				BuildRegisterText( regText, sizeof( regText ) );
				SetWindowTextA( hRegEdit, regText );
			}

			ShowTab( hDlg, TAB_CALLSTACK );
			return FALSE;
		}

		case WM_NOTIFY:
		{
			NMHDR* pnmhdr = reinterpret_cast<NMHDR*>( lParam );
			if( pnmhdr->idFrom == /*ID_CRASH_TABS*/ 4002 &&
					pnmhdr->code   == TCN_SELCHANGE )
			{
				ShowTab( hDlg, TabCtrl_GetCurSel( pnmhdr->hwndFrom ) );
			}
			return FALSE;
		}

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case IDCLOSE:
				{
					EndDialog( hDlg, 0 );
					return TRUE;
				}
				case /*IDC_REPORT_GITHUB*/ 4020:
				{
					char url[512];
					BuildGitHubURL( url, sizeof( url ) );
					ShellExecuteA( hDlg, "open", url, NULL, NULL, SW_SHOWNORMAL );
					return TRUE;
				}
			}
			return FALSE;
		}

		case WM_MOUSEWHEEL:
		{
			HWND hFocused = GetFocus();
			if( hFocused )
			{
				SendMessage( hFocused, WM_MOUSEWHEEL, wParam, lParam );
			}
			return TRUE;
		}

		case WM_KEYDOWN:
		{
			if( wParam == VK_ESCAPE )
			{
				EndDialog( hDlg, 0 );
				return TRUE;
			}
			return FALSE;
		}

	} // switch uMsg

	return FALSE;
}
#endif

/*
================================================
idFatalException
================================================
*/
idFatalException::idFatalException( const char* text, bool doStackTrace, bool emergencyExit )
{
	strncpy( idException::error, text, MAX_ERROR_LEN );

	// Print our stack trace
	if( doStackTrace )
	{
		idLib::PrintCallStack();
	}

#ifdef _WIN32
	if( !idLib::IsMainThread() )
	{
		int bRet = MessageBox( NULL, text, GAME_NAME " Unhandled Exception", MB_SYSTEMMODAL | MB_CANCELTRYCONTINUE );
		if( bRet == IDCANCEL )
		{
		}
		else if( bRet == IDCONTINUE )
		{
		}
	}
	else
	{
		HWND hParentWindow = FindParentWindow();
		HINSTANCE hParentInstance = GetApplicationInstance();
		DialogBox( hParentInstance, MAKEINTRESOURCE( 4001 /*IDD_CRASH_DIALOG*/ ), hParentWindow, CrashHandlerDialogProc );
	}
#endif

	// During emergency exiting, do not botter printing or do the error just quit
	if( emergencyExit )
	{
		Sys_Printf( "%s\n", text );

		// write the console to a log file?
		Sys_Quit();
	}
	else
	{
		Sys_Printf( "shutting down: %s\n", text );
		Sys_Error( "%s", text );
	}
}