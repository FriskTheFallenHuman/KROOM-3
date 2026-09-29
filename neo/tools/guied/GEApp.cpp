/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software

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

#include "GEApp.h"

static const char* const PANE_WORKSPACES = "workspaces";
static const char* const PANE_NAVIGATOR = "navigator";
static const char* const PANE_PROPERTIES = "properties";

static const char* const STRIP_LEFT = "leftStrip";
static const char* const STRIP_RIGHT = "rightStrip";
static const char* const STRIP_BOTTOM = "bottomStrip";

wxBEGIN_EVENT_TABLE( rvGEApp, wxFrame )
	EVT_MENU( ID_GUIED_FILE_NEW, rvGEApp::OnFileNew )
	EVT_MENU( ID_GUIED_FILE_OPEN, rvGEApp::OnFileOpen )
	EVT_MENU( ID_GUIED_FILE_SAVE, rvGEApp::OnFileSave )
	EVT_MENU( ID_GUIED_FILE_SAVEAS, rvGEApp::OnFileSaveAs )
	EVT_MENU( ID_GUIED_FILE_CLOSE, rvGEApp::OnFileClose )
	EVT_MENU( ID_GUIED_FILE_EXIT, rvGEApp::OnFileExit )
	EVT_MENU_RANGE( 10000, 10000 + 32, rvGEApp::OnFileRecent )
	EVT_MENU( ID_GUIED_EDIT_UNDO, rvGEApp::OnEditUndo )
	EVT_MENU( ID_GUIED_EDIT_REDO, rvGEApp::OnEditRedo )
	EVT_MENU( ID_GUIED_EDIT_CUT, rvGEApp::OnEditCut )
	EVT_MENU( ID_GUIED_EDIT_COPY, rvGEApp::OnEditCopy )
	EVT_MENU( ID_GUIED_EDIT_PASTE, rvGEApp::OnEditPaste )
	EVT_MENU( ID_GUIED_EDIT_DELETE, rvGEApp::OnEditDelete )
	EVT_MENU( ID_GUIED_HELP_ABOUT, rvGEApp::OnHelpAbout )
	EVT_MENU( ID_GUIED_VIEW_OPTIONS, rvGEApp::OnViewOptions )
	EVT_MENU( ID_GUIED_VIEW_SHOWGRID, rvGEApp::OnViewToggleGrid )
	EVT_MENU( ID_GUIED_VIEW_SNAPTOGRID, rvGEApp::OnViewToggleSnap )
	EVT_MENU( ID_GUIED_VIEW_ZOOMFIT, rvGEApp::OnViewZoomFit )
	EVT_MENU( ID_GUIED_VIEW_STATUSBAR, rvGEApp::OnViewStatusBar )
	EVT_MENU( ID_GUIED_TOOLS_VIEWER, rvGEApp::OnToolsViewer )
	EVT_MENU( ID_GUIED_TOOLS_RELOADMATERIALS, rvGEApp::OnToolsReloadMaterials )

	EVT_UPDATE_UI( ID_GUIED_FILE_SAVE, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_FILE_SAVEAS, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_FILE_CLOSE, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_VIEW_ZOOMIN, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_VIEW_ZOOMOUT, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_VIEW_ZOOMFIT, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_VIEW_STATUSBAR, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_VIEW_SHOWGRID, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_VIEW_SNAPTOGRID, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_ITEM_PROPERTIES, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_ITEM_SCRIPTS, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_EDIT_UNDO, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_EDIT_REDO, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_EDIT_DELETE, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_EDIT_CUT, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_EDIT_COPY, rvGEApp::OnUpdateUI )
	EVT_UPDATE_UI( ID_GUIED_EDIT_PASTE, rvGEApp::OnUpdateUI )

	EVT_CLOSE( rvGEApp::OnClose )
wxEND_EVENT_TABLE()

/*
================
rvGEApp::rvGEApp
================
*/
rvGEApp::rvGEApp()
	: wxFrame( NULL, wxID_ANY, "GUI Editor", wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE )
	, m_navigator( this )
	, m_properties( this )
	, m_transformer( &m_properties )
	, m_notebook( NULL )
	, m_viewer( NULL )
	, m_closing( false )
	, m_leftStrip( NULL )
	, m_rightStrip( NULL )
	, m_bottomStrip( NULL )
	, m_recentMenu( NULL )
{
	const wxIcon icon = ImageUtils::IconFromRelativePath( "editors/icons/guied.ico" );
	if( icon.IsOk() )
	{
		SetIcon( icon );
	}
}

/*
================
rvGEApp::~rvGEApp

Clears the global pointer, uninitialises AUI, and destroys the viewer.
================
*/
rvGEApp::~rvGEApp()
{
	if( gApp == this )
	{
		gApp = NULL;
	}

	m_aui.UnInit();
	delete m_viewer;
	m_viewer = NULL;
}

/*
================
rvGEApp::Initialize
================
*/
bool rvGEApp::Initialize()
{
	m_options.Init();
	m_options.Load();

	BuildMenuBar();
	BuildAuiLayout();

	// Restore saved AUI perspective after all panes are registered.
	wxString perspective = wxString::FromUTF8( m_options.GetString( "AuiPerspective", "" ) );
	if( !perspective.IsEmpty() )
	{
		m_aui.LoadPerspective( perspective, false );
		m_aui.Update();
	}

	RebuildAutoHideStrips();

	if( !m_options.GetWindowPlacement( "mdiframe", this ) )
	{
		SetSize( 1024, 768 );
		CentreOnScreen();
	}

	Show();
	Update();

	return true;
}

/*
================
rvGEApp::Shutdown
================
*/
void rvGEApp::Shutdown()
{
	m_options.SetWindowPlacement( "mdiframe", this );
	m_options.SetString( "AuiPerspective", m_aui.SavePerspective().utf8_str() );

	if( m_aui.GetPane( PANE_NAVIGATOR ).IsOk() )
	{
		m_options.SetNavigatorVisible( m_aui.GetPane( PANE_NAVIGATOR ).IsShown() );
	}

	if( m_aui.GetPane( PANE_PROPERTIES ).IsOk() )
	{
		m_options.SetPropertiesVisible( m_aui.GetPane( PANE_PROPERTIES ).IsShown() );
	}

	m_options.Save();

	if( m_viewer )
	{
		m_viewer->Destroy();
		m_viewer = NULL;
	}
}

/*
================
rvGEApp::MessageBox
================
*/
int rvGEApp::MessageBox( const wxString& text, long style )
{
	return wxMessageBox( text, "GUI Editor", style, this );
}

/*
================
rvGEApp::RunFrame

Pumps one frame into the active workspace and (if present) the viewer.
================
*/
void rvGEApp::RunFrame()
{
	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		ws->RunFrame();
	}

	if( m_viewer )
	{
		m_viewer->RunFrame();
	}
}

/*
================
rvGEApp::GetActiveWorkspace
================
*/
rvGEWorkspace* rvGEApp::GetActiveWorkspace()
{
	if( !m_notebook )
	{
		return NULL;
	}

	const int sel = m_notebook->GetSelection();
	if( sel == wxNOT_FOUND )
	{
		return NULL;
	}

	return dynamic_cast< rvGEWorkspace* >( m_notebook->GetPage( sel ) );
}

/*
================
rvGEApp::ActivateWorkspace

Points navigator/properties/transformer at the newly active workspace.
================
*/
void rvGEApp::ActivateWorkspace( rvGEWorkspace* ws )
{
	if( !ws )
	{
		return;
	}

	m_navigator.SetWorkspace( ws );
	m_properties.SetWorkspace( ws );
	m_transformer.SetWorkspace( ws );
}

/*
================
rvGEApp::NewWorkspace
================
*/
rvGEWorkspace* rvGEApp::NewWorkspace()
{
	rvGEWorkspace* ws = new rvGEWorkspace( this, m_notebook );
	if( !ws->NewFile() )
	{
		delete ws;
		return NULL;
	}

	m_notebook->AddPage( ws, "untitled.gui", true );

	ws->CallAfter( [ws]()
	{
		ws->ZoomFit();
	} );

	ActivateWorkspace( ws );

	return ws;
}

/*
================
rvGEApp::OpenFile

Opens the file in a new tab, or focuses an existing tab for the same path.
================
*/
bool rvGEApp::OpenFile( const char* filename )
{
	for( size_t i = 0; i < m_notebook->GetPageCount(); ++i )
	{
		rvGEWorkspace* w = dynamic_cast< rvGEWorkspace* >( m_notebook->GetPage( i ) );
		if( w && !wxStricmp( w->GetFilename(), filename ) )
		{
			m_notebook->SetSelection( i );
			ActivateWorkspace( w );
			return false;
		}
	}

	wxBusyCursor wait;

	rvGEWorkspace* ws = new rvGEWorkspace( this, m_notebook );
	wxString error = wxString::Format( "Failed to parse '%s'", filename );

	if( !ws->LoadFile( filename, &error ) )
	{
		wxMessageBox( error, "GUI Editor", wxOK | wxICON_ERROR, this );
		delete ws;
		return false;
	}

	m_notebook->AddPage( ws, ws->GetFilename(), true );

	ws->CallAfter( [ws]()
	{
		ws->ZoomFit();
	} );

	ActivateWorkspace( ws );

	m_options.AddRecentFile( filename );
	m_options.Save();
	UpdateRecentFiles();

	return true;
}

/*
================
rvGEApp::BuildMenuBar
================
*/
void rvGEApp::BuildMenuBar()
{
	wxMenuBar* bar = new wxMenuBar();

	wxMenu* file = new wxMenu;
	file->Append( ID_GUIED_FILE_NEW, "&New\tCtrl+N" );
	file->Append( ID_GUIED_FILE_OPEN, "&Open...\tCtrl+O" );
	file->Append( ID_GUIED_FILE_CLOSE, "&Close" );
	file->AppendSeparator();
	file->Append( ID_GUIED_FILE_SAVE, "&Save\tCtrl+S" );
	file->Append( ID_GUIED_FILE_SAVEAS, "Save &As..." );
	file->AppendSeparator();

	m_recentMenu = new wxMenu;
	file->AppendSubMenu( m_recentMenu, "Recent &Files" );

	file->AppendSeparator();
	file->Append( ID_GUIED_FILE_EXIT, "E&xit" );
	bar->Append( file, "&File" );

	wxMenu* edit = new wxMenu;
	edit->Append( ID_GUIED_EDIT_UNDO, "&Undo\tCtrl+Z" );
	edit->Append( ID_GUIED_EDIT_REDO, "&Redo\tCtrl+Y" );
	edit->AppendSeparator();
	edit->Append( ID_GUIED_EDIT_CUT, "Cu&t\tCtrl+X" );
	edit->Append( ID_GUIED_EDIT_COPY, "&Copy\tCtrl+C" );
	edit->Append( ID_GUIED_EDIT_PASTE, "&Paste\tCtrl+V" );
	edit->Append( ID_GUIED_EDIT_DELETE, "&Delete\tDel" );
	bar->Append( edit, "&Edit" );

	wxMenu* view = new wxMenu;
	view->Append( ID_GUIED_VIEW_ZOOMIN, "Zoom &In\tCtrl++" );
	view->Append( ID_GUIED_VIEW_ZOOMOUT, "Zoom &Out\tCtrl+-" );
	view->Append( ID_GUIED_VIEW_ZOOMFIT, "&Fit to Viewport\tCtrl+0" );
	view->AppendSeparator();

	view->AppendCheckItem( ID_GUIED_VIEW_SHOWGRID, "Show &Grid\tCtrl+`" );
	view->AppendCheckItem( ID_GUIED_VIEW_SNAPTOGRID, "S&nap To Grid\tShift+Ctrl+`" );
	view->AppendSeparator();
	view->AppendCheckItem( ID_GUIED_VIEW_STATUSBAR, "Show &Status Bar" );
	view->AppendSeparator();
	view->Append( ID_GUIED_VIEW_OPTIONS, "O&ptions..." );
	bar->Append( view, "&View" );

	wxMenu* tools = new wxMenu;
	tools->Append( ID_GUIED_TOOLS_VIEWER, "&Viewer\tCtrl+t" );
	tools->Append( ID_GUIED_TOOLS_RELOADMATERIALS, "Reload &Materials\tCtrl+m" );
	bar->Append( tools, "&Tools" );

	wxMenu* help = new wxMenu;
	help->Append( ID_GUIED_HELP_ABOUT, "&About" );
	bar->Append( help, "&Help" );

	SetMenuBar( bar );

	CreateStatusBar( 3 );
	const int widths[] = { -1, 120, 120 };
	SetStatusWidths( 3, widths );
	SetStatusText( "Ready", 0 );
	SetStatusText( "", 1 );
	SetStatusText( "", 2 );
}

/*
================
rvGEApp::UpdateRecentFiles
================
*/
void rvGEApp::UpdateRecentFiles()
{
	if( !m_recentMenu )
	{
		return;
	}

	while( m_recentMenu->GetMenuItemCount() )
	{
		m_recentMenu->Delete( m_recentMenu->FindItemByPosition( 0 ) );
	}

	const int count = m_options.GetRecentFileCount();
	for( int i = 0; i < count; ++i )
	{
		m_recentMenu->Append( 10000 + i, wxString::Format( "&%d %s", i + 1, m_options.GetRecentFile( count - 1 - i ) ) );
	}
}

/*
================
rvGEApp::BuildAuiLayout
================
*/
void rvGEApp::BuildAuiLayout()
{
	m_aui.SetManagedWindow( this );

	m_notebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP | wxAUI_NB_TAB_MOVE | wxAUI_NB_CLOSE_ON_ALL_TABS );

	m_notebook->Bind( wxEVT_AUINOTEBOOK_PAGE_CHANGED, &rvGEApp::OnNotebookPageChanged, this );
	m_notebook->Bind( wxEVT_AUINOTEBOOK_PAGE_CLOSE, &rvGEApp::OnNotebookPageClose, this );
	m_notebook->Bind( wxEVT_AUINOTEBOOK_PAGE_CLOSED, &rvGEApp::OnNotebookPageClosed, this );
	m_notebook->Bind( wxEVT_AUINOTEBOOK_BUTTON, &rvGEApp::OnNotebookButton, this );

	wxAuiPaneInfo centre;
	centre.Name( PANE_WORKSPACES ).CenterPane().Caption( "Workspace" ).CloseButton( false ).MaximizeButton( false );
	m_aui.AddPane( m_notebook, centre );

	wxAuiPaneInfo nav;
	nav.Name( PANE_NAVIGATOR ).Left().Caption( "Navigator" ).BestSize( 240, -1 ).MinSize( 140, -1 )
	.MaximizeButton( true ).Floatable( true ).PinButton( true ).CloseButton( false );
	m_aui.AddPane( &m_navigator, nav );

	wxAuiPaneInfo props;
	props.Name( PANE_PROPERTIES ).Right().Caption( "Properties" ).BestSize( 260, 300 ).MinSize( 180, 100 )
	.MaximizeButton( true ).Floatable( true ).PinButton( true ).CloseButton( false ).Row( 0 );
	m_aui.AddPane( &m_properties, props );

	m_properties.EmbedTransformer( &m_transformer );

	BuildAutoHideStrips();

	m_aui.Update();

	m_aui.GetPane( PANE_NAVIGATOR ).Show( m_options.GetNavigatorVisible() );
	m_aui.GetPane( PANE_PROPERTIES ).Show( m_options.GetPropertiesVisible() );

	m_aui.Update();

	Bind( wxEVT_AUI_PANE_CLOSE, &rvGEApp::OnAuiPaneClose, this );
	Bind( wxEVT_AUI_PANE_BUTTON, &rvGEApp::OnAuiPaneButton, this );
}

/*
================
rvGEApp::BuildAutoHideStrips
================
*/
void rvGEApp::BuildAutoHideStrips()
{
	const int thickness = AutoHideStrip::GetThickness();

	m_leftStrip = new AutoHideStrip( this, AutoHideStrip::Left );
	m_rightStrip = new AutoHideStrip( this, AutoHideStrip::Right );
	m_bottomStrip = new AutoHideStrip( this, AutoHideStrip::Bottom );

	auto onTab = [this]( wxWindow * w )
	{
		OnAutoHideTabClick( w );
	};
	m_leftStrip->SetTabClickHandler( onTab );
	m_rightStrip->SetTabClickHandler( onTab );
	m_bottomStrip->SetTabClickHandler( onTab );

	auto stripInfo = [&]( const wxString & name ) -> wxAuiPaneInfo
	{
		return wxAuiPaneInfo().Name( name ).CaptionVisible( false ).CloseButton( false ).PinButton( false ).MaximizeButton( false )
		.MinimizeButton( false ).Gripper( false ).PaneBorder( false ).Floatable( false ).Movable( false ).Resizable( false ).Layer( 10 ).Hide();
	};

	m_aui.AddPane( m_leftStrip, stripInfo( STRIP_LEFT ).Left().BestSize( thickness, -1 ).MinSize( thickness, -1 ).MaxSize( thickness, -1 ) );
	m_aui.AddPane( m_rightStrip, stripInfo( STRIP_RIGHT ).Right().BestSize( thickness, -1 ).MinSize( thickness, -1 ).MaxSize( thickness, -1 ) );
	m_aui.AddPane( m_bottomStrip, stripInfo( STRIP_BOTTOM ).Bottom() .BestSize( -1, thickness ).MinSize( -1, thickness ).MaxSize( -1, thickness ) );
}

/*
================
rvGEApp::GetStripForPane
================
*/
AutoHideStrip* rvGEApp::GetStripForPane( wxAuiPaneInfo& pane ) const
{
	if( pane.dock_direction == wxAUI_DOCK_LEFT )
	{
		return m_leftStrip;
	}
	if( pane.dock_direction == wxAUI_DOCK_RIGHT )
	{
		return m_rightStrip;
	}
	if( pane.dock_direction == wxAUI_DOCK_BOTTOM )
	{
		return m_bottomStrip;
	}
	return NULL;
}

/*
================
rvGEApp::RebuildAutoHideStrips
================
*/
void rvGEApp::RebuildAutoHideStrips()
{
	if( !m_leftStrip || !m_rightStrip || !m_bottomStrip )
	{
		return;
	}

	m_leftStrip->Clear();
	m_rightStrip->Clear();
	m_bottomStrip->Clear();

	const wxAuiPaneInfoArray& panes = m_aui.GetAllPanes();

	for( size_t i = 0; i < panes.GetCount(); ++i )
	{
		wxAuiPaneInfo& p = panes[i];

		if( p.window == m_leftStrip || p.window == m_rightStrip || p.window == m_bottomStrip )
		{
			continue;
		}

		if( p.caption.IsEmpty() )
		{
			continue;
		}

		if( p.IsShown() )
		{
			continue;
		}

		AutoHideStrip* strip = GetStripForPane( p );
		if( strip )
		{
			strip->AddItem( p.window, p.caption );
		}
	}

	m_leftStrip->Rebuild();
	m_rightStrip->Rebuild();
	m_bottomStrip->Rebuild();

	m_aui.GetPane( m_leftStrip ).Show( m_leftStrip->GetItemCount() > 0 );
	m_aui.GetPane( m_rightStrip ).Show( m_rightStrip->GetItemCount() > 0 );
	m_aui.GetPane( m_bottomStrip ).Show( m_bottomStrip->GetItemCount() > 0 );

	m_aui.Update();
}

/*
================
rvGEApp::OnAutoHideTabClick

Re-shows a hidden pane and re-fits the layout.
================
*/
void rvGEApp::OnAutoHideTabClick( wxWindow* paneWindow )
{
	if( !paneWindow )
	{
		return;
	}

	wxAuiPaneInfoArray& panes = m_aui.GetAllPanes();
	for( size_t i = 0; i < panes.GetCount(); ++i )
	{
		if( panes[i].IsMaximized() )
		{
			m_aui.RestorePane( panes[i] );
			break;
		}
	}

	m_aui.GetPane( paneWindow ).Show( true );
	m_aui.Update();

	RebuildAutoHideStrips();

	CallAfter( [this]()
	{
		m_aui.Update();
		SendSizeEvent();
		Layout();
		Refresh();
	} );
}

/*
================
rvGEApp::OnAuiPaneClose
================
*/
void rvGEApp::OnAuiPaneClose( wxAuiManagerEvent& e )
{
	m_aui.Update();
	RebuildAutoHideStrips();
	e.Skip();
}

/*
================
rvGEApp::OnAuiPaneButton

Interprets the pin button as "auto hide" instead of "float".
================
*/
void rvGEApp::OnAuiPaneButton( wxAuiManagerEvent& e )
{
	if( e.GetButton() != wxAUI_BUTTON_PIN )
	{
		e.Skip();
		return;
	}

	wxAuiPaneInfo* pane = e.GetPane();
	if( !pane || !pane->window )
	{
		e.Skip();
		return;
	}

	const int dir = pane->dock_direction;

	m_aui.GetPane( pane->window ).Show( false );
	m_aui.GetPane( pane->window ).dock_direction = dir;
	m_aui.Update();

	RebuildAutoHideStrips();

	e.Veto();
}

/*
================
rvGEApp::OnNotebookButton
================
*/
void rvGEApp::OnNotebookButton( wxAuiNotebookEvent& e )
{
	if( e.GetInt() != wxAUI_BUTTON_CLOSE )
	{
		e.Skip();
		return;
	}

	e.Veto();

	const int sel = e.GetSelection();
	if( sel < 0 || sel >= ( int )m_notebook->GetPageCount() )
	{
		return;
	}

	rvGEWorkspace* ws = dynamic_cast< rvGEWorkspace* >( m_notebook->GetPage( sel ) );
	if( !ws )
	{
		return;
	}

	if( ws->IsModified() )
	{
		const int r = wxMessageBox( wxString::Format( "Save changes to \"%s\"?", ws->GetFilename() ), "GUI Editor", wxYES_NO | wxCANCEL | wxICON_QUESTION, this );
		if( r == wxCANCEL )
		{
			return;
		}

		if( r == wxYES )
		{
			ws->SaveFile( ws->GetFilename() );
		}
	}

	if( m_notebook->GetPageCount() > 1 )
	{
		m_notebook->DeletePage( sel );
	}
	else
	{
		CallAfter( [this]()
		{
			Close( true );
		} );
	}
}

/*
================
rvGEApp::OnNotebookPageChanged
================
*/
void rvGEApp::OnNotebookPageChanged( wxAuiNotebookEvent& e )
{
	const int sel = e.GetSelection();
	rvGEWorkspace* ws = NULL;

	if( sel != wxNOT_FOUND && sel < ( int )m_notebook->GetPageCount() )
	{
		ws = dynamic_cast< rvGEWorkspace* >( m_notebook->GetPage( sel ) );
	}

	ActivateWorkspace( ws );

	e.Skip();
}

/*
================
rvGEApp::OnNotebookPageClose
================
*/
void rvGEApp::OnNotebookPageClose( wxAuiNotebookEvent& e )
{
	const int sel = e.GetSelection();
	if( sel < 0 || sel >= ( int )m_notebook->GetPageCount() )
	{
		e.Skip();
		return;
	}

	rvGEWorkspace* ws = dynamic_cast< rvGEWorkspace* >( m_notebook->GetPage( sel ) );
	if( !ws )
	{
		e.Skip();
		return;
	}

	if( ws->IsModified() )
	{
		const int r = wxMessageBox( wxString::Format( "Save changes to \"%s\"?", ws->GetFilename() ), "GUI Editor", wxYES_NO | wxCANCEL | wxICON_QUESTION, this );
		if( r == wxCANCEL )
		{
			e.Veto();
			return;
		}

		if( r == wxYES )
		{
			ws->SaveFile( ws->GetFilename() );
		}
	}

	if( m_notebook->GetPageCount() <= 1 )
	{
		e.Veto();
		CallAfter( [this]()
		{
			Close( true );
		} );
		return;
	}

	e.Skip();
}

/*
================
rvGEApp::OnNotebookPageClosed
================
*/
void rvGEApp::OnNotebookPageClosed( wxAuiNotebookEvent& e )
{
	const int sel = e.GetSelection();
	rvGEWorkspace* ws = NULL;
	if( sel != wxNOT_FOUND && sel < ( int )m_notebook->GetPageCount() )
	{
		ws = dynamic_cast< rvGEWorkspace* >( m_notebook->GetPage( sel ) );
	}
	ActivateWorkspace( ws );
	e.Skip();
}

/*
================
rvGEApp::OnFileNew
================
*/
void rvGEApp::OnFileNew( wxCommandEvent& )
{
	NewWorkspace();
}

/*
================
rvGEApp::OnFileOpen
================
*/
void rvGEApp::OnFileOpen( wxCommandEvent& )
{
	wxFileDialog dlg( this, "Open GUI File", "", "", "GUI Files (*.gui)|*.gui|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST );
	if( dlg.ShowModal() == wxID_OK )
	{
		OpenFile( dlg.GetPath().utf8_str() );
	}
}

/*
================
rvGEApp::OnFileSave
================
*/
void rvGEApp::OnFileSave( wxCommandEvent& )
{
	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		ws->SaveFile( ws->GetFilename() );
	}
}

/*
================
rvGEApp::OnFileSaveAs
================
*/
void rvGEApp::OnFileSaveAs( wxCommandEvent& )
{
	rvGEWorkspace* ws = GetActiveWorkspace();
	if( !ws )
	{
		return;
	}

	wxFileDialog dlg( this, "Save GUI File", "", ws->GetFilename(), "GUI Files (*.gui)|*.gui|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
	if( dlg.ShowModal() == wxID_OK )
	{
		ws->SaveFile( dlg.GetPath().utf8_str() );
		m_options.AddRecentFile( ws->GetFilename() );
		m_options.Save();
		UpdateRecentFiles();
	}
}

/*
================
rvGEApp::OnFileClose
================
*/
void rvGEApp::OnFileClose( wxCommandEvent& )
{
	rvGEWorkspace* ws = GetActiveWorkspace();
	if( !ws )
	{
		return;
	}

	if( ws->IsModified() )
	{
		const int r = wxMessageBox( wxString::Format( "Save changes to \"%s\"?", ws->GetFilename() ), "GUI Editor", wxYES_NO | wxCANCEL | wxICON_QUESTION, this );
		if( r == wxCANCEL )
		{
			return;
		}

		if( r == wxYES )
		{
			ws->SaveFile( ws->GetFilename() );
		}
	}

	if( m_notebook->GetPageCount() > 1 )
	{
		m_notebook->DeletePage( m_notebook->GetSelection() );
	}
	else
	{
		CallAfter( [this]()
		{
			Close( true );
		} );
	}
}

/*
================
rvGEApp::OnFileExit
================
*/
void rvGEApp::OnFileExit( wxCommandEvent& )
{
	Close( true );
}

/*
================
rvGEApp::OnFileRecent
================
*/
void rvGEApp::OnFileRecent( wxCommandEvent& e )
{
	const int i = e.GetId() - 10000;
	const int count = m_options.GetRecentFileCount();
	if( i >= 0 && i < count )
	{
		OpenFile( m_options.GetRecentFile( count - 1 - i ) );
	}
}

/*
================
rvGEApp::OnEditUndo
================
*/

void rvGEApp::OnEditUndo( wxCommandEvent& )
{
	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		ws->GetModifierStack().Undo();
	}
}

/*
================
rvGEApp::OnEditRedo
================
*/
void rvGEApp::OnEditRedo( wxCommandEvent& )
{
	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		ws->GetModifierStack().Redo();
	}
}

/*
================
rvGEApp::OnEditCut
================
*/
void rvGEApp::OnEditCut( wxCommandEvent& )
{
	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		ws->Copy();
		ws->DeleteSelected();
	}
}

/*
================
rvGEApp::OnEditCopy
================
*/
void rvGEApp::OnEditCopy( wxCommandEvent& )
{
	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		ws->Copy();
	}
}

void rvGEApp::OnEditPaste( wxCommandEvent& )
{
	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		ws->Paste();
	}
}

/*
================
rvGEApp::OnEditDelete
================
*/
void rvGEApp::OnEditDelete( wxCommandEvent& )
{
	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		ws->DeleteSelected();
	}
}

/*
================
rvGEApp::OnHelpAbout
================
*/
void rvGEApp::OnHelpAbout( wxCommandEvent& )
{
	wxArrayString holders;
	holders.Add( "Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company." );
	holders.Add( "Copyright (C) 2005 Raven Software" );

	wxString copyright = wxJoin( holders, '\n' );

	AboutDialog dlg(
		this,
		"GUI Editor",
		wxString::Format( "Build %d - %s %s (x%u Build)", BUILD_NUMBER, ID__DATE__, ID__TIME__, ( unsigned int )( sizeof( void* ) * 8 ) ),
		copyright,
		true );

	dlg.SetIcon( GetIcon() );
	dlg.CentreOnParent();
	dlg.ShowModal();
}

/*
================
rvGEApp::OnViewOptions
================
*/
void rvGEApp::OnViewOptions( wxCommandEvent& )
{
	rvGEOptionsDlg dlg( this );
	if( dlg.ShowModal() == wxID_OK )
	{
		m_options.Save();
	}
}

/*
================
rvGEApp::OnViewToggleGrid
================
*/
void rvGEApp::OnViewToggleGrid( wxCommandEvent& )
{
	m_options.SetGridVisible( !m_options.GetGridVisible() );
	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		ws->RunFrame();
	}
}

/*
================
rvGEApp::OnViewToggleSnap
================
*/
void rvGEApp::OnViewToggleSnap( wxCommandEvent& )
{
	m_options.SetGridSnap( !m_options.GetGridSnap() );
}

/*
================
rvGEApp::OnViewZoomFit
================
*/
void rvGEApp::OnViewZoomFit( wxCommandEvent& )
{
	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		ws->ZoomFit();
	}
}

/*
================
rvGEApp::OnViewStatusBar
================
*/
void rvGEApp::OnViewStatusBar( wxCommandEvent& )
{
	const bool show = !m_options.GetStatusBarVisible();
	m_options.SetStatusBarVisible( show );
	GetStatusBar()->Show( show );
	SendSizeEvent();
}

/*
================
rvGEApp::OnToolsViewer

Opens the viewer.
================
*/
void rvGEApp::OnToolsViewer( wxCommandEvent& )
{
	if( m_viewer )
	{
		return;
	}

	m_viewer = new rvGEViewer( this );
	m_viewer->Show();

	if( rvGEWorkspace* ws = GetActiveWorkspace() )
	{
		if( !ws->IsModified() || wxMessageBox( "Save current file before opening the viewer?", "GUI Viewer", wxYES_NO | wxICON_QUESTION, this ) == wxYES )
		{
			ws->SaveFile( ws->GetFilename() );
		}
		m_viewer->OpenFile( ws->GetFilename() );
	}
}

/*
================
rvGEApp::OnToolsReloadMaterials
================
*/
void rvGEApp::OnToolsReloadMaterials( wxCommandEvent& )
{
	wxBusyCursor wait;
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "reloadImages\n" );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "reloadMaterials\n" );
}

/*
================
rvGEApp::CloseViewer
================
*/
void rvGEApp::CloseViewer()
{
	if( !m_viewer )
	{
		return;
	}

	m_viewer->Destroy();
	m_viewer = NULL;
}

/*
================
rvGEApp::OnUpdateUI
================
*/
void rvGEApp::OnUpdateUI( wxUpdateUIEvent& e )
{
	rvGEWorkspace* ws = GetActiveWorkspace();
	const bool hasWs = ws != NULL;
	const bool hasSel = hasWs && ws->GetSelectionMgr().Num() > 0;
	const bool oneSel = hasWs && ws->GetSelectionMgr().Num() == 1;

	switch( e.GetId() )
	{
		case ID_GUIED_FILE_SAVE:
		case ID_GUIED_FILE_SAVEAS:
		case ID_GUIED_FILE_CLOSE:
		case ID_GUIED_VIEW_ZOOMIN:
		case ID_GUIED_VIEW_ZOOMOUT:
		case ID_GUIED_VIEW_ZOOMFIT:
			e.Enable( hasWs );
			break;

		case ID_GUIED_EDIT_DELETE:
		case ID_GUIED_EDIT_COPY:
		case ID_GUIED_EDIT_CUT:
			e.Enable( hasSel );
			break;

		case ID_GUIED_EDIT_PASTE:
			e.Enable( hasWs && ws->GetClipboard().Num() > 0 );
			break;

		case ID_GUIED_EDIT_UNDO:
			e.Enable( hasWs && ws->GetModifierStack().CanUndo() );
			break;

		case ID_GUIED_EDIT_REDO:
			e.Enable( hasWs && ws->GetModifierStack().CanRedo() );
			break;

		case ID_GUIED_ITEM_PROPERTIES:
		case ID_GUIED_ITEM_SCRIPTS:
			e.Enable( oneSel );
			break;

		case ID_GUIED_VIEW_STATUSBAR:
			e.Check( m_options.GetStatusBarVisible() );
			break;

		case ID_GUIED_VIEW_SHOWGRID:
			e.Check( m_options.GetGridVisible() );
			break;

		case ID_GUIED_VIEW_SNAPTOGRID:
			e.Check( m_options.GetGridSnap() );
			break;

		default:
			e.Skip();
			break;
	}
}

/*
================
rvGEApp::OnClose
================
*/
void rvGEApp::OnClose( wxCloseEvent& )
{
	if( m_closing )
	{
		return;
	}

	m_options.SetWindowPlacement( "mdiframe", this );
	m_options.SetString( "AuiPerspective", m_aui.SavePerspective().utf8_str() );

	if( m_aui.GetPane( PANE_NAVIGATOR ).IsOk() )
	{
		m_options.SetNavigatorVisible( m_aui.GetPane( PANE_NAVIGATOR ).IsShown() );
	}

	if( m_aui.GetPane( PANE_PROPERTIES ).IsOk() )
	{
		m_options.SetPropertiesVisible( m_aui.GetPane( PANE_PROPERTIES ).IsShown() );
	}

	m_options.Save();

	for( size_t i = 0; i < m_notebook->GetPageCount(); ++i )
	{
		rvGEWorkspace* w = dynamic_cast< rvGEWorkspace* >( m_notebook->GetPage( i ) );
		if( w && w->IsModified() )
		{
			const int r = wxMessageBox( wxString::Format( "Save changes to \"%s\"?", w->GetFilename() ), "GUI Editor", wxYES_NO | wxCANCEL | wxICON_QUESTION, this );
			if( r == wxCANCEL )
			{
				return;
			}

			if( r == wxYES )
			{
				w->SaveFile( w->GetFilename() );
			}
		}
	}

	m_closing = true;
	Destroy();
}