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

#include "../../renderer/RenderCommon.h"

#include "MaterialEditor.h"
#include "MEMainFrame.h"
#include "MaterialEditView.h"
#include "MaterialPreviewView.h"
#include "MaterialPreviewPropView.h"
#include "MaterialPropTreeView.h"
#include "StageView.h"
#include "../common/ConsoleWidget.h"
#include "FindDialog.h"
#include "../common/ToolBarStrip.h"
#include "../common/AboutDialog.h"
#include "../common/ImageUtils.h"

static const char* const PANE_PREVIEW = "preview";
static const char* const PANE_TREE = "tree";
static const char* const PANE_PREVIEWPROP = "previewProps";
static const char* const PANE_CONSOLE = "console";

static const char* const STRIP_LEFT = "leftStrip";
static const char* const STRIP_RIGHT = "rightStrip";
static const char* const STRIP_BOTTOM = "bottomStrip";

/*
================
MEMainFrame::MEMainFrame
================
*/
MEMainFrame::MEMainFrame()
	: wxFrame( NULL, wxID_ANY, "Material Editor", wxDefaultPosition, wxSize( 1280, 800 ), wxDEFAULT_FRAME_STYLE )
	, m_materialEditFrame( NULL )
	, m_materialTreeView( NULL )
	, m_materialEditView( NULL )
	, m_previewPropertyView( NULL )
	, m_materialPreviewView( NULL )
	, m_consoleView( NULL )
	, m_stageView( NULL )
	, m_materialPropertyView( NULL )
	, m_materialEditSplitter( NULL )
	, m_leftStrip( NULL )
	, m_rightStrip( NULL )
	, m_bottomStrip( NULL )
	, m_find( NULL )
	, m_toolbarIcons( NULL )
	, m_shuttingDown( false )
{
	searchData.searched = false;

	const wxIcon icon = ImageUtils::IconFromRelativePath( "editors/icons/MaterialEditor.ico" );
	if( icon.IsOk() )
	{
		SetIcon( icon );
	}

	// Load the material property definitions from MaterialEditorDefs.med.
	MaterialDefManager::InitializeMaterialDefLists();

	// Load persisted options (layout, window placement, etc.).
	options.Load();

	// Tell AUI to manage this frame before restoring the saved perspective.
	m_auiManager.SetManagedWindow( this );

	BuildMenuBar();
	BuildToolBar();
	BuildStatusBar();

	// Material editor window.
	m_materialEditFrame = new wxFrame( this, wxID_ANY, "Material", wxDefaultPosition, wxSize( 540, 640 ), wxDEFAULT_FRAME_STYLE );
	m_materialEditView = new MaterialEditView( m_materialEditFrame );

	wxBoxSizer* editSizer = new wxBoxSizer( wxVERTICAL );
	editSizer->Add( m_materialEditView, 1, wxEXPAND );
	m_materialEditFrame->SetSizer( editSizer );

	// Hide instead of closing.
	m_materialEditFrame->Bind( wxEVT_CLOSE_WINDOW, [this]( wxCloseEvent & e )
	{
		if( m_shuttingDown )
		{
			e.Skip();
			return;
		}

		if( e.CanVeto() )
		{
			m_materialEditFrame->Hide();
			e.Veto();
		}
		else
		{
			e.Skip();
		}
	} );

	m_materialEditFrame->Hide();

	// Dockable views.
	m_materialTreeView = new MaterialTreeView( this );
	m_previewPropertyView = new MaterialPreviewPropView( this );
	m_materialPreviewView = new MaterialPreviewView( this );
	m_consoleView = new ConsoleWidget( this );

	m_consoleView->SetQuitHandler( [this]()
	{
		this->Close( true );
	} );


	m_stageView = m_materialEditView->GetStageView();
	m_materialPropertyView = m_materialEditView->GetMaterialPropTreeView();
	m_materialEditSplitter = m_materialEditView->GetEditSplitter();

	if( m_stageView && m_materialPropertyView )
	{
		m_stageView->SetMaterialPropertyView( m_materialPropertyView );
	}

	if( m_materialEditSplitter )
	{
		const int stageW = options.GetStageWidth();
		if( stageW > 0 )
		{
			m_materialEditSplitter->SetSashPosition( stageW );
		}
	}

	if( m_previewPropertyView && m_materialPreviewView )
	{
		m_previewPropertyView->RegisterPreviewView( m_materialPreviewView );
	}

	if( m_previewPropertyView )
	{
		m_previewPropertyView->InitializePropTree();
		m_previewPropertyView->SetColumn( options.GetPreviewPropHeadingWidth() > 0 ? options.GetPreviewPropHeadingWidth() : 120 );
	}

	if( m_materialPropertyView )
	{
		m_materialPropertyView->LoadSettings();
		m_materialPropertyView->SetColumn( options.GetMaterialPropHeadingWidth() > 0 ? options.GetMaterialPropHeadingWidth() : 200 );
	}

	// Dockable panes.
	m_auiManager.AddPane( m_materialPreviewView, wxAuiPaneInfo().Name( PANE_PREVIEW ).CenterPane().Caption( "Preview" )
						  .MaximizeButton( true ).MinimizeButton( true ).CloseButton( false ).Floatable( true ).PinButton( true ) );

	m_auiManager.AddPane( m_materialTreeView, wxAuiPaneInfo().Name( PANE_TREE ).Left().Caption( "Materials" ).BestSize( 300, -1 ).MinSize( 140, -1 )
						  .MaximizeButton( true ).Floatable( true ).PinButton( true ).CloseButton( false ) );

	m_auiManager.AddPane( m_previewPropertyView, wxAuiPaneInfo().Name( PANE_PREVIEWPROP ).Right().Caption( "Preview Properties" )
						  .BestSize( 300, -1 ).MinSize( 180, -1 ).MaximizeButton( true ).Floatable( true ).PinButton( true ).CloseButton( false ).Row( 0 ) );

	m_auiManager.AddPane( m_consoleView, wxAuiPaneInfo().Name( PANE_CONSOLE ).Bottom().Caption( "Console" ).BestSize( -1, 220 ).MinSize( -1, 80 )
						  .MaximizeButton( true ).Floatable( true ).PinButton( true ).CloseButton( false ) );

	BuildAutoHideStrips();

	// Restore saved perspective or fall back to a sane default.
	const char* savedPerspective = options.GetString( "auiPerspective", "" );
	bool restored = false;
	if( savedPerspective && *savedPerspective )
	{
		wxString persp = wxString::FromUTF8( savedPerspective );
		restored = m_auiManager.LoadPerspective( persp, false );
	}

	if( !restored )
	{
		m_auiManager.GetPane( PANE_PREVIEW ).Dock().Center();
		m_auiManager.GetPane( PANE_TREE ).Dock().Left();
		m_auiManager.GetPane( PANE_PREVIEWPROP ).Dock().Right().Row( 0 );
		m_auiManager.GetPane( PANE_CONSOLE ).Dock().Bottom();
	}

	m_auiManager.Update();

	RebuildAutoHideStrips();

	// Restore window placement if we have it.
	options.GetWindowPlacement( "mainframe", this );

	// Do a re-layout.
	CallAfter( [this]()
	{
		m_auiManager.Update();
		SendSizeEvent();
		Layout();
		Refresh();
	} );

	// Hook up events.
	Bind( wxEVT_CLOSE_WINDOW, &MEMainFrame::OnClose, this );

	// File
	Bind( wxEVT_MENU, &MEMainFrame::OnFileOpenMaterial, this, ID_ME_FILE_OPEN );
	Bind( wxEVT_MENU, &MEMainFrame::OnFileShowAllMaterials, this, ID_ME_FILE_SHOW_ALL_MATERIALS );
	Bind( wxEVT_MENU, &MEMainFrame::OnFileExit, this, ID_ME_FILE_EXIT );
	Bind( wxEVT_MENU, &MEMainFrame::OnFileSaveMaterial, this, ID_ME_FILE_SAVEMATERIAL );
	Bind( wxEVT_MENU, &MEMainFrame::OnFileSaveFile, this, ID_ME_FILE_SAVEFILE );
	Bind( wxEVT_MENU, &MEMainFrame::OnFileSaveAll, this, ID_ME_FILE_SAVE );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnFileSaveMaterialUpdate, this, ID_ME_FILE_SAVEMATERIAL );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnFileSaveFileUpdate, this, ID_ME_FILE_SAVEFILE );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnFileSaveAllUpdate, this, ID_ME_FILE_SAVE );

	// Preview / Apply
	Bind( wxEVT_MENU, &MEMainFrame::OnApplyMaterial, this, ID_ME_PREVIEW_APPLYCHANGES );
	Bind( wxEVT_MENU, &MEMainFrame::OnApplyFile, this, ID_ME_PREVIEW_APPLYFILE );
	Bind( wxEVT_MENU, &MEMainFrame::OnApplyAll, this, ID_ME_PREVIEW_APPLYALL );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnApplyMaterialUpdate, this, ID_ME_PREVIEW_APPLYCHANGES );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnApplyFileUpdate, this, ID_ME_PREVIEW_APPLYFILE );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnApplyAllUpdate, this, ID_ME_PREVIEW_APPLYALL );

	// Edit
	Bind( wxEVT_MENU, &MEMainFrame::OnEditCut, this, ID_ME_EDIT_CUT );
	Bind( wxEVT_MENU, &MEMainFrame::OnEditCopy, this, ID_ME_EDIT_COPY );
	Bind( wxEVT_MENU, &MEMainFrame::OnEditPaste, this, ID_ME_EDIT_PASTE );
	Bind( wxEVT_MENU, &MEMainFrame::OnEditDelete, this, ID_ME_EDIT_DELETE );
	Bind( wxEVT_MENU, &MEMainFrame::OnEditRename, this, ID_ME_EDIT_RENAME );
	Bind( wxEVT_MENU, &MEMainFrame::OnEditFind, this, ID_ME_EDIT_FIND );
	Bind( wxEVT_MENU, &MEMainFrame::OnEditFindNext, this, ID_ME_EDIT_FIND_NEXT );
	Bind( wxEVT_MENU, &MEMainFrame::OnEditUndo, this, ID_ME_EDIT_UNDO );
	Bind( wxEVT_MENU, &MEMainFrame::OnEditRedo, this, ID_ME_EDIT_REDO );

	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnEditCutUpdate, this, ID_ME_EDIT_CUT );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnEditCopyUpdate, this, ID_ME_EDIT_COPY );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnEditPasteUpdate, this, ID_ME_EDIT_PASTE );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnEditDeleteUpdate, this, ID_ME_EDIT_DELETE );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnEditRenameUpdate, this, ID_ME_EDIT_RENAME );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnEditUndoUpdate, this, ID_ME_EDIT_UNDO );
	Bind( wxEVT_UPDATE_UI, &MEMainFrame::OnEditRedoUpdate, this, ID_ME_EDIT_REDO );

	// View
	Bind( wxEVT_MENU, &MEMainFrame::OnViewIncludeFile, this, ID_VIEW_INCLUDEFILENAME );
	Bind( wxEVT_MENU, &MEMainFrame::OnViewResetLayout, this, ID_ME_VIEW_RESET_LAYOUT );

	// Preview reloads
	Bind( wxEVT_MENU, &MEMainFrame::OnReloadShaders, this, ID_PREVIEW_RELOADSHADERS );
	Bind( wxEVT_MENU, &MEMainFrame::OnReloadImages, this, ID_PREVIEW_RELOADIMAGES );

	// Help
	Bind( wxEVT_MENU, &MEMainFrame::OnAbout, this, IDD_ME_ABOUTBOX );

	// AUI notifications.
	Bind( wxEVT_AUI_PANE_CLOSE, &MEMainFrame::OnAuiPaneClose, this );
	Bind( wxEVT_AUI_PANE_MAXIMIZE, &MEMainFrame::OnAuiPaneMaximize, this );
	Bind( wxEVT_AUI_PANE_RESTORE, &MEMainFrame::OnAuiPaneRestore, this );
	Bind( wxEVT_AUI_PANE_BUTTON, &MEMainFrame::OnAuiPaneButton, this );

	// Register views with the document manager.
	RegisterViews();
}

/*
================
MEMainFrame::~MEMainFrame
================
*/
MEMainFrame::~MEMainFrame()
{
	extern MEMainFrame* meMainFrame;
	if( meMainFrame == this )
	{
		meMainFrame = NULL;
	}

	if( m_find )
	{
		m_find->Destroy();
		m_find = NULL;
	}

	if( m_toolbarIcons )
	{
		delete m_toolbarIcons;
		m_toolbarIcons = NULL;
	}
}

/*
================
MEMainFrame::BuildAutoHideStrips
================
*/
void MEMainFrame::BuildAutoHideStrips()
{
	const int thickness = AutoHideStrip::GetThickness();

	m_leftStrip = new AutoHideStrip( this, AutoHideStrip::Left );
	m_rightStrip = new AutoHideStrip( this, AutoHideStrip::Right );
	m_bottomStrip = new AutoHideStrip( this, AutoHideStrip::Bottom );

	m_leftStrip->SetTabClickHandler( [this]( wxWindow * w )
	{
		OnAutoHideTabClick( w );
	} );
	m_rightStrip->SetTabClickHandler( [this]( wxWindow * w )
	{
		OnAutoHideTabClick( w );
	} );
	m_bottomStrip->SetTabClickHandler( [this]( wxWindow * w )
	{
		OnAutoHideTabClick( w );
	} );

	auto stripInfo = [&]( const wxString & name ) -> wxAuiPaneInfo
	{
		return wxAuiPaneInfo()
		.Name( name )
		.CaptionVisible( false )
		.CloseButton( false )
		.PinButton( false )
		.MaximizeButton( false )
		.MinimizeButton( false )
		.Gripper( false )
		.PaneBorder( false )
		.Floatable( false )
		.Movable( false )
		.Resizable( false )
		.Layer( 10 )
		.Hide();
	};

	m_auiManager.AddPane( m_leftStrip, stripInfo( STRIP_LEFT ).Left().BestSize( thickness, -1 ).MinSize( thickness, -1 ).MaxSize( thickness, -1 ) );
	m_auiManager.AddPane( m_rightStrip, stripInfo( STRIP_RIGHT ).Right().BestSize( thickness, -1 ).MinSize( thickness, -1 ).MaxSize( thickness, -1 ) );
	m_auiManager.AddPane( m_bottomStrip, stripInfo( STRIP_BOTTOM ).Bottom().BestSize( -1, thickness ).MinSize( -1, thickness ).MaxSize( -1, thickness ) );
}

/*
================
MEMainFrame::GetStripForPane
================
*/
AutoHideStrip* MEMainFrame::GetStripForPane( wxAuiPaneInfo& pane ) const
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
MEMainFrame::RebuildAutoHideStrips
================
*/
void MEMainFrame::RebuildAutoHideStrips()
{
	if( !m_leftStrip || !m_rightStrip || !m_bottomStrip )
	{
		return;
	}

	m_leftStrip->Clear();
	m_rightStrip->Clear();
	m_bottomStrip->Clear();

	const wxAuiPaneInfoArray& panes = m_auiManager.GetAllPanes();

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

	m_auiManager.GetPane( m_leftStrip ).Show( m_leftStrip->GetItemCount() > 0 );
	m_auiManager.GetPane( m_rightStrip ).Show( m_rightStrip->GetItemCount() > 0 );
	m_auiManager.GetPane( m_bottomStrip ).Show( m_bottomStrip->GetItemCount() > 0 );

	m_auiManager.Update();
}

/*
================
MEMainFrame::OnAutoHideTabClick
================
*/
void MEMainFrame::OnAutoHideTabClick( wxWindow* paneWindow )
{
	if( !paneWindow )
	{
		return;
	}

	wxAuiPaneInfoArray& panes = m_auiManager.GetAllPanes();
	for( size_t i = 0; i < panes.GetCount(); ++i )
	{
		if( panes[i].IsMaximized() )
		{
			m_auiManager.RestorePane( panes[i] );
			break;
		}
	}

	m_auiManager.GetPane( paneWindow ).Show( true );
	m_auiManager.Update();

	RebuildAutoHideStrips();

	CallAfter( [this]()
	{
		m_auiManager.Update();
		SendSizeEvent();
		Layout();
		Refresh();
	} );
}

/*
================
MEMainFrame::ApplyDefaultAuiPerspective
================
*/
void MEMainFrame::ApplyDefaultAuiPerspective( bool maximizePreview )
{
	wxAuiPaneInfoArray& panes = m_auiManager.GetAllPanes();
	for( size_t i = 0; i < panes.GetCount(); ++i )
	{
		if( panes[i].IsMaximized() )
		{
			m_auiManager.RestorePane( panes[i] );
		}
	}

	m_auiManager.GetPane( PANE_PREVIEW ).Dock().Center().Show();
	m_auiManager.GetPane( PANE_TREE ).Dock().Left().Show();
	m_auiManager.GetPane( PANE_PREVIEWPROP ).Dock().Right().Row( 0 ).Show();
	m_auiManager.GetPane( PANE_CONSOLE ).Dock().Bottom().Show();

	m_auiManager.Update();

	RebuildAutoHideStrips();

	CallAfter( [this]()
	{
		m_auiManager.Update();
		SendSizeEvent();
		Layout();
		Refresh();
	} );
}

/*
================
MEMainFrame::BuildMenuBar
================
*/
void MEMainFrame::BuildMenuBar()
{
	wxMenu* fileMenu = new wxMenu();
	fileMenu->Append( ID_ME_FILE_OPEN, "&Open Material File...\tCtrl-O" );
	fileMenu->Append( ID_ME_FILE_SHOW_ALL_MATERIALS, "Show &All Materials" );
	fileMenu->AppendSeparator();
	fileMenu->Append( ID_ME_FILE_SAVEMATERIAL, "Save &Material" );
	fileMenu->Append( ID_ME_FILE_SAVEFILE, "Save &File\tCtrl-S" );
	fileMenu->Append( ID_ME_FILE_SAVE, "&Save All" );
	fileMenu->AppendSeparator();
	fileMenu->Append( ID_ME_FILE_EXIT, "E&xit" );

	wxMenu* editMenu = new wxMenu();
	editMenu->Append( ID_ME_EDIT_UNDO, "Undo\tCtrl-Z" );
	editMenu->Append( ID_ME_EDIT_REDO, "Redo\tCtrl-Y" );
	editMenu->AppendSeparator();
	editMenu->Append( ID_ME_EDIT_CUT, "Cut\tCtrl-X" );
	editMenu->Append( ID_ME_EDIT_COPY, "Copy\tCtrl-C" );
	editMenu->Append( ID_ME_EDIT_PASTE, "Paste\tCtrl-V" );
	editMenu->Append( ID_ME_EDIT_DELETE, "Delete\tDel" );
	editMenu->Append( ID_ME_EDIT_RENAME, "Rename\tF2" );
	editMenu->AppendSeparator();
	editMenu->Append( ID_ME_EDIT_FIND, "Find\tCtrl-F" );

	wxMenu* viewMenu = new wxMenu();
	viewMenu->AppendCheckItem( ID_VIEW_INCLUDEFILENAME, "&Include Filename" );
	viewMenu->Check( ID_VIEW_INCLUDEFILENAME, true );
	viewMenu->AppendSeparator();
	viewMenu->Append( ID_ME_VIEW_RESET_LAYOUT, "Reset Layout" );

	wxMenu* previewMenu = new wxMenu();
	previewMenu->Append( ID_ME_PREVIEW_APPLYCHANGES, "Apply Material\tCtrl-A" );
	previewMenu->Append( ID_ME_PREVIEW_APPLYFILE, "Apply File" );
	previewMenu->Append( ID_ME_PREVIEW_APPLYALL, "Apply All" );
	previewMenu->AppendSeparator();
	previewMenu->Append( ID_PREVIEW_RELOADSHADERS, "&Reload Shaders\tCtrl-R" );
	previewMenu->Append( ID_PREVIEW_RELOADIMAGES, "Reload Images\tCtrl-I" );

	wxMenu* helpMenu = new wxMenu();
	helpMenu->Append( IDD_ME_ABOUTBOX, "&About Material Editor..." );

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append( fileMenu, "&File" );
	menuBar->Append( editMenu, "&Edit" );
	menuBar->Append( viewMenu, "&Material View" );
	menuBar->Append( previewMenu, "&Preview" );
	menuBar->Append( helpMenu, "&Help" );

	SetMenuBar( menuBar );
}

/*
================
MEMainFrame::BuildToolBar
================
*/
void MEMainFrame::BuildToolBar()
{
	wxToolBar* tb = CreateToolBar( wxTB_FLAT | wxTB_HORIZONTAL | wxTB_TEXT );
	if( !tb )
	{
		return;
	}

	rvToolbarImageStrip strip;
	if( !strip.Load( "editors/gfx/MEtoolbar.bmp", 16, 15 ) )
	{
		common->Warning( "MEMainFrame: could not load toolbar bitmap" );
	}
	else
	{
		m_toolbarIcons = strip.CreateImageList();
		tb->SetToolBitmapSize( wxSize( strip.GetIconWidth(), strip.GetIconHeight() ) );

		int icon = 0;
		auto add = [&]( int id, const char* label, bool sep = false )
		{
			if( sep )
			{
				tb->AddSeparator();
				return;
			}
			if( m_toolbarIcons && icon < strip.GetIconCount() )
			{
				tb->AddTool( id, label, strip.GetIcon( icon ) );
			}
			else
			{
				tb->AddTool( id, label, wxNullBitmap );
			}
			++icon;
		};

		add( ID_ME_FILE_SAVEMATERIAL, "Save Material" );
		add( ID_ME_FILE_SAVEFILE, "Save File" );
		add( ID_ME_FILE_SAVE, "Save All" );
		add( 0, "", true );
		add( ID_ME_EDIT_UNDO, "Undo" );
		add( ID_ME_EDIT_REDO, "Redo" );
		add( 0, "", true );
		add( ID_ME_EDIT_CUT, "Cut" );
		add( ID_ME_EDIT_COPY, "Copy" );
		add( ID_ME_EDIT_PASTE, "Paste" );
		add( ID_ME_EDIT_DELETE, "Delete" );
		add( ID_ME_EDIT_RENAME, "Rename" );
		add( ID_ME_EDIT_FIND, "Find" );
		add( 0, "", true );
		add( ID_ME_PREVIEW_APPLYCHANGES, "Apply" );
		add( ID_ME_PREVIEW_APPLYFILE, "Apply File" );
		add( ID_ME_PREVIEW_APPLYALL, "Apply All" );
		add( 0, "", true );
		add( ID_PREVIEW_RELOADSHADERS, "Reload Shaders" );
		add( ID_PREVIEW_RELOADIMAGES, "Reload Images" );
	}

	tb->Realize();
}

/*
================
MEMainFrame::BuildStatusBar
================
*/
void MEMainFrame::BuildStatusBar()
{
	CreateStatusBar( 1 );
	SetStatusText( "Ready" );
}

/*
================
MEMainFrame::RegisterViews
================
*/
void MEMainFrame::RegisterViews()
{
	materialDocManager.RegisterMaterialView( this );

	if( m_materialTreeView )
	{
		materialDocManager.RegisterMaterialView( m_materialTreeView );
	}
	if( m_stageView )
	{
		materialDocManager.RegisterMaterialView( m_stageView );
	}
	if( m_materialPropertyView )
	{
		materialDocManager.RegisterMaterialView( m_materialPropertyView );
	}
	if( m_materialPreviewView )
	{
		materialDocManager.RegisterMaterialView( m_materialPreviewView );
	}
	if( m_materialEditView )
	{
		materialDocManager.RegisterMaterialView( m_materialEditView );
	}
}

/*
================
MEMainFrame::PrintConsoleMessage
================
*/
void MEMainFrame::PrintConsoleMessage( const char* msg )
{
	if( m_consoleView )
	{
		m_consoleView->AddText( msg );
	}
}

/*
================
MEMainFrame::OnFileExit
================
*/
void MEMainFrame::OnFileExit( wxCommandEvent& )
{
	Close( true );
}

/*
================
MEMainFrame::OnFileSaveMaterial
================
*/
void MEMainFrame::OnFileSaveMaterial( wxCommandEvent& )
{
	MaterialDoc* material = materialDocManager.GetCurrentMaterialDoc();
	if( material )
	{
		materialDocManager.SaveMaterial( material );
	}
}

/*
================
MEMainFrame::OnFileSaveFile
================
*/
void MEMainFrame::OnFileSaveFile( wxCommandEvent& )
{
	if( !m_materialTreeView )
	{
		return;
	}

	idStr filename = m_materialTreeView->GetSaveFilename();
	if( filename.Length() > 0 )
	{
		materialDocManager.SaveFile( filename );
	}
}

/*
================
MEMainFrame::OnFileSaveAll
================
*/
void MEMainFrame::OnFileSaveAll( wxCommandEvent& )
{
	materialDocManager.SaveAllMaterials();
}

/*
================
MEMainFrame::OnApplyMaterial
================
*/
void MEMainFrame::OnApplyMaterial( wxCommandEvent& )
{
	MaterialDoc* material = materialDocManager.GetCurrentMaterialDoc();
	if( material )
	{
		materialDocManager.ApplyMaterial( material );
	}
}

/*
================
MEMainFrame::OnApplyFile
================
*/
void MEMainFrame::OnApplyFile( wxCommandEvent& )
{
	if( !m_materialTreeView )
	{
		return;
	}

	idStr filename = m_materialTreeView->GetSaveFilename();
	if( filename.Length() > 0 )
	{
		materialDocManager.ApplyFile( filename );
	}
}

/*
================
MEMainFrame::OnApplyAll
================
*/
void MEMainFrame::OnApplyAll( wxCommandEvent& )
{
	materialDocManager.ApplyAll();
}

/*
================
MEMainFrame::OnEditCut
================
*/
void MEMainFrame::OnEditCut( wxCommandEvent& )
{
	if( m_materialTreeView )
	{
		m_materialTreeView->OnCut();
	}
}

/*
================
MEMainFrame::OnEditCopy
================
*/
void MEMainFrame::OnEditCopy( wxCommandEvent& )
{
	if( m_materialTreeView )
	{
		m_materialTreeView->OnCopy();
	}
}

/*
================
MEMainFrame::OnEditPaste
================
*/
void MEMainFrame::OnEditPaste( wxCommandEvent& )
{
	if( m_materialTreeView )
	{
		m_materialTreeView->OnPaste();
	}
}

/*
================
MEMainFrame::OnEditDelete
================
*/
void MEMainFrame::OnEditDelete( wxCommandEvent& )
{
	if( m_materialTreeView )
	{
		m_materialTreeView->OnDeleteMaterial();
	}
}

/*
================
MEMainFrame::OnEditRename
================
*/
void MEMainFrame::OnEditRename( wxCommandEvent& )
{
	if( m_materialTreeView )
	{
		m_materialTreeView->OnRenameMaterial();
	}
}

/*
================
MEMainFrame::OnEditFind
================
*/
void MEMainFrame::OnEditFind( wxCommandEvent& )
{
	if( m_find == NULL )
	{
		m_find = new FindDialog( this );
		m_find->Show();
	}
	else
	{
		m_find->Raise();
	}
}

/*
================
MEMainFrame::OnEditFindNext
================
*/
void MEMainFrame::OnEditFindNext( wxCommandEvent& )
{
	FindNext( NULL );
}

/*
================
MEMainFrame::OnEditUndo
================
*/
void MEMainFrame::OnEditUndo( wxCommandEvent& )
{
	materialDocManager.Undo();
}

/*
================
MEMainFrame::OnEditRedo
================
*/
void MEMainFrame::OnEditRedo( wxCommandEvent& )
{
	materialDocManager.Redo();
}

/*
================
MEMainFrame::OnViewIncludeFile
================
*/
void MEMainFrame::OnViewIncludeFile( wxCommandEvent& event )
{
	bool checked = event.IsChecked();
	if( m_materialTreeView )
	{
		m_materialTreeView->InitializeMaterialList( checked );
	}
}

/*
================
MEMainFrame::OnViewResetLayout
================
*/
void MEMainFrame::OnViewResetLayout( wxCommandEvent& )
{
	ApplyDefaultAuiPerspective( true );
}

/*
================
MEMainFrame::OnReloadShaders
================
*/
void MEMainFrame::OnReloadShaders( wxCommandEvent& )
{
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "reloadShaders" );
}

/*
================
MEMainFrame::OnReloadImages
================
*/
void MEMainFrame::OnReloadImages( wxCommandEvent& )
{
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "reloadImages" );
}

/*
================
MEMainFrame::OnAuiPaneClose
================
*/
void MEMainFrame::OnAuiPaneClose( wxAuiManagerEvent& event )
{
	m_auiManager.Update();
	RebuildAutoHideStrips();
	event.Skip();
}

/*
================
MEMainFrame::OnAuiPaneMaximize
================
*/
void MEMainFrame::OnAuiPaneMaximize( wxAuiManagerEvent& event )
{
	m_auiManager.Update();
	RebuildAutoHideStrips();
	event.Skip();
}

/*
================
MEMainFrame::OnAuiPaneRestore
================
*/
void MEMainFrame::OnAuiPaneRestore( wxAuiManagerEvent& event )
{
	m_auiManager.Update();
	RebuildAutoHideStrips();
	event.Skip();
}

/*
================
MEMainFrame::OnAuiPaneButton
================
*/
void MEMainFrame::OnAuiPaneButton( wxAuiManagerEvent& event )
{
	if( event.GetButton() == wxAUI_BUTTON_PIN )
	{
		wxAuiPaneInfo* pane = event.GetPane();
		if( pane && pane->window )
		{
			const int dir = pane->dock_direction;

			m_auiManager.GetPane( pane->window ).Show( false );
			m_auiManager.GetPane( pane->window ).dock_direction = dir;
			m_auiManager.Update();

			RebuildAutoHideStrips();

			event.Veto();
			return;
		}
	}

	event.Skip();
}

/*
================
MEMainFrame::OnAbout
================
*/
void MEMainFrame::OnAbout( wxCommandEvent& )
{
	AboutDialog dlg(
		this,
		"Material Editor",
		wxString::Format( "Build %d - %s %s (x%u Build)", BUILD_NUMBER, ID__DATE__, ID__TIME__, ( unsigned int )( sizeof( void* ) * 8 ) ),
		"Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company." );

	dlg.SetIcon( GetIcon() );
	dlg.CenterOnParent();
	dlg.ShowModal();
}

/*
================
MEMainFrame::OnClose
================
*/
void MEMainFrame::OnClose( wxCloseEvent& event )
{
	if( m_shuttingDown )
	{
		event.Skip();
		return;
	}

	m_shuttingDown = true;

	wxString perspective = m_auiManager.SavePerspective();
	options.SetString( "auiPerspective", perspective.ToUTF8().data() );

	if( m_materialPropertyView && m_auiManager.GetPane( m_materialPropertyView ).IsShown() )
	{
		options.SetMaterialPropHeadingWidth( m_materialPropertyView->GetColumn() );
		m_materialPropertyView->SaveSettings();
	}

	if( m_previewPropertyView && m_auiManager.GetPane( m_previewPropertyView ).IsShown() )
	{
		options.SetPreviewPropHeadingWidth( m_previewPropertyView->GetColumn() );
	}

	if( m_materialEditSplitter )
	{
		options.SetStageWidth( m_materialEditSplitter->GetSashPosition() );
	}

	options.SetWindowPlacement( "mainframe", this );
	options.Save();

	// Close the Find dialog.
	if( m_find )
	{
		m_find->Destroy();
		m_find = NULL;
	}

	// Hide the edit window.
	if( m_materialEditFrame )
	{
		m_materialEditFrame->Hide();
	}

	// Free the material property definition lists.
	MaterialDefManager::DestroyMaterialDefLists();

	// Shut down AUI before the frame is destroyed.
	m_auiManager.UnInit();

	// Free the toolbar image list.
	if( m_toolbarIcons )
	{
		delete m_toolbarIcons;
		m_toolbarIcons = NULL;
	}

	event.Skip();
}

/*
================
MEMainFrame::OnFileOpenMaterial
================
*/
void MEMainFrame::OnFileOpenMaterial( wxCommandEvent& )
{
	if( !m_materialTreeView )
	{
		return;
	}

	const char* baseDir = fileSystem->RelativePathToOSPath( "materials", "fs_basepath" );

	wxFileDialog dlg( this, "Open Material File", baseDir ? baseDir : "", "", "Material files (*.mtr)|*.mtr|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST );
	if( dlg.ShowModal() != wxID_OK )
	{
		return;
	}

	idStr relative = fileSystem->OSPathToRelativePath( dlg.GetPath().ToUTF8().data() );
	relative.BackSlashesToSlashes();

	materialDocManager.ReloadFile( relative.c_str() );

	m_materialTreeView->InitializeMaterialList( true, relative.c_str() );
}

/*
================
MEMainFrame::OnFileShowAllMaterials
================
*/
void MEMainFrame::OnFileShowAllMaterials( wxCommandEvent& )
{
	if( m_materialTreeView )
	{
		m_materialTreeView->InitializeMaterialList( true, NULL );
	}
}

/*
================
MEMainFrame::OnFileSaveMaterialUpdate
================
*/
void MEMainFrame::OnFileSaveMaterialUpdate( wxUpdateUIEvent& event )
{
	MaterialDoc* pDoc = materialDocManager.GetCurrentMaterialDoc();
	event.Enable( pDoc && pDoc->modified );
}

/*
================
MEMainFrame::OnFileSaveFileUpdate
================
*/
void MEMainFrame::OnFileSaveFileUpdate( wxUpdateUIEvent& event )
{
	event.Enable( m_materialTreeView && m_materialTreeView->CanSaveFile() );
}

/*
================
MEMainFrame::OnFileSaveAllUpdate
================
*/
void MEMainFrame::OnFileSaveAllUpdate( wxUpdateUIEvent& event )
{
	event.Enable( materialDocManager.IsAnyModified() );
}

/*
================
MEMainFrame::OnApplyMaterialUpdate
================
*/
void MEMainFrame::OnApplyMaterialUpdate( wxUpdateUIEvent& event )
{
	MaterialDoc* pDoc = materialDocManager.GetCurrentMaterialDoc();
	event.Enable( pDoc && pDoc->applyWaiting );
}

/*
================
MEMainFrame::OnApplyFileUpdate
================
*/
void MEMainFrame::OnApplyFileUpdate( wxUpdateUIEvent& event )
{
	MaterialDoc* pDoc = materialDocManager.GetCurrentMaterialDoc();
	event.Enable( pDoc && materialDocManager.DoesFileNeedApply( pDoc->renderMaterial->GetFileName() ) );
}

/*
================
MEMainFrame::OnApplyAllUpdate
================
*/
void MEMainFrame::OnApplyAllUpdate( wxUpdateUIEvent& event )
{
	event.Enable( materialDocManager.DoesAnyNeedApply() );
}

/*
================
MEMainFrame::OnEditCutUpdate
================
*/
void MEMainFrame::OnEditCutUpdate( wxUpdateUIEvent& event )
{
	event.Enable( m_materialTreeView && m_materialTreeView->CanCut() );
}

/*
================
MEMainFrame::OnEditCopyUpdate
================
*/
void MEMainFrame::OnEditCopyUpdate( wxUpdateUIEvent& event )
{
	event.Enable( m_materialTreeView && m_materialTreeView->CanCopy() );
}

/*
================
MEMainFrame::OnEditPasteUpdate
================
*/
void MEMainFrame::OnEditPasteUpdate( wxUpdateUIEvent& event )
{
	event.Enable( m_materialTreeView && m_materialTreeView->CanPaste() );
}

/*
================
MEMainFrame::OnEditDeleteUpdate
================
*/
void MEMainFrame::OnEditDeleteUpdate( wxUpdateUIEvent& event )
{
	event.Enable( m_materialTreeView && m_materialTreeView->CanDelete() );
}

/*
================
MEMainFrame::OnEditRenameUpdate
================
*/
void MEMainFrame::OnEditRenameUpdate( wxUpdateUIEvent& event )
{
	event.Enable( m_materialTreeView && m_materialTreeView->CanRename() );
}

/*
================
MEMainFrame::OnEditUndoUpdate
================
*/
void MEMainFrame::OnEditUndoUpdate( wxUpdateUIEvent& event )
{
	event.Enable( materialDocManager.IsUndoAvailable() );
}

/*
================
MEMainFrame::OnEditRedoUpdate
================
*/
void MEMainFrame::OnEditRedoUpdate( wxUpdateUIEvent& event )
{
	event.Enable( materialDocManager.IsRedoAvailable() );
}

/*
================
MEMainFrame::CloseFind
================
*/
void MEMainFrame::CloseFind()
{
	m_find = NULL;
}

/*
================
MEMainFrame::FindNext
================
*/
void MEMainFrame::FindNext( MaterialSearchData_t* search )
{
	if( search )
	{
		searchData = *search;
	}
	else if( !searchData.searched )
	{
		return;
	}

	if( !m_materialTreeView || !m_materialTreeView->FindNextMaterial( &searchData ) )
	{
		wxMessageBox( wxString::Format( "Unable to find '%s'.", searchData.searchText.c_str() ), "Find", wxOK | wxICON_INFORMATION, this );
	}

	searchData.searched = true;
}

/*
================
MEMainFrame::MV_OnMaterialSelectionChange
================
*/
void MEMainFrame::MV_OnMaterialSelectionChange( MaterialDoc* pMaterial )
{
	if( !m_materialEditFrame )
	{
		return;
	}

	if( pMaterial && pMaterial->renderMaterial )
	{
		m_materialEditFrame->SetTitle( pMaterial->renderMaterial->GetName() );

		if( !m_materialEditFrame->IsShown() )
		{
			m_materialEditFrame->Show();
		}
	}
	else
	{
		m_materialEditFrame->Hide();
	}
}