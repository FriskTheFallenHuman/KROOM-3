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

#include "precompiled.h"
#pragma hdrstop

#include "DebuggerApp.h"
#include "DebuggerQuickWatchDlg.h"
#include "DebuggerFindDlg.h"

#ifndef _MSC_VER
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	#if defined(__MINGW32__) || defined(__MINGW64__)
		// MingW
		#include <process.h>
		#include <direct.h>
		#include <errno.h>
	#else
		// POSIX (Linux, macOS, *nix)
		#include <unistd.h>
		#include <sys/types.h>
		#include <sys/wait.h>
		#include <limits.h>
		#include <errno.h>
	#endif
#endif

wxBEGIN_EVENT_TABLE( rvDebuggerWindow, wxFrame )
	EVT_SIZE( rvDebuggerWindow::OnSize )
	EVT_CLOSE( rvDebuggerWindow::OnClose )
	EVT_UPDATE_UI( wxID_ANY, rvDebuggerWindow::OnUpdateUI )

	EVT_MENU( ID_DBG_FILE_OPEN, rvDebuggerWindow::OnMenuFileOpen )
	EVT_MENU( ID_DBG_FILE_CLOSE, rvDebuggerWindow::OnMenuFileClose )
	EVT_MENU( ID_DBG_FILE_EXIT, rvDebuggerWindow::OnMenuFileExit )
	EVT_MENU( ID_DBG_EDIT_FIND, rvDebuggerWindow::OnMenuEditFind )
	EVT_MENU( ID_DBG_DEBUG_RUN, rvDebuggerWindow::OnMenuDebugRun )
	EVT_MENU( ID_DBG_DEBUG_BREAK, rvDebuggerWindow::OnMenuDebugBreak )
	EVT_MENU( ID_DBG_DEBUG_STEPOVER, rvDebuggerWindow::OnMenuDebugStepOver )
	EVT_MENU( ID_DBG_DEBUG_STEPINTO, rvDebuggerWindow::OnMenuDebugStepInto )
	EVT_MENU( ID_DBG_DEBUG_STEPOUT, rvDebuggerWindow::OnMenuDebugStepOut )
	EVT_MENU( ID_DBG_DEBUG_TOGGLEBREAKPOINT, rvDebuggerWindow::OnMenuDebugToggleBreakpoint )
	EVT_MENU( ID_DBG_DEBUG_QUICKWATCH, rvDebuggerWindow::OnMenuDebugQuickWatch )
	EVT_MENU( ID_DBG_DEBUG_RUNTOCURSOR, rvDebuggerWindow::OnMenuDebugRunToCursor )
	EVT_MENU( ID_DBG_DEBUG_SHOWNEXTSTATEMENT, rvDebuggerWindow::OnMenuDebugShowNextStatement )
	EVT_MENU( ID_DBG_WINDOW_CLOSEALL, rvDebuggerWindow::OnMenuWindowCloseAll )
	EVT_MENU( ID_DBG_HELP_ABOUT, rvDebuggerWindow::OnMenuHelpAbout )

	EVT_MENU_RANGE( ID_DBG_WINDOW_MIN, ID_DBG_WINDOW_MAX, rvDebuggerWindow::OnMenuWindowSelect )

	EVT_MENU( ID_DBG_TOOLBAR_OPEN, rvDebuggerWindow::OnToolbarOpen )
	EVT_MENU( ID_DBG_TOOLBAR_RUN, rvDebuggerWindow::OnToolbarRun )
	EVT_MENU( ID_DBG_TOOLBAR_BREAK, rvDebuggerWindow::OnToolbarBreak )
	EVT_MENU( ID_DBG_TOOLBAR_STEPOVER, rvDebuggerWindow::OnToolbarStepOver )
	EVT_MENU( ID_DBG_TOOLBAR_STEPINTO, rvDebuggerWindow::OnToolbarStepInto )
	EVT_MENU( ID_DBG_TOOLBAR_STEPOUT, rvDebuggerWindow::OnToolbarStepOut )

	EVT_MENU_RANGE( ID_DBG_RECENT_FIRST, ID_DBG_RECENT_LAST, rvDebuggerWindow::OnMenuRecentFile )

	EVT_NOTEBOOK_PAGE_CHANGED( wxID_ANY, rvDebuggerWindow::OnNotebookPageChanged )

	EVT_STC_MARGINCLICK( wxID_ANY, rvDebuggerWindow::OnScriptMarginClick )

	EVT_LIST_ITEM_ACTIVATED( wxID_ANY, rvDebuggerWindow::OnCallstackItemActivated )
	EVT_LIST_ITEM_ACTIVATED( wxID_ANY, rvDebuggerWindow::OnScriptsItemActivated )
	EVT_LIST_ITEM_ACTIVATED( wxID_ANY, rvDebuggerWindow::OnBreakpointsItemActivated )
	EVT_LIST_KEY_DOWN( wxID_ANY, rvDebuggerWindow::OnBreakpointsKeyDown )
	EVT_LIST_BEGIN_LABEL_EDIT( wxID_ANY, rvDebuggerWindow::OnWatchBeginLabelEdit )
	EVT_LIST_END_LABEL_EDIT( wxID_ANY, rvDebuggerWindow::OnWatchEndLabelEdit )
	EVT_LIST_KEY_DOWN( wxID_ANY, rvDebuggerWindow::OnWatchKeyDown )
wxEND_EVENT_TABLE()

rvDebuggerWindow* rvDebuggerWindow::s_instance = nullptr;

/*
================
rvDebuggerWindow::rvDebuggerWindow
================
*/
rvDebuggerWindow::rvDebuggerWindow()
	: wxFrame( NULL, wxID_ANY, GAME_NAME " Script Debugger", wxDefaultPosition, wxSize( 800, 600 ) )
{
	mWndScript = NULL;
	mWndOutput = NULL;
	mWndTabs = NULL;
	m_consoleWidget = NULL;
	mWndCallstack = NULL;
	mWndScriptList = NULL;
	mWndBreakList = NULL;
	mWndWatch = NULL;
	mWndThreads = NULL;
	mWndToolbar = NULL;
	mSplitter = NULL;
	mScriptTabs = NULL;
	mTabSync = false;

	mRecentFileMenuWx = NULL;
	mWindowMenuWx = NULL;
	mRecentFileInsertPos = 0;
	mWindowMenuPos = 0;

	mClient = NULL;
	mActiveScript = 0;
	mCurrentStackDepth = 0;

	mZoomScaleNum = 0;
	mZoomScaleDem = 0;
	mMarginSize = 0;
}

/*
================
rvDebuggerWindow::~rvDebuggerWindow
================
*/
rvDebuggerWindow::~rvDebuggerWindow()
{
	for( int i = 0; i < mScripts.Num(); i++ )
	{
		delete mScripts[i];
	}

	s_instance = nullptr;

	gDebuggerApp.NotifyWindowDestroyed( this );
}

/*
================
rvDebuggerWindow::Create
================
*/
bool rvDebuggerWindow::Create()
{
	s_instance = this;

	mClient = &gDebuggerApp.GetClient();

	CreateMenuBar();

	CreateToolbar();

	CreateStatusBar( 1 );
	SetStatusText( "Ready" );

	mSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE );
	mSplitter->SetMinimumPaneSize( 50 );

	mScriptTabs = new wxNotebook( mSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP | wxNB_MULTILINE );
	mScriptTabs->Bind( wxEVT_NOTEBOOK_PAGE_CHANGED, &rvDebuggerWindow::OnScriptTabChanged, this );
	mScriptTabs->Hide();

	mWndTabs = new wxNotebook( mSplitter, wxID_ANY );

	mSplitter->Initialize( mWndTabs );

	mWndOutput = new wxTextCtrl( mWndTabs, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH );
	mWndTabs->AddPage( mWndOutput, "Output" );

	m_consoleWidget = new ConsoleWidget( mWndTabs );
	m_consoleWidget->SetCommandHandler( [this]( const char* cmd )
	{
		this->OnConsoleCommand( cmd );
	} );
	m_consoleWidget->SetQuitHandler( [this]()
	{
		this->Close( true );
	} );
	mWndTabs->AddPage( m_consoleWidget, "Console" );

	mWndCallstack = new wxListView( mWndTabs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
	mWndCallstack->AppendColumn( "Function", wxLIST_FORMAT_LEFT, 150 );
	mWndCallstack->AppendColumn( "Line", wxLIST_FORMAT_LEFT, 50 );
	mWndCallstack->AppendColumn( "Filename", wxLIST_FORMAT_LEFT, 350 );
	mWndTabs->AddPage( mWndCallstack, "Call Stack" );

	mWndWatch = new wxListView( mWndTabs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
	mWndWatch->AppendColumn( "Name", wxLIST_FORMAT_LEFT, 150 );
	mWndWatch->AppendColumn( "Value", wxLIST_FORMAT_LEFT, 200 );
	mWndTabs->AddPage( mWndWatch, "Watch" );

	mWndThreads = new wxListView( mWndTabs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
	mWndThreads->AppendColumn( "ID", wxLIST_FORMAT_LEFT, 50 );
	mWndThreads->AppendColumn( "Name", wxLIST_FORMAT_LEFT, 150 );
	mWndThreads->AppendColumn( "State", wxLIST_FORMAT_LEFT, 100 );
	mWndTabs->AddPage( mWndThreads, "Threads" );

	mWndScriptList = new wxListView( mWndTabs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
	mWndScriptList->AppendColumn( "Filename", wxLIST_FORMAT_LEFT, 400 );
	mWndTabs->AddPage( mWndScriptList, "Scripts" );

	mWndBreakList = new wxListView( mWndTabs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
	mWndBreakList->AppendColumn( "Filename", wxLIST_FORMAT_LEFT, 350 );
	mWndBreakList->AppendColumn( "Line", wxLIST_FORMAT_LEFT, 50 );
	mWndTabs->AddPage( mWndBreakList, "Breakpoints" );

	gDebuggerApp.GetOptions().GetColumnWidths( "cw_callstack", mWndCallstack );
	gDebuggerApp.GetOptions().GetColumnWidths( "cw_watch",     mWndWatch );
	gDebuggerApp.GetOptions().GetColumnWidths( "cw_threads",   mWndThreads );

	gDebuggerApp.GetOptions().GetWindowPlacement( "wp_main", this );

	InitRecentFiles();
	UpdateRecentFiles();

	for( int i = 0; ; i++ )
	{
		const char* s = gDebuggerApp.GetOptions().GetString( va( "watch%d", i ) );
		if( !s || !s[0] )
		{
			break;
		}
		AddWatch( s );
	}

	Show( true );

	Printf( GAME_NAME " Script Debugger v1.1\n\n" );
	UpdateTitle();
	UpdateToolbar();

	return true;
}

/*
================
rvDebuggerWindow::CreateScriptEditor
================
*/
wxStyledTextCtrl* rvDebuggerWindow::CreateScriptEditor()
{
	wxStyledTextCtrl* editor = new wxStyledTextCtrl( mScriptTabs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE );

	editor->SetLexer( wxSTC_LEX_CPP );
	editor->StyleSetBackground( wxSTC_STYLE_DEFAULT, wxColour( 240, 240, 240 ) );
	editor->StyleSetForeground( wxSTC_STYLE_DEFAULT, wxColour( 30, 30, 30 ) );
	editor->StyleClearAll();
	editor->StyleSetForeground( wxSTC_C_WORD, wxColour( 0, 0, 255 ) );
	editor->StyleSetForeground( wxSTC_C_STRING, wxColour( 163, 21, 21 ) );
	editor->StyleSetForeground( wxSTC_C_COMMENTLINE, wxColour( 0, 128, 0 ) );
	editor->StyleSetForeground( wxSTC_C_NUMBER, wxColour( 9, 134, 88 ) );
	editor->StyleSetForeground( wxSTC_C_PREPROCESSOR, wxColour( 128, 0, 128 ) );
	editor->StyleSetForeground( wxSTC_C_COMMENT, wxColour( 0, 128, 0 ) );
	editor->StyleSetForeground( wxSTC_C_COMMENTDOC, wxColour( 0, 128, 0 ) );
	editor->SetKeyWords( 0, "scriptEvent boolean vector entity float void return if else eachFrame while object namespace string break true false for do waitUntil" );
	editor->SetMarginType( 0, wxSTC_MARGIN_NUMBER );
	editor->SetMarginWidth( 0, editor->TextWidth( wxSTC_STYLE_LINENUMBER, "_9999" ) );
	editor->SetMarginType( 1, wxSTC_MARGIN_SYMBOL );
	editor->SetMarginWidth( 1, 16 );
	editor->SetMarginMask( 1, ~wxSTC_MASK_FOLDERS | wxSTC_MASK_FOLDERS );
	editor->SetMarginSensitive( 1, true );
	editor->MarkerDefine( 0, wxSTC_MARK_ROUNDRECT, wxColour( 255, 0, 0 ), wxColour( 255, 0, 0 ) );
	editor->MarkerDefine( 1, wxSTC_MARK_ARROW,     wxColour( 0, 255, 0 ), wxColour( 0, 255, 0 ) );
	editor->SetReadOnly( true );
	editor->Bind( wxEVT_CONTEXT_MENU, &rvDebuggerWindow::OnScriptContextMenu, this );
	editor->Bind( wxEVT_KEY_DOWN, &rvDebuggerWindow::OnScriptKeyDown, this );

	return editor;
}

/*
================
rvDebuggerWindow::CreateMenuBar
================
*/
void rvDebuggerWindow::CreateMenuBar()
{
	wxMenuBar* menuBar = new wxMenuBar;

	wxMenu* fileMenu = new wxMenu;
	fileMenu->Append( ID_DBG_FILE_OPEN, "&Open...\tCtrl-O" );
	fileMenu->Append( ID_DBG_FILE_CLOSE, "&Close" );
	mRecentFileMenuWx = new wxMenu;
	fileMenu->Append( ID_DBG_FILE_MRU, "Recent Files", mRecentFileMenuWx );
	fileMenu->AppendSeparator();
	fileMenu->Append( ID_DBG_FILE_EXIT, "E&xit" );
	menuBar->Append( fileMenu, "&File" );

	wxMenu* editMenu = new wxMenu;
	editMenu->Append( ID_DBG_EDIT_CUT, "Cu&t" );
	editMenu->Append( ID_DBG_EDIT_COPY, "&Copy" );
	editMenu->Append( ID_DBG_EDIT_PASTE, "&Paste" );
	editMenu->AppendSeparator();
	editMenu->Append( ID_DBG_EDIT_FIND, "&Find...\tCtrl-F" );
	menuBar->Append( editMenu, "&Edit" );

	wxMenu* debugMenu = new wxMenu;
	debugMenu->Append( ID_DBG_DEBUG_RUN, "&Run\tF5" );
	debugMenu->Append( ID_DBG_DEBUG_BREAK, "&Break" );
	debugMenu->AppendSeparator();
	debugMenu->Append( ID_DBG_DEBUG_QUICKWATCH, "&Quick Watch...\tShift-F9" );
	debugMenu->AppendSeparator();
	debugMenu->Append( ID_DBG_DEBUG_STEPOVER, "Step &Over\tF10" );
	debugMenu->Append( ID_DBG_DEBUG_STEPINTO, "Step &Into\tF11" );
	debugMenu->Append( ID_DBG_DEBUG_STEPOUT, "Step O&ut\tShift-F11" );
	debugMenu->AppendSeparator();
	debugMenu->Append( ID_DBG_DEBUG_TOGGLEBREAKPOINT, "Toggle &Breakpoint\tF9" );
	menuBar->Append( debugMenu, "&Debug" );

	mWindowMenuWx = new wxMenu;
	mWindowMenuWx->Append( ID_DBG_WINDOW_CLOSEALL, "&Close All" );
	menuBar->Append( mWindowMenuWx, "&Window" );

	wxMenu* helpMenu = new wxMenu;
	helpMenu->Append( ID_DBG_HELP_ABOUT, "&About..." );
	menuBar->Append( helpMenu, "&Help" );

	SetMenuBar( menuBar );
}

/*
================
rvDebuggerWindow::CreateToolbar
================
*/
void rvDebuggerWindow::CreateToolbar()
{
	wxColour keyColour( 192, 192, 192 );
	bool loaded = mToolbarIcons.Load( "editors/gfx/dbg_toolbar.bmp", 16, 16, keyColour );
	const long style = wxTB_HORIZONTAL | ( loaded ? 0 : wxTB_TEXT );

	mWndToolbar = CreateToolBar( style );

	mWndToolbar->AddTool( ID_DBG_TOOLBAR_OPEN, "Open", mToolbarIcons.GetIcon( 8 ), "Open Script" );
	mWndToolbar->AddSeparator();
	mWndToolbar->AddTool( ID_DBG_TOOLBAR_RUN, "Run", mToolbarIcons.GetIcon( 0 ), "Run/Continue" );
	mWndToolbar->AddTool( ID_DBG_TOOLBAR_BREAK, "Break", mToolbarIcons.GetIcon( 1 ), "Break" );
	mWndToolbar->AddSeparator();
	mWndToolbar->AddTool( ID_DBG_TOOLBAR_STEPINTO, "Step Into", mToolbarIcons.GetIcon( 4 ), "Step Into" );
	mWndToolbar->AddTool( ID_DBG_TOOLBAR_STEPOVER, "Step Over", mToolbarIcons.GetIcon( 5 ), "Step Over" );
	mWndToolbar->AddTool( ID_DBG_TOOLBAR_STEPOUT, "Step Out", mToolbarIcons.GetIcon( 6 ), "Step Out" );
	mWndToolbar->Realize();
}

/*
================
rvDebuggerWindow::InitRecentFiles
================
*/
bool rvDebuggerWindow::InitRecentFiles()
{
	return true;
}

/*
================
rvDebuggerWindow::GetServerScriptName
================
*/
idStr rvDebuggerWindow::GetServerScriptName( const char* localFilename ) const
{
	if( !localFilename || !*localFilename )
	{
		return idStr();
	}

	idStr local( localFilename );
	local.BackSlashesToSlashes();

	idStr localBase( localFilename );
	localBase.StripPath();

	idStr result;

	const idStrList& serverScripts = mClient->GetServerScripts();
	for( int i = 0; i < serverScripts.Num(); ++i )
	{
		idStr serverBase( serverScripts[i] );
		serverBase.StripPath();
		if( !serverBase.Icmp( localBase ) )
		{
			result = serverScripts[i];
			break;
		}
	}

	if( result.IsEmpty() )
	{
		idStr basePath( cvarSystem->GetCVarString( "fs_basepath" ) );
		basePath.BackSlashesToSlashes();
		if( basePath.Length() && basePath[ basePath.Length() - 1 ] != '/' )
		{
			basePath += "/";
		}

		idStr game( cvarSystem->GetCVarString( "fs_game" ) );
		if( game.IsEmpty() )
		{
			game = BASE_GAMEDIR;
		}

		idStr prefix = basePath + game + "/";

		if( local.Length() >= prefix.Length() &&
				!idStr::Icmpn( local, prefix, prefix.Length() ) )
		{
			result = idStr( local.c_str() + prefix.Length() );
		}
		else
		{
			result = localBase;
		}
	}

	return result;
}

/*
================
rvDebuggerWindow::Activate
================
*/
bool rvDebuggerWindow::Activate()
{
	if( s_instance )
	{
		s_instance->Raise();
		return true;
	}
	return false;
}

/*
================
rvDebuggerWindow::ProcessNetMessage
================
*/
void rvDebuggerWindow::ProcessNetMessage( idBitMsg* msg )
{
	short command = msg->ReadShort();

	switch( command )
	{
		case DBMSG_REMOVEBREAKPOINT:
			wxBell();
			if( mWndScript )
			{
				mWndScript->Refresh();
			}
			UpdateBreakpointList();
			break;

		case DBMSG_RESUMED:
			UpdateTitle();
			UpdateToolbar();
			break;

		case DBMSG_INSPECTVARIABLE:
		{
			char temp[1024], temp2[1024];
			msg->ReadShort();
			msg->ReadString( temp, sizeof( temp ) );
			msg->ReadString( temp2, sizeof( temp2 ) );
			for( int i = 0; i < mWatches.Num(); i++ )
			{
				rvDebuggerWatch* watch = mWatches[i];
				if( watch->mVariable.Cmp( temp ) == 0 )
				{
					if( watch->mValue.Cmp( temp2 ) != 0 )
					{
						watch->mValue = temp2;
						watch->mModified = true;
						for( int l = 0; l < mWndWatch->GetItemCount(); l++ )
						{
							if( ( rvDebuggerWatch* )mWndWatch->GetItemData( l ) == watch )
							{
								mWndWatch->SetItem( l, 1, temp2 );
								break;
							}
						}
					}
				}
			}
			break;
		}

		case DBMSG_CONNECT:
		case DBMSG_CONNECTED:
			UpdateTitle();
			UpdateToolbar();
			Printf( "Connected...\n" );
			break;

		case DBMSG_DISCONNECT:
			UpdateTitle();
			UpdateToolbar();
			Printf( "Disconnected...\n" );
			break;

		case DBMSG_PRINT:
		{
			const char* text = ( const char* )msg->GetReadData() + msg->GetReadCount();
			m_consoleWidget->AddText( text );
			break;
		}

		case DBMSG_BREAK:
		{
			Printf( "Break:  line=%d  file='%s'\n", mClient->GetBreakLineNumber(), mClient->GetBreakFilename() );
			mCurrentStackDepth = 0;
			UpdateWatch();
			UpdateBreakpointList();
			EnableWindows( true );
			OpenScript( mClient->GetBreakFilename(), mClient->GetBreakLineNumber() - 1 );
			UpdateTitle();
			UpdateToolbar();
			Raise();
			break;
		}

		case DBMSG_INSPECTSCRIPTS:
			UpdateScriptList();
			break;

		case DBMSG_INSPECTCALLSTACK:
			UpdateCallstack();
			break;

		case DBMSG_INSPECTTHREADS:
		{
			mWndThreads->DeleteAllItems();
			for( int i = 0; i < mClient->GetThreads().Num(); i++ )
			{
				rvDebuggerThread* entry = mClient->GetThreads()[i];
				wxString state;
				if( entry->mDying )
				{
					state = "Dying";
				}
				else if( entry->mWaiting )
				{
					state = "Waiting";
				}
				else if( entry->mDoneProcessing )
				{
					state = "Stopped";
				}
				else
				{
					state = "Running";
				}

				int idx = mWndThreads->InsertItem( i, "" );
				mWndThreads->SetItem( idx, 0, wxString::Format( "%d", entry->mID ) );
				mWndThreads->SetItem( idx, 1, entry->mName.c_str() );
				mWndThreads->SetItem( idx, 2, state );
			}
			break;
		}
	}
}

/*
================
rvDebuggerWindow::Printf
================
*/
void rvDebuggerWindow::Printf( const char* fmt, ... )
{
	va_list		argptr;
	char		msg[4096];

	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

	mWndOutput->AppendText( msg );
	mWndOutput->ShowPosition( mWndOutput->GetLastPosition() );
}

/*
================
rvDebuggerWindow::AddWatch
================
*/
void rvDebuggerWindow::AddWatch( const char* name, bool update )
{
	rvDebuggerWatch* watch = new rvDebuggerWatch;
	watch->mVariable = name;
	watch->mValue = "???";
	watch->mModified = false;
	mWatches.Append( watch );

	int idx = mWndWatch->GetItemCount();
	mWndWatch->InsertItem( idx, name );
	mWndWatch->SetItem( idx, 1, "???" );
	mWndWatch->SetItemData( idx, ( long )reinterpret_cast<wxUIntPtr>( watch ) );

	if( update )
	{
		mClient->InspectVariable( name, mCurrentStackDepth );
	}
}

/*
================
rvDebuggerWindow::OnSize
================
*/
void rvDebuggerWindow::OnSize( wxSizeEvent& event )
{
	event.Skip();
}

/*
================
rvDebuggerWindow::OnClose
================
*/
void rvDebuggerWindow::OnClose( wxCloseEvent& event )
{
	if( mClient->IsConnected() )
	{
		if( wxMessageBox( "The debugger is currently connected to a running version of the game. Are you sure you want to close now?",
						  GAME_NAME " Script Debugger", wxYES_NO | wxICON_QUESTION ) == wxNO )
		{
			event.Veto();
			return;
		}
	}

	gDebuggerApp.GetOptions().SetWindowPlacement( "wp_main", this );
	gDebuggerApp.GetOptions().SetColumnWidths( "cw_callstack", mWndCallstack );
	gDebuggerApp.GetOptions().SetColumnWidths( "cw_watch", mWndWatch );
	gDebuggerApp.GetOptions().SetColumnWidths( "cw_threads", mWndThreads );

	for( int i = 0; i < mWatches.Num(); i++ )
	{
		gDebuggerApp.GetOptions().SetString( va( "watch%d", i ), mWatches[i]->mVariable );
	}

	gDebuggerApp.GetOptions().SetString( va( "watch%d", mWatches.Num() ), "" );
	gDebuggerApp.GetOptions().Save();

	event.Skip();
}

/*
================
rvDebuggerWindow::OnUpdateUI
================
*/
void rvDebuggerWindow::OnUpdateUI( wxUpdateUIEvent& event )
{
	int id = event.GetId();
	bool connected = mClient->IsConnected();
	bool stopped = mClient->IsStopped();
	bool hasScripts = mScripts.Num() > 0;

	switch( id )
	{
		case ID_DBG_DEBUG_RUN:
			event.Enable( !connected || stopped );
			break;
		case ID_DBG_DEBUG_BREAK:
			event.Enable( connected && !stopped );
			break;
		case ID_DBG_DEBUG_STEPOVER:
		case ID_DBG_DEBUG_STEPINTO:
		case ID_DBG_DEBUG_STEPOUT:
		case ID_DBG_DEBUG_QUICKWATCH:
		case ID_DBG_DEBUG_SHOWNEXTSTATEMENT:
			event.Enable( connected && stopped );
			break;
		case ID_DBG_EDIT_FIND:
		case ID_DBG_DEBUG_TOGGLEBREAKPOINT:
		case ID_DBG_FILE_CLOSE:
		case ID_DBG_WINDOW_CLOSEALL:
			event.Enable( hasScripts );
			break;
		default:
			event.Enable( true );
	}
}

/*
================
rvDebuggerWindow::OnMenuFileOpen
================
*/
void rvDebuggerWindow::OnMenuFileOpen( wxCommandEvent& event )
{
	wxFileDialog dlg( this, "Open Doom Script", wxEmptyString, wxEmptyString, "*.script", wxFD_OPEN | wxFD_FILE_MUST_EXIST );
	if( dlg.ShowModal() == wxID_OK )
	{
		if( !OpenScript( dlg.GetPath().c_str() ) )
		{
			wxMessageBox( wxString::Format( "Failed to open script '%s'", dlg.GetPath() ), GAME_NAME " Script Debugger", wxOK | wxICON_ERROR );
		}
	}
}

/*
================
rvDebuggerWindow::OnMenuFileClose
================
*/
void rvDebuggerWindow::OnMenuFileClose( wxCommandEvent& event )
{
	if( mScripts.Num() < 1 || mActiveScript < 0 || mActiveScript >= mScripts.Num() )
	{
		return;
	}

	mTabSync = true;
	mScriptTabs->DeletePage( mActiveScript );

	delete mScripts[mActiveScript];
	mScripts.RemoveIndex( mActiveScript );
	mTabSync = false;

	if( mScripts.Num() < 1 )
	{
		mActiveScript = -1;
		mWndScript = NULL;
	}
	else
	{
		if( mActiveScript >= mScripts.Num() )
		{
			mActiveScript = mScripts.Num() - 1;
		}
		mScriptTabs->SetSelection( mActiveScript );
	}

	UpdateScript();
	UpdateWindowMenu();
	UpdateToolbar();
}

/*
================
rvDebuggerWindow::OnMenuFileExit
================
*/
void rvDebuggerWindow::OnMenuFileExit( wxCommandEvent& event )
{
	Close( true );
}

/*
================
rvDebuggerWindow::OnMenuEditFind
================
*/
void rvDebuggerWindow::OnMenuEditFind( wxCommandEvent& event )
{
	rvDebuggerFindDlg dlg( this );
	if( dlg.DoModal() )
	{
		FindNext( dlg.GetFindText() );
	}
}

/*
================
rvDebuggerWindow::OnMenuDebugRun
================
*/
void rvDebuggerWindow::OnMenuDebugRun( wxCommandEvent& event )
{
	// Run the game if its not running
	if( !mClient->IsConnected() )
	{
#ifdef _MSC_VER
		char exeFile[MAX_PATH];
		char curDir[MAX_PATH];
		char cmdLine[2048];

		STARTUPINFO         startup;
		PROCESS_INFORMATION process;

		ZeroMemory( &startup, sizeof( startup ) );
		startup.cb = sizeof( startup );

		GetCurrentDirectory( MAX_PATH, curDir );
		GetModuleFileName( NULL, exeFile, MAX_PATH );

		_snprintf_s( cmdLine, sizeof( cmdLine ), _TRUNCATE,
					 "\"%s\" +set com_skipIntroVideos 1 +set com_skipLegalScreens 1"
					 " +set fs_game %s +set com_smp 0 +set com_enableDebuggerServer 1",
					 exeFile, cvarSystem->GetCVarString( "fs_game" ) );

		BOOL ok = CreateProcess( NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, curDir, &startup, &process );
		if( !ok )
		{
			DWORD err = GetLastError();
			common->Printf( "rvDebuggerWindow::OnMenuDebugRun: CreateProcess failed (err=%lu), cmd='%s'\n",
							err, cmdLine );
			return;
		}

		CloseHandle( process.hThread );
		CloseHandle( process.hProcess );
#else // MinGW

#if defined(__MINGW32__) || defined(__MINGW64__)
		{
			char exeFile[MAX_PATH];
			char curDir[MAX_PATH];

			// Get the current directory
			if( _getcwd( curDir, MAX_PATH ) == NULL )
			{
				fprintf( stderr, "getcwd failed: %d\n", errno );
				curDir[0] = '\0';
			}

#ifdef __MINGW32__
			if( __argc > 0 && __argv[0] )
			{
				strncpy( exeFile, __argv[0], MAX_PATH - 1 );
				exeFile[MAX_PATH - 1] = '\0';
			}
			else
			{
				assert( !"MinGW what the fuck??" );
			}
#else
			assert( !"MinGW what the fuck??" );
#endif

			// Build launch arguments
			const char* fs_game = cvarSystem->GetCVarString( "fs_game" );
			const char* argv_spawn[] =
			{
				exeFile,
				"+set", "com_skipIntroVideos", "1",
				"+set", "com_skipLegalScreens", "1",
				"+set", "fs_game", fs_game ? fs_game : "",
				"+set", "com_enableDebuggerServer", "1",
				NULL
			};

			int pid = _spawnv( _P_NOWAIT, exeFile, ( const char* const* )argv_spawn );
			if( pid == -1 )
			{
				perror( "_spawnv" );
			}
		}

#else // POSIX (Linux, macOS, *nix)
		{
			char exeFile[PATH_MAX];
			char curDir[PATH_MAX];
			ssize_t len = 0;
			int have_exe = 0;

			// Get the current directory
			if( getcwd( curDir, sizeof( curDir ) ) == NULL )
			{
				fprintf( stderr, "getcwd failed: %s\n", strerror( errno ) );
				curDir[0] = '\0';
			}

#if defined(__linux__)
			// probe /proc/self/exe
			len = readlink( "/proc/self/exe", exeFile, sizeof( exeFile ) - 1 );
			if( len > 0 )
			{
				exeFile[len] = '\0';
				have_exe = 1;
			}
#elif defined(__APPLE__)
			// use _NSGetExecutablePath
#include <mach-o/dyld.h>
			uint32_t size = sizeof( exeFile );
			if( _NSGetExecutablePath( exeFile, &size ) == 0 )
			{
				have_exe = 1;
			}
#endif

			if( !have_exe )
			{
				assert( !"This shouldn't happen" );
			}

			// Build launch arguments
			const char* fs_game = cvarSystem->GetCVarString( "fs_game" );
			char* argv_exec[12];
			int ai = 0;
			argv_exec[ai++] = exeFile;
			argv_exec[ai++] = ( char* )"+set";
			argv_exec[ai++] = ( char* )"com_skipIntroVideos";
			argv_exec[ai++] = ( char* )"1";
			argv_exec[ai++] = ( char* )"+set";
			argv_exec[ai++] = ( char* )"com_skipLegalScreens";
			argv_exec[ai++] = ( char* )"1";
			argv_exec[ai++] = ( char* )"+set";
			argv_exec[ai++] = ( char* )"fs_game";
			argv_exec[ai++] = ( char* )( fs_game ? fs_game : "" );
			argv_exec[ai++] = ( char* )"+set";
			argv_exec[ai++] = ( char* )"com_enableDebuggerServer";
			argv_exec[ai++] = ( char* )"1";
			argv_exec[ai] = NULL;

			pid_t pid = fork();
			if( pid < 0 )
			{
				fprintf( stderr, "fork failed: %s\n", strerror( errno ) );
			}
			else if( pid == 0 )
			{
				// Set the working directory then execute
				if( curDir[0] != '\0' )
				{
					if( chdir( curDir ) != 0 )
					{
						fprintf( stderr, "child chdir(%s) failed: %s\n", curDir, strerror( errno ) );
					}
				}

				execv( exeFile, argv_exec );

				fprintf( stderr, "execv(%s) failed: %s\n", exeFile, strerror( errno ) );
				_exit( 127 );
			}
		}

#endif // MinGW/POSIX/Linux

#endif // _MSC_VER
	}
	else if( mClient->IsStopped() )
	{
		EnableWindows( false );
		mClient->Resume();
		UpdateToolbar();
		UpdateTitle();
	}
}

/*
================
rvDebuggerWindow::OnMenuDebugBreak
================
*/
void rvDebuggerWindow::OnMenuDebugBreak( wxCommandEvent& event )
{
	mClient->Break();
}

/*
================
rvDebuggerWindow::OnMenuDebugStepOver
================
*/
void rvDebuggerWindow::OnMenuDebugStepOver( wxCommandEvent& event )
{
	EnableWindows( false );
	mClient->StepOver();
}

/*
================
rvDebuggerWindow::OnMenuDebugStepInto
================
*/
void rvDebuggerWindow::OnMenuDebugStepInto( wxCommandEvent& event )
{
	EnableWindows( false );
	mClient->StepInto();
}

/*
================
rvDebuggerWindow::OnMenuDebugStepOut
================
*/
void rvDebuggerWindow::OnMenuDebugStepOut( wxCommandEvent& event )
{
	//EnableWindows( false );
	//mClient->StepOut();
}

/*
================
rvDebuggerWindow::OnMenuDebugToggleBreakpoint
================
*/
void rvDebuggerWindow::OnMenuDebugToggleBreakpoint( wxCommandEvent& event )
{
	ToggleBreakpoint();
}

/*
================
rvDebuggerWindow::OnMenuDebugQuickWatch
================
*/
void rvDebuggerWindow::OnMenuDebugQuickWatch( wxCommandEvent& event )
{
	idStr text;
	GetSelectedText( text );
	rvDebuggerQuickWatchDlg dlg( this, this, mCurrentStackDepth, text.c_str() );
	dlg.DoModal();
}

/*
================
rvDebuggerWindow::OnMenuDebugRunToCursor
================
*/
void rvDebuggerWindow::OnMenuDebugRunToCursor( wxCommandEvent& event )
{
	if( !mWndScript || mActiveScript < 0 || mActiveScript >= mScripts.Num() )
	{
		return;
	}

	int line = mWndScript->GetCurrentLine() + 1;
	idStr serverFile = GetServerScriptName( mScripts[mActiveScript]->GetFilename() );
	if( serverFile.IsEmpty() )
	{
		return;
	}

	mClient->AddBreakpoint( serverFile, line, true );
	mClient->Resume();
}

/*
================
rvDebuggerWindow::OnMenuDebugShowNextStatement
================
*/
void rvDebuggerWindow::OnMenuDebugShowNextStatement( wxCommandEvent& event )
{
	// Only meaningful while the debugger is stopped at a breakpoint.
	if( !mClient->IsStopped() )
	{
		return;
	}

	const idList<rvDebuggerCallstack*>& stack = mClient->GetCallstack();
	if( stack.Num() < 1 )
	{
		return;
	}

	int depth = mCurrentStackDepth;
	if( depth < 0 || depth >= stack.Num() )
	{
		depth = 0;
	}

	rvDebuggerCallstack* frame = stack[depth];
	if( !frame || frame->mFilename.Length() < 1 || frame->mLineNumber <= 0 )
	{
		return;
	}

	OpenScript( frame->mFilename, frame->mLineNumber - 1 );

	if( mWndScript )
	{
		mWndScript->SetFocus();
	}
}

/*
================
rvDebuggerWindow::OnMenuWindowCloseAll
================
*/
void rvDebuggerWindow::OnMenuWindowCloseAll( wxCommandEvent& event )
{
	mTabSync = true;
	while( mScriptTabs->GetPageCount() > 0 )
	{
		mScriptTabs->DeletePage( 0 );
	}

	for( int i = 0; i < mScripts.Num(); i++ )
	{
		delete mScripts[i];
	}
	mScripts.Clear();
	mTabSync = false;

	mActiveScript = -1;
	mWndScript = NULL;

	UpdateScript();
	UpdateWindowMenu();
	UpdateToolbar();
}

/*
================
rvDebuggerWindow::OnMenuWindowSelect
================
*/
void rvDebuggerWindow::OnMenuWindowSelect( wxCommandEvent& event )
{
	int idx = event.GetId() - ID_DBG_WINDOW_MIN;
	if( idx >= 0 && idx < mScripts.Num() )
	{
		mScriptTabs->SetSelection( idx );   // fires OnScriptTabChanged
	}
}

/*
================
rvDebuggerWindow::OnMenuHelpAbout
================
*/
void rvDebuggerWindow::OnMenuHelpAbout( wxCommandEvent& event )
{
	wxMessageBox( GAME_NAME " Script Debugger v1.1\n\n"
				  "Original version by Raven\n"
				  "Dhewm3 version by Harrie van Ginneken\n"
				  "and Daniel Gibson",
				  "About " GAME_NAME " Script Debugger", wxOK | wxICON_INFORMATION );
}

/*
================
rvDebuggerWindow::OnMenuRecentFile
================
*/
void rvDebuggerWindow::OnMenuRecentFile( wxCommandEvent& event )
{
	int id = event.GetId();
	int index = id - ID_DBG_RECENT_FIRST;
	if( index >= 0 && index < gDebuggerApp.GetOptions().GetRecentFileCount() )
	{
		const char* filename = gDebuggerApp.GetOptions().GetRecentFile( index );
		if( !OpenScript( filename ) )
		{
			wxMessageBox( wxString::Format( "Failed to open script '%s'", filename ), GAME_NAME " Script Debugger", wxOK | wxICON_ERROR );
		}
	}
}

/*
================
rvDebuggerWindow::OnToolbarRun
================
*/
void rvDebuggerWindow::OnToolbarRun( wxCommandEvent& event )
{
	OnMenuDebugRun( event );
}

/*
================
rvDebuggerWindow::OnToolbarBreak
================
*/
void rvDebuggerWindow::OnToolbarBreak( wxCommandEvent& event )
{
	OnMenuDebugBreak( event );
}

/*
================
rvDebuggerWindow::OnToolbarStepOver
================
*/
void rvDebuggerWindow::OnToolbarStepOver( wxCommandEvent& event )
{
	OnMenuDebugStepOver( event );
}

/*
================
rvDebuggerWindow::OnToolbarStepInto
================
*/
void rvDebuggerWindow::OnToolbarStepInto( wxCommandEvent& event )
{
	OnMenuDebugStepInto( event );
}

/*
================
rvDebuggerWindow::OnToolbarStepOut
================
*/
void rvDebuggerWindow::OnToolbarStepOut( wxCommandEvent& event )
{
	OnMenuDebugStepOut( event );
}

/*
================
rvDebuggerWindow::OnToolbarOpen
================
*/
void rvDebuggerWindow::OnToolbarOpen( wxCommandEvent& event )
{
	OnMenuFileOpen( event );
}

/*
================
rvDebuggerWindow::OnScriptMarginClick
================
*/
void rvDebuggerWindow::OnScriptMarginClick( wxStyledTextEvent& event )
{
	if( event.GetMargin() != 1 )
	{
		return;
	}

	wxStyledTextCtrl* editor = wxDynamicCast( event.GetEventObject(), wxStyledTextCtrl );
	if( !editor )
	{
		return;
	}

	int page = mScriptTabs->FindPage( editor );
	if( page < 0 || page >= mScripts.Num() )
	{
		return;
	}

	int line = editor->LineFromPosition( event.GetPosition() ) + 1;
	idStr serverFile = GetServerScriptName( mScripts[page]->GetFilename() );
	if( serverFile.IsEmpty() )
	{
		return;
	}

	rvDebuggerBreakpoint* bp = mClient->FindBreakpoint( serverFile, line );
	if( bp )
	{
		mClient->RemoveBreakpoint( bp->GetID() );
	}
	else if( mScripts[page]->IsLineCode( line ) )
	{
		mClient->AddBreakpoint( serverFile, line );
	}
	else
	{
		wxBell();
	}

	UpdateBreakpointList();
	RefreshBreakpointMarkers();
	editor->Refresh();
}

/*
================
rvDebuggerWindow::OnScriptMouseHover
================
*/
void rvDebuggerWindow::OnScriptMouseHover( wxStyledTextEvent& event )
{
	wxStyledTextCtrl* editor = wxDynamicCast( event.GetEventObject(), wxStyledTextCtrl );
	if( !editor )
	{
		return;
	}

	int pos = event.GetPosition();
	int start = editor->WordStartPosition( pos, true );
	int end = editor->WordEndPosition( pos, true );
	if( start != end )
	{
		wxString word = editor->GetTextRange( start, end );
		mClient->InspectVariable( word.c_str(), mCurrentStackDepth );
		SetStatusText( wxString::Format( "Hover: %s", word ) );
	}
}

/*
================
rvDebuggerWindow::OnScriptKeyDown
================
*/
void rvDebuggerWindow::OnScriptKeyDown( wxKeyEvent& event )
{
	if( event.ControlDown() && event.GetKeyCode() == 'F' )
	{
		OnMenuEditFind( wxCommandEvent() );
		return;
	}
	event.Skip();
}

/*
================
rvDebuggerWindow::OnScriptContextMenu
================
*/
void rvDebuggerWindow::OnScriptContextMenu( wxContextMenuEvent& event )
{
	wxStyledTextCtrl* editor = wxDynamicCast( event.GetEventObject(), wxStyledTextCtrl );
	if( !editor )
	{
		return;
	}

	int page = mScriptTabs->FindPage( editor );
	if( page >= 0 )
	{
		mScriptTabs->SetSelection( page );
	}

	wxPoint screenPos = event.GetPosition();

	if( screenPos != wxPoint( -1, -1 ) )
	{
		wxPoint clientPos = editor->ScreenToClient( screenPos );
		int pos = editor->PositionFromPoint( clientPos );
		if( pos >= 0 )
		{
			editor->GotoPos( pos );
		}
	}

	wxMenu menu;
	menu.Append( ID_DBG_DEBUG_TOGGLEBREAKPOINT, "Toggle &Breakpoint\tF9" );
	menu.AppendSeparator();
	menu.Append( ID_DBG_DEBUG_SHOWNEXTSTATEMENT, "&Show Next Statement" );
	menu.Append( ID_DBG_DEBUG_RUNTOCURSOR, "&Run To Cursor" );

	PopupMenu( &menu, ScreenToClient( screenPos ) );
}

/*
================
rvDebuggerWindow::OnScriptTabChanged
================
*/
void rvDebuggerWindow::OnScriptTabChanged( wxBookCtrlEvent& event )
{
	if( mTabSync )
	{
		return;
	}

	int page = event.GetSelection();
	if( page < 0 || page >= mScripts.Num() )
	{
		mActiveScript = -1;
		mWndScript = NULL;
	}
	else
	{
		mActiveScript = page;
		mWndScript = ( wxStyledTextCtrl* ) mScriptTabs->GetPage( page );
	}

	UpdateTitle();
	UpdateWindowMenu();
	RefreshBreakpointMarkers();
	UpdateToolbar();
}

/*
================
rvDebuggerWindow::OnCallstackItemActivated
================
*/
void rvDebuggerWindow::OnCallstackItemActivated( wxListEvent& event )
{
	int sel = event.GetIndex();
	if( sel != -1 )
	{
		mCurrentStackDepth = sel;
		UpdateCallstack();
		UpdateWatch();
		OpenScript( mClient->GetCallstack()[mCurrentStackDepth]->mFilename,
					mClient->GetCallstack()[mCurrentStackDepth]->mLineNumber );
	}
}

/*
================
rvDebuggerWindow::OnScriptsItemActivated
================
*/
void rvDebuggerWindow::OnScriptsItemActivated( wxListEvent& event )
{
	int sel = event.GetIndex();
	if( sel != -1 )
	{
		wxString filename = mWndScriptList->GetItemText( sel, 0 );
		OpenScript( filename.c_str() );
		UpdateScriptList();
	}
}

/*
================
rvDebuggerWindow::OnBreakpointsItemActivated
================
*/
void rvDebuggerWindow::OnBreakpointsItemActivated( wxListEvent& event )
{
	int sel = event.GetIndex();
	if( sel != -1 )
	{
		rvDebuggerBreakpoint* bp = mClient->GetBreakpoint( sel );
		if( bp )
		{
			OpenScript( bp->GetFilename(), bp->GetLineNumber() - 1 );
		}
	}
}

/*
================
rvDebuggerWindow::OnBreakpointsKeyDown
================
*/
void rvDebuggerWindow::OnBreakpointsKeyDown( wxListEvent& event )
{
	if( event.GetKeyCode() == WXK_DELETE )
	{
		int sel = event.GetIndex();
		if( sel != -1 )
		{
			rvDebuggerBreakpoint* bp = mClient->GetBreakpoint( sel );
			if( bp )
			{
				mClient->RemoveBreakpoint( bp->GetID() );
				UpdateBreakpointList();
			}
		}
	}
}

/*
================
rvDebuggerWindow::OnWatchBeginLabelEdit
================
*/
void rvDebuggerWindow::OnWatchBeginLabelEdit( wxListEvent& event )
{
	if( event.GetColumn() != 0 )
	{
		event.Veto();
	}
}

/*
================
rvDebuggerWindow::OnWatchEndLabelEdit
================
*/
void rvDebuggerWindow::OnWatchEndLabelEdit( wxListEvent& event )
{
	if( event.IsEditCancelled() )
	{
		event.Veto();
		return;
	}
	wxString newName = event.GetLabel();
	if( newName.IsEmpty() )
	{
		event.Veto();
		return;
	}
	int item = event.GetIndex();
	if( item == mWndWatch->GetItemCount() - 1 )
	{
		AddWatch( newName.c_str(), true );
		UpdateWatch();
		event.Veto();
	}
	else
	{
		rvDebuggerWatch* watch = ( rvDebuggerWatch* )mWndWatch->GetItemData( item );
		if( watch )
		{
			watch->mVariable = newName.c_str();
			mClient->InspectVariable( newName.c_str(), mCurrentStackDepth );
		}
	}
}

/*
================
rvDebuggerWindow::OnWatchKeyDown
================
*/
void rvDebuggerWindow::OnWatchKeyDown( wxListEvent& event )
{
	if( event.GetKeyCode() == WXK_DELETE )
	{
		int sel = event.GetIndex();
		if( sel != -1 && sel != mWndWatch->GetItemCount() - 1 )
		{
			rvDebuggerWatch* watch = ( rvDebuggerWatch* )mWndWatch->GetItemData( sel );
			if( watch )
			{
				mWatches.Remove( watch );
				delete watch;
				mWndWatch->DeleteItem( sel );
			}
		}
	}
	if( event.GetKeyCode() == WXK_RETURN )
	{
		int sel = event.GetIndex();
		if( sel != -1 )
		{
			mWndWatch->EditLabel( sel );
		}
	}
}

/*
================
rvDebuggerWindow::OnNotebookPageChanged
================
*/
void rvDebuggerWindow::OnNotebookPageChanged( wxNotebookEvent& event )
{
	event.Skip();
}

/*
================
rvDebuggerWindow::OnConsoleCommand
================
*/
void rvDebuggerWindow::OnConsoleCommand( const char* cmd )
{
	if( !cmd || !cmd[0] )
	{
		return;
	}

	idStr echo( cmd );
	echo += "\n";
	m_consoleWidget->AddText( echo.c_str() );

	if( mClient->IsConnected() )
	{
		mClient->SendCommand( cmd );
	}
}

/*
================
rvDebuggerWindow::UpdateTitle
================
*/
void rvDebuggerWindow::UpdateTitle()
{
	idStr title = GAME_NAME " Script Debugger - ";
	if( mClient->IsConnected() )
	{
		if( mClient->IsStopped() )
		{
			title += "[break]";
		}
		else
		{
			title += "[run]";
		}
	}
	else
	{
		title += "[disconnected]";
	}

	if( mScripts.Num() )
	{
		title += " - [";
		if( mActiveScript != -1 && mActiveScript < mScripts.Num() )
		{
			title += idStr( mScripts[mActiveScript]->GetFilename() ).StripPath();
		}
		else
		{
			title += "Load Error";
		}
		title += "]";
	}
	SetTitle( title.c_str() );
}

/*
================
rvDebuggerWindow::UpdateScript
================
*/
void rvDebuggerWindow::UpdateScript()
{
	UpdateTitle();

	const bool haveScripts = ( mScripts.Num() > 0 );

	if( mScriptTabs )
	{
		mScriptTabs->Show( haveScripts );
	}

	if( mSplitter )
	{
		if( haveScripts && !mSplitter->IsSplit() )
		{
			mSplitter->SplitHorizontally( mScriptTabs, mWndTabs, -200 );
			mSplitter->SetSashGravity( 0.7 );
		}
		else if( !haveScripts && mSplitter->IsSplit() )
		{
			mSplitter->Unsplit( mScriptTabs );
		}
	}

	RefreshBreakpointMarkers();
}

/*
================
rvDebuggerWindow::UpdateWindowMenu
================
*/
void rvDebuggerWindow::UpdateWindowMenu()
{
	wxMenuBar* menuBar = GetMenuBar();
	if( !menuBar )
	{
		return;
	}

	wxMenu* windowMenu = menuBar->GetMenu( 3 );
	if( windowMenu )
	{
		while( windowMenu->GetMenuItemCount() > 1 )
		{
			wxMenuItem* item = windowMenu->FindItemByPosition( 1 );
			if( item == NULL )
			{
				break;
			}
			windowMenu->Delete( item );
		}
		if( mScripts.Num() )
		{
			windowMenu->AppendSeparator();
			for( int i = 0; i < mScripts.Num(); i++ )
			{
				idStr label = idStr( va( "&%d ", i + 1 ) ) + idStr( mScripts[i]->GetFilename() ).StripPath();
				windowMenu->Append( ID_DBG_WINDOW_MIN + i, label.c_str(), "", wxITEM_CHECK );
				if( i == mActiveScript )
				{
					windowMenu->Check( ID_DBG_WINDOW_MIN + i, true );
				}
			}
		}
	}
}

/*
================
rvDebuggerWindow::UpdateCallstack
================
*/
void rvDebuggerWindow::UpdateCallstack()
{
	mWndCallstack->DeleteAllItems();
	const idList<rvDebuggerCallstack*>& stack = mClient->GetCallstack();
	for( int i = 0; i < stack.Num(); i++ )
	{
		rvDebuggerCallstack* entry = stack[i];
		int idx = mWndCallstack->InsertItem( i, "" );
		mWndCallstack->SetItem( idx, 0, entry->mFunction.c_str() );
		mWndCallstack->SetItem( idx, 1, wxString::Format( "%d", entry->mLineNumber ) );
		mWndCallstack->SetItem( idx, 2, entry->mFilename.c_str() );
	}
}

/*
================
rvDebuggerWindow::RefreshBreakpointMarkers
================
*/
void rvDebuggerWindow::RefreshBreakpointMarkers()
{
	if( mWndScript == NULL )
	{
		return;
	}

	mWndScript->MarkerDeleteAll( 0 );
	mWndScript->MarkerDeleteAll( 1 );

	if( mScripts.Num() < 1 || mActiveScript < 0 || mActiveScript >= mScripts.Num() )
	{
		return;
	}

	idStr serverFile = GetServerScriptName( mScripts[mActiveScript]->GetFilename() );
	if( serverFile.IsEmpty() )
	{
		return;
	}

	int count = mClient->GetBreakpointCount();
	for( int i = 0; i < count; i++ )
	{
		rvDebuggerBreakpoint* bp = mClient->GetBreakpoint( i );
		if( bp && idStr::Icmp( bp->GetFilename(), serverFile ) == 0 )
		{
			mWndScript->MarkerAdd( bp->GetLineNumber() - 1, 0 );
		}
	}

	if( mClient->IsStopped() )
	{
		const char* breakName = mClient->GetBreakFilename();
		if( breakName && *breakName )
		{
			idStr breakFile = GetServerScriptName( breakName );
			if( !breakFile.IsEmpty() && idStr::Icmp( breakFile, serverFile ) == 0 )
			{
				mWndScript->MarkerAdd( mClient->GetBreakLineNumber() - 1, 1 );
			}
		}
	}
}

/*
================
rvDebuggerWindow::UpdateBreakpointList
================
*/
void rvDebuggerWindow::UpdateBreakpointList()
{
	mWndBreakList->DeleteAllItems();
	int count = mClient->GetBreakpointCount();
	for( int i = 0; i < count; i++ )
	{
		rvDebuggerBreakpoint* bp = mClient->GetBreakpoint( i );
		if( bp )
		{
			int idx = mWndBreakList->InsertItem( i, "" );
			mWndBreakList->SetItem( idx, 0, bp->GetFilename() );
			mWndBreakList->SetItem( idx, 1, wxString::Format( "%d", bp->GetLineNumber() ) );
		}
	}
}

/*
================
rvDebuggerWindow::UpdateScriptList
================
*/
void rvDebuggerWindow::UpdateScriptList()
{
	mWndScriptList->DeleteAllItems();
	idStrList& scripts = mClient->GetServerScripts();
	for( int i = 0; i < scripts.Num(); i++ )
	{
		int idx = mWndScriptList->InsertItem( i, "" );
		mWndScriptList->SetItem( idx, 0, scripts[i].c_str() );
	}
}

/*
================
rvDebuggerWindow::UpdateWatch
================
*/
void rvDebuggerWindow::UpdateWatch()
{
	for( int i = 0; i < mWatches.Num(); i++ )
	{
		mWatches[i]->mModified = false;
		mClient->InspectVariable( mWatches[i]->mVariable, mCurrentStackDepth );
	}
}

/*
================
rvDebuggerWindow::UpdateToolbar
================
*/
void rvDebuggerWindow::UpdateToolbar()
{
	bool connected = mClient->IsConnected();
	bool stopped = mClient->IsStopped();
	mWndToolbar->EnableTool( ID_DBG_TOOLBAR_RUN, !connected || stopped );
	mWndToolbar->EnableTool( ID_DBG_TOOLBAR_BREAK, connected && !stopped );
	mWndToolbar->EnableTool( ID_DBG_TOOLBAR_STEPOVER, connected && stopped );
	mWndToolbar->EnableTool( ID_DBG_TOOLBAR_STEPINTO, connected && stopped );
	mWndToolbar->EnableTool( ID_DBG_TOOLBAR_STEPOUT, connected && stopped );
	mWndToolbar->Refresh();
}

/*
================
rvDebuggerWindow::UpdateRecentFiles
================
*/
void rvDebuggerWindow::UpdateRecentFiles()
{
	if( !mRecentFileMenuWx )
	{
		return;
	}

	while( mRecentFileMenuWx->GetMenuItemCount() > 0 )
	{
		wxMenuItem* item = mRecentFileMenuWx->FindItemByPosition( 0 );
		if( item == NULL )
		{
			break;
		}
		mRecentFileMenuWx->Delete( item );
	}

	int count = gDebuggerApp.GetOptions().GetRecentFileCount();
	for( int i = 0; i < count; i++ )
	{
		wxString label = wxString::Format( "&%d %s", i + 1, gDebuggerApp.GetOptions().GetRecentFile( i ) );
		mRecentFileMenuWx->Append( ID_DBG_RECENT_FIRST + i, label );
	}
}

/*
================
rvDebuggerWindow::OpenScript
================
*/
bool rvDebuggerWindow::OpenScript( const char* filename, int lineNumber, idProgram* program )
{
	SetCursor( wxCURSOR_WAIT );

	idStr incomingBase( filename );
	incomingBase.StripPath();

	// Already open? Just bring the tab forward.
	for( int i = 0; i < mScripts.Num(); i++ )
	{
		idStr openBase( mScripts[i]->GetFilename() );
		openBase.StripPath();

		if( !incomingBase.Icmp( openBase ) )
		{
			mScriptTabs->SetSelection( i );
			if( lineNumber != -1 )
			{
				wxStyledTextCtrl* ed = ( wxStyledTextCtrl* ) mScriptTabs->GetPage( i );
				ed->GotoLine( lineNumber );
				ed->SetFocus();
			}
			SetCursor( wxCURSOR_ARROW );
			return true;
		}
	}

	// Not already open, so load the script file.
	rvDebuggerScript* script = new rvDebuggerScript;
	if( !script->Load( filename ) )
	{
		delete script;
		SetCursor( wxCURSOR_ARROW );
		return false;
	}

	// Give this script its own tab.
	wxStyledTextCtrl* editor = CreateScriptEditor();
	editor->SetReadOnly( false );
	editor->AddText( script->GetContents() );
	editor->SetReadOnly( true );
	editor->EmptyUndoBuffer();

	idStr label = script->GetFilename();
	label.StripPath();

	mTabSync = true;
	mScriptTabs->AddPage( editor, label.c_str(), true );
	mScripts.Append( script );
	mTabSync = false;

	mActiveScript = mScripts.Num() - 1;
	mWndScript = editor;

	gDebuggerApp.GetOptions().AddRecentFile( filename );
	gDebuggerApp.GetOptions().Save();
	UpdateRecentFiles();

	UpdateScript();
	UpdateWindowMenu();
	UpdateToolbar();

	if( lineNumber != -1 )
	{
		editor->GotoLine( lineNumber );
		editor->SetFocus();
	}

	SetCursor( wxCURSOR_ARROW );
	return true;
}

/*
================
rvDebuggerWindow::ToggleBreakpoint
================
*/
void rvDebuggerWindow::ToggleBreakpoint()
{
	if( mScripts.Num() < 1 || mActiveScript == -1 || !mWndScript )
	{
		return;
	}

	int line = mWndScript->GetCurrentLine() + 1;
	idStr serverFile = GetServerScriptName( mScripts[mActiveScript]->GetFilename() );
	if( serverFile.IsEmpty() )
	{
		return;
	}

	rvDebuggerBreakpoint* bp = mClient->FindBreakpoint( serverFile, line );
	if( bp )
	{
		mClient->RemoveBreakpoint( bp->GetID() );
	}
	else if( mScripts[mActiveScript]->IsLineCode( line ) )
	{
		mClient->AddBreakpoint( serverFile, line );
	}
	else
	{
		wxBell();
	}

	UpdateBreakpointList();
	RefreshBreakpointMarkers();
	mWndScript->Refresh();
}

/*
================
rvDebuggerWindow::EnableWindows
================
*/
void rvDebuggerWindow::EnableWindows( bool state )
{
	mWndCallstack->Enable( state );
	mWndWatch->Enable( state );
	mWndThreads->Enable( state );
}

/*
================
rvDebuggerWindow::GetSelectedText
================
*/
int rvDebuggerWindow::GetSelectedText( idStr& text )
{
	text.Empty();
	if( mScripts.Num() < 1 || mActiveScript == -1 || !mWndScript )
	{
		return -1;
	}
	int start = mWndScript->GetSelectionStart();
	int end = mWndScript->GetSelectionEnd();
	if( start == end )
	{
		int pos = mWndScript->GetCurrentPos();
		start = mWndScript->WordStartPosition( pos, true );
		end = mWndScript->WordEndPosition( pos, true );
	}
	if( start != end )
	{
		text = mWndScript->GetTextRange( start, end ).c_str();
		return start;
	}
	return -1;
}

/*
================
rvDebuggerWindow::FindNext
================
*/
bool rvDebuggerWindow::FindNext( const char* text )
{
	if( !mWndScript )
	{
		return false;
	}

	if( text )
	{
		mFind = text;
	}
	if( mFind.IsEmpty() )
	{
		return false;
	}

	int flags = wxSTC_FIND_NONE;
	int pos = mWndScript->GetCurrentPos();
	int start = mWndScript->FindText( pos, mWndScript->GetLength(), mFind.c_str(), flags );
	if( start == -1 )
	{
		start = mWndScript->FindText( 0, pos, mFind.c_str(), flags );
	}
	if( start != -1 )
	{
		mWndScript->SetSelection( start, start + mFind.Length() );
		mWndScript->EnsureCaretVisible();
		return true;
	}
	return false;
}

/*
================
rvDebuggerWindow::FindPrev
================
*/
bool rvDebuggerWindow::FindPrev( const char* text )
{
	if( !mWndScript )
	{
		return false;
	}

	if( text )
	{
		mFind = text;
	}
	if( mFind.IsEmpty() )
	{
		return false;
	}

	int flags = wxSTC_FIND_NONE;
	int pos = mWndScript->GetCurrentPos();
	int start = mWndScript->FindText( 0, pos, mFind.c_str(), flags );
	if( start == -1 )
	{
		start = mWndScript->FindText( pos, mWndScript->GetLength(), mFind.c_str(), flags );
	}
	if( start != -1 )
	{
		mWndScript->SetSelection( start, start + mFind.Length() );
		mWndScript->EnsureCaretVisible();
		return true;
	}
	return false;
}