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

#include "win_local.h"

extern HWND FindParentWindow();
extern HINSTANCE GetApplicationInstance();
INT_PTR CALLBACK CrashHandlerDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

/*
** Sys_UnhandledExceptionFilter
*/
LONG CALLBACK Sys_UnhandledExceptionFilter( EXCEPTION_POINTERS* exceptionInfo )
{
	static volatile LONG entered = 0;
	if( InterlockedExchange( &entered, 1 ) != 0 )
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}

	// Capture the call stack NOW.
	Sys_CaptureExceptionStack( exceptionInfo );

	// Write the minidump.
	Sys_WriteMiniDump( exceptionInfo );

	// Minimise the game window.
	//if ( win32.hWnd ) {
	//	ShowWindow( win32.hWnd, SW_MINIMIZE );
	//}
	Sys_ShowWindow( false );

	// Show the crash dialog.
	HWND hParent = FindParentWindow();
	HINSTANCE hInst = GetApplicationInstance();
	DialogBox( hInst, MAKEINTRESOURCE( 4001 /*IDD_CRASH_DIALOG*/ ), hParent, CrashHandlerDialogProc );

	// Let Windows terminate the process normally.
	return EXCEPTION_EXECUTE_HANDLER;
}