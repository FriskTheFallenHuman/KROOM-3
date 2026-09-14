/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software
Copyright (C) 2021 Harrie van Ginneken

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

#ifndef __DEBUGGERWINDOW_H__
#define __DEBUGGERWINDOW_H__

#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/stc/stc.h>
#include <wx/listctrl.h>
#include <wx/toolbar.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>

#ifndef __CONSOLEWIDGET_H__
	#include "../../common/ConsoleWidget.h"
#endif

#ifndef __TOOLBARSTRIP_H__
	#include "../../common/ToolBarStrip.h"
#endif

#ifndef __DEBUGGERSCRIPT_H__
	#include "../server/DebuggerScript.h"
#endif

#ifndef __DEBUGGERBREAKPOINT_H__
	#include "../server/DebuggerBreakpoint.h"
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

class rvDebuggerWindow : public wxFrame
{
public:
	rvDebuggerWindow();
	~rvDebuggerWindow();

	bool							Create();

	static bool						Activate();

	void							ProcessNetMessage( idBitMsg* msg );

	void							Printf( VERIFY_FORMAT_STRING const char* fmt, ... ) ID_STATIC_ATTRIBUTE_PRINTF( 1, 2 );

	void							AddWatch( const char* name, bool update = true );

protected:
	void							OnSize( wxSizeEvent& event );
	void							OnClose( wxCloseEvent& event );
	void							OnUpdateUI( wxUpdateUIEvent& event );
	void							OnMenuFileOpen( wxCommandEvent& event );
	void							OnMenuFileClose( wxCommandEvent& event );
	void							OnMenuFileExit( wxCommandEvent& event );
	void							OnMenuEditFind( wxCommandEvent& event );
	void							OnMenuDebugRun( wxCommandEvent& event );
	void							OnMenuDebugBreak( wxCommandEvent& event );
	void							OnMenuDebugStepOver( wxCommandEvent& event );
	void							OnMenuDebugStepInto( wxCommandEvent& event );
	void							OnMenuDebugStepOut( wxCommandEvent& event );
	void							OnMenuDebugToggleBreakpoint( wxCommandEvent& event );
	void							OnMenuDebugQuickWatch( wxCommandEvent& event );
	void							OnMenuDebugRunToCursor( wxCommandEvent& event );
	void							OnMenuDebugShowNextStatement( wxCommandEvent& event );
	void							OnMenuWindowCloseAll( wxCommandEvent& event );
	void							OnMenuWindowSelect( wxCommandEvent& event );
	void							OnMenuHelpAbout( wxCommandEvent& event );
	void							OnMenuRecentFile( wxCommandEvent& event );
	void							OnToolbarRun( wxCommandEvent& event );
	void							OnToolbarBreak( wxCommandEvent& event );
	void							OnToolbarStepOver( wxCommandEvent& event );
	void							OnToolbarStepInto( wxCommandEvent& event );
	void							OnToolbarStepOut( wxCommandEvent& event );
	void							OnToolbarOpen( wxCommandEvent& event );
	void							OnScriptMarginClick( wxStyledTextEvent& event );
	void							OnScriptMouseHover( wxStyledTextEvent& event );
	void							OnScriptKeyDown( wxKeyEvent& event );
	void							OnScriptContextMenu( wxContextMenuEvent& event );
	void							OnScriptTabChanged( wxBookCtrlEvent& event );
	void							OnCallstackItemActivated( wxListEvent& event );
	void							OnScriptsItemActivated( wxListEvent& event );
	void							OnBreakpointsItemActivated( wxListEvent& event );
	void							OnBreakpointsKeyDown( wxListEvent& event );
	void							OnWatchBeginLabelEdit( wxListEvent& event );
	void							OnWatchEndLabelEdit( wxListEvent& event );
	void							OnWatchKeyDown( wxListEvent& event );
	void							OnNotebookPageChanged( wxNotebookEvent& event );
	void							OnConsoleCommand( const char* cmd );

private:
	wxStyledTextCtrl*				CreateScriptEditor();

	void							RefreshBreakpointMarkers();
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

	bool							FindPrev( const char* text = NULL );
	bool							FindNext( const char* text = NULL );

	void							CreateMenuBar();
	void							CreateToolbar();
	bool							InitRecentFiles();

	idStr							GetServerScriptName( const char* localFilename ) const;

private:
	enum
	{
		ID_DBG_RUN = 1000,
		ID_DBG_BREAK,
		ID_DBG_STEPINTO,
		ID_DBG_STEPOVER,
		ID_DBG_STEPOUT,
		ID_DBG_TOGGLEBREAKPOINT,
		ID_DBG_OPEN,
		ID_DBG_CLOSE,
		ID_DBG_CLOSEALL,
		ID_DBG_NEXT,
		ID_DBG_FIND,
		ID_DBG_FINDNEXT,
		ID_DBG_FINDPREV,
		ID_DBG_FINDSELECTED,
		ID_DBG_QUICKWATCH,
		ID_DBG_RUNTOCURSOR,
		ID_DBG_SHOWNEXTSTATEMENT,
		ID_DBG_ABOUT,
		ID_DBG_FILE_MRU1,
		ID_DBG_WINDOW_MIN,
		ID_DBG_WINDOW_MAX,

		ID_DBG_FILE_MRU,
		ID_DBG_FILE_OPEN,
		ID_DBG_FILE_OPENSCRIPT,
		ID_DBG_FILE_EXIT,
		ID_DBG_WINDOW_CLOSEALL,
		ID_DBG_DEBUG_TOGGLEBREAKPOINT,
		ID_DBG_DEBUG_RUN,
		ID_DBG_DEBUG_BREAK,
		ID_DBG_FILE_NEXT,
		ID_DBG_DEBUG_STEPOVER,
		ID_DBG_FILE_CLOSE,
		ID_DBG_DEBUG_STEPINTO,
		ID_DBG_DEBUG_STEPOUT,
		ID_DBG_DEBUG_QUICKWATCH,
		ID_DBG_DEBUG_RUNTOCURSOR,
		ID_DBG_DEBUG_SHOWNEXTSTATEMENT,
		ID_DBG_EDIT_COPY,
		ID_DBG_EDIT_CUT,
		ID_DBG_EDIT_PASTE,
		ID_DBG_EDIT_FIND,
		ID_DBG_EDIT_FINDSELECTED,
		ID_DBG_EDIT_FINDNEXT,
		ID_DBG_EDIT_FINDPREV,
		ID_DBG_EDIT_FINDSELECTEDPREV,
		ID_DBG_HELP_ABOUT,
		ID_DBG_SEND_COMMAND,

		ID_DBG_TOOLBAR_RUN,
		ID_DBG_TOOLBAR_BREAK,
		ID_DBG_TOOLBAR_STEPOVER,
		ID_DBG_TOOLBAR_STEPINTO,
		ID_DBG_TOOLBAR_STEPOUT,
		ID_DBG_TOOLBAR_OPEN,

		ID_DBG_RECENT_FIRST,
		ID_DBG_RECENT_LAST = ID_DBG_RECENT_FIRST + 9,
	};

	static rvDebuggerWindow*		s_instance;

	wxStyledTextCtrl*				mWndScript;
	wxTextCtrl*						mWndOutput;
	wxNotebook*						mWndTabs;
	ConsoleWidget*                  m_consoleWidget;
	wxListView*						mWndCallstack;
	wxListView*						mWndScriptList;
	wxListView*						mWndBreakList;
	wxListView*						mWndWatch;
	wxListView*						mWndThreads;
	wxToolBar*						mWndToolbar;
	wxSplitterWindow*				mSplitter;
	wxNotebook*						mScriptTabs;

	bool							mTabSync;

	int								mRecentFileInsertPos;
	int								mWindowMenuPos;

	idList<rvDebuggerScript*>		mScripts;
	int								mActiveScript;
	int								mCurrentStackDepth;

	idStr							mFind;

	rvDebuggerClient*				mClient;

	rvDebuggerWatchList				mWatches;

	int								mZoomScaleNum;
	int								mZoomScaleDem;
	int								mMarginSize;

	wxMenu*							mRecentFileMenuWx;
	wxMenu*							mWindowMenuWx;

	rvToolbarImageStrip				mToolbarIcons;

	DECLARE_EVENT_TABLE()
};

#endif // __DEBUGGERWINDOW_H__