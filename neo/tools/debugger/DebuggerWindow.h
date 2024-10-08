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
#ifndef DEBUGGERWINDOW_H_
#define DEBUGGERWINDOW_H_

#ifndef DEBUGGERSCRIPT_H_
	#include "DebuggerScript.h"
#endif

class rvDebuggerWatch
{
public:

	idStr	mVariable;
	idStr	mValue;
	bool	mModified;
};

typedef idList<rvDebuggerWatch*>		rvDebuggerWatchList;

class rvDebuggerClient;

class rvDebuggerWindow
{
public:

	rvDebuggerWindow( );
	~rvDebuggerWindow( );

	bool							Create( HINSTANCE hInstance );

	static bool						Activate();

	void							ProcessNetMessage( idBitMsg* msg );

	void							Printf( const char* format, ... );

	HWND							GetWindow();

	void							AddWatch( const char* name, bool update = true );

	HINSTANCE						GetInstance();

private:
	bool							RegisterClass();
	void							CreateToolbar();
	bool							InitRecentFiles();

	int								HandleInitMenu( WPARAM wParam, LPARAM lParam );
	int								HandleCommand( WPARAM wParam, LPARAM lParam );
	int								HandleCreate( WPARAM wparam, LPARAM lparam );
	int								HandleActivate( WPARAM wparam, LPARAM lparam );
	int								HandleDrawItem( WPARAM wparam, LPARAM lparam );
	void							HandleTooltipGetDispInfo( WPARAM wparam, LPARAM lparam );

	void							ResizeImageList( int& widthOut, int& heightOut );
	static LRESULT					CALLBACK WndProc( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
	static LRESULT					CALLBACK MarginWndProc( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
	static LRESULT					CALLBACK ScriptWndProc( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
	static INT_PTR					CALLBACK AboutDlgProc( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
	static int						CALLBACK ScriptWordBreakProc( LPTSTR text, int current, int max, int action );

	bool							FindPrev( const char* text = NULL );
	bool							FindNext( const char* text = NULL );

	void							UpdateBreakpointList();
	void							UpdateScriptList();
	void							UpdateWatch();
	void							UpdateWindowMenu();
	void							UpdateScript();
	void							UpdateToolbar();
	void							UpdateTitle();
	void							UpdateCallstack();
	void							UpdateRecentFiles();
	bool							OpenScript( const char* filename, int lineNumber = -1, idProgram* program = NULL );
	void							EnableWindows( bool state );

	int								GetSelectedText( idStr& text );

	void							ToggleBreakpoint();
	float							GetMarginWidth();
	HWND							mWnd;
	HWND							mWndScript;
	HWND							mWndOutput;
	HWND							mWndMargin;
	HWND							mWndTabs;
	HWND							mWndBorder;
	HWND							mWndConsole;
	HWND							mWndConsoleInput;
	HWND							mWndCallstack;
	HWND							mWndScriptList;
	HWND							mWndBreakList; // list of breakpoints
	HWND							mWndWatch;
	HWND							mWndThreads;
	HWND							mWndToolTips;
	HWND							mWndToolbar;

	HMENU							mRecentFileMenu;
	int								mRecentFileInsertPos;

	WNDPROC							mOldWatchProc;
	WNDPROC							mOldScriptProc;
	idStr							mTooltipVar;
	idStr							mTooltipValue;

	HINSTANCE						mInstance;
	HIMAGELIST						mImageList;
	HIMAGELIST						mTmpImageList;

	RECT							mSplitterRect;
	bool							mSplitterDrag;

	idList<rvDebuggerScript*>		mScripts;
	int								mActiveScript;
	int								mLastActiveScript;
	int								mCurrentStackDepth;

	HMENU							mWindowMenu;
	int								mWindowMenuPos;

	int								mZoomScaleNum;
	int								mZoomScaleDem;
	int								mMarginSize;

	idStr							mFind;

	rvDebuggerClient*				mClient;

	rvDebuggerWatchList				mWatches;
};

/*
================
rvDebuggerWindow::GetWindow
================
*/
ID_INLINE HWND rvDebuggerWindow::GetWindow()
{
	return mWnd;
}

/*
================
rvDebuggerWindow::UpdateToolbar
================
*/
ID_INLINE void rvDebuggerWindow::UpdateToolbar()
{
	HandleInitMenu( ( WPARAM )GetMenu( mWnd ), 0 );
}

/*
================
rvDebuggerWindow::GetInstance
================
*/
ID_INLINE HINSTANCE rvDebuggerWindow::GetInstance()
{
	return mInstance;
}

#endif // DEBUGGERWINDOW_H_
