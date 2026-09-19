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

/*
================
MEMainFrame::MEMainFrame
================
*/
MEMainFrame::MEMainFrame()
	: wxFrame( NULL, wxID_ANY, "Material Editor", wxDefaultPosition, wxSize( 1280, 800 ), wxDEFAULT_FRAME_STYLE )
	, m_tabs( NULL )
	, m_editorPage( NULL )
	, m_consolePage( NULL )
	, m_mainSplitter( NULL )
	, m_editSplitter( NULL )
	, m_previewSplitter( NULL )
	, m_materialTreeView( NULL )
	, m_materialEditView( NULL )
	, m_previewPropertyView( NULL )
	, m_materialPreviewView( NULL )
	, m_consoleView( NULL )
	, m_stageView( NULL )
	, m_materialPropertyView( NULL )
	, m_materialEditSplitter( NULL )
	, m_find( NULL )
	, m_toolbarIcons( NULL )
{
	searchData.searched = false;

	// Load the material property definitions from MaterialEditorDefs.med.
	MaterialDefManager::InitializeMaterialDefLists();

	// Load persisted options (splitter sizes, window placement, etc.).
	options.Load();

	// Restore window placement if we have it.
	options.GetWindowPlacement( "mainframe", this );

	BuildMenuBar();
	BuildToolBar();
	BuildStatusBar();

	// Central notebook with bottom tabs.
	m_tabs = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_BOTTOM );

	m_editorPage  = new wxPanel( m_tabs );
	m_consolePage = new wxPanel( m_tabs );

	if( !BuildEditorPage( m_editorPage ) )
	{
		common->Error( "MEMainFrame: failed to build editor page" );
	}
	if( !BuildConsolePage( m_consolePage ) )
	{
		common->Error( "MEMainFrame: failed to build console page" );
	}

	m_tabs->AddPage( m_editorPage, "Editor", true );
	m_tabs->AddPage( m_consolePage, "Console", false );

	// Layout: notebook fills the frame's client area, toolbar/statusbar take
	// their space via the frame's sizer.
	auto* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_tabs, 1, wxEXPAND );
	SetSizer( sizer );

	// Restore splitter positions from options.
	const int editHeight = options.GetMaterialEditHeight();
	const int treeWidth = options.GetMaterialTreeWidth();
	const int stageWidth = options.GetStageWidth();
	const int previewWidth = options.GetPreviewPropertiesWidth();

	m_mainSplitter->SetSashPosition( editHeight > 0 ? editHeight : 300 );
	m_editSplitter->SetSashPosition( treeWidth > 0 ? treeWidth : 300 );
	if( m_materialEditSplitter )
	{
		m_materialEditSplitter->SetSashPosition( stageWidth > 0 ? stageWidth : 200 );
	}
	m_previewSplitter->SetSashPosition( previewWidth > 0 ? previewWidth : 300 );

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

	// Preview reloads
	Bind( wxEVT_MENU, &MEMainFrame::OnReloadShaders, this, ID_PREVIEW_RELOADSHADERS );
	Bind( wxEVT_MENU, &MEMainFrame::OnReloadImages, this, ID_PREVIEW_RELOADIMAGES );

	// Register with document manager
	RegisterViews();
}

/*
================
MEMainFrame::~MEMainFrame
================
*/
MEMainFrame::~MEMainFrame()
{
	if( m_find )
	{
		m_find->Destroy();
		m_find = NULL;
	}
	delete m_toolbarIcons;
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

	wxMenu* previewMenu = new wxMenu();
	previewMenu->Append( ID_ME_PREVIEW_APPLYCHANGES, "Apply Material\tCtrl-A" );
	previewMenu->Append( ID_ME_PREVIEW_APPLYFILE, "Apply File" );
	previewMenu->Append( ID_ME_PREVIEW_APPLYALL, "Apply All" );
	previewMenu->AppendSeparator();
	previewMenu->Append( ID_PREVIEW_RELOADSHADERS, "&Reload Shaders\tCtrl-R" );
	previewMenu->Append( ID_PREVIEW_RELOADIMAGES, "Reload Images\tCtrl-I" );

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append( fileMenu, "&File" );
	menuBar->Append( editMenu, "&Edit" );
	menuBar->Append( viewMenu, "&Material View" );
	menuBar->Append( previewMenu, "&Preview" );

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

bool MEMainFrame::BuildEditorPage( wxWindow* parent )
{
	// editor splitter.
	m_mainSplitter = new wxSplitterWindow( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH | wxSP_BORDER );
	m_mainSplitter->SetMinimumPaneSize( 80 );

	// tree + edit splitter.
	m_editSplitter = new wxSplitterWindow( m_mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH | wxSP_BORDER );
	m_editSplitter->SetMinimumPaneSize( 80 );

	m_materialTreeView = new MaterialTreeView( m_editSplitter );
	m_materialEditView = new MaterialEditView( m_editSplitter );

	m_editSplitter->SplitVertically( m_materialTreeView, m_materialEditView );

	// preview props + preview splitter.
	m_previewSplitter = new wxSplitterWindow( m_mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH | wxSP_BORDER );
	m_previewSplitter->SetMinimumPaneSize( 80 );

	m_previewPropertyView = new MaterialPreviewPropView( m_previewSplitter );
	m_materialPreviewView = new MaterialPreviewView( m_previewSplitter );

	m_previewSplitter->SplitVertically( m_previewPropertyView, m_materialPreviewView );

	m_mainSplitter->SplitHorizontally( m_editSplitter, m_previewSplitter );

	if( m_materialEditView )
	{
		m_stageView = m_materialEditView->GetStageView();
		m_materialPropertyView  = m_materialEditView->GetMaterialPropTreeView();
		m_materialEditSplitter  = m_materialEditView->GetEditSplitter();

		if( m_stageView && m_materialPropertyView )
		{
			m_stageView->SetMaterialPropertyView( m_materialPropertyView );
		}
	}

	// preview view.
	if( m_previewPropertyView && m_materialPreviewView )
	{
		m_previewPropertyView->RegisterPreviewView( m_materialPreviewView );
	}

	// Initialise property trees.
	if( m_previewPropertyView )
	{
		m_previewPropertyView->InitializePropTree();
	}

	if( m_materialPropertyView )
	{
		m_materialPropertyView->LoadSettings();
	}

	// Column widths.
	if( m_materialPropertyView )
	{
		m_materialPropertyView->SetColumn( options.GetMaterialPropHeadingWidth() > 0 ? options.GetMaterialPropHeadingWidth() : 200 );
	}

	if( m_previewPropertyView )
	{
		m_previewPropertyView->SetColumn( options.GetPreviewPropHeadingWidth() > 0 ? options.GetPreviewPropHeadingWidth() : 120 );
	}

	// Layout the page.
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_mainSplitter, 1, wxEXPAND );
	parent->SetSizer( sizer );

	return true;
}

/*
================
MEMainFrame::BuildConsolePage
================
*/
bool MEMainFrame::BuildConsolePage( wxWindow* parent )
{
	m_consoleView = new ConsoleWidget( parent );
	m_consoleView->SetQuitHandler( [this]()
	{
		this->Close( true );
	} );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_consoleView, 1, wxEXPAND );
	parent->SetSizer( sizer );

	return true;
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
MEMainFrame::OnClose
================
*/
void MEMainFrame::OnClose( wxCloseEvent& event )
{
	// Persist options.
	options.SetMaterialEditHeight( m_mainSplitter->GetSashPosition() );
	options.SetMaterialTreeWidth( m_editSplitter->GetSashPosition() );
	if( m_materialEditSplitter )
	{
		options.SetStageWidth( m_materialEditSplitter->GetSashPosition() );
	}
	options.SetPreviewPropertiesWidth( m_previewSplitter->GetSashPosition() );

	if( m_materialPropertyView )
	{
		options.SetMaterialPropHeadingWidth( m_materialPropertyView->GetColumn() );
	}
	if( m_previewPropertyView )
	{
		options.SetPreviewPropHeadingWidth( m_previewPropertyView->GetColumn() );
	}

	options.SetWindowPlacement( "mainframe", this );
	options.Save();

	if( m_materialPropertyView )
	{
		m_materialPropertyView->SaveSettings();
	}

	// Detach views from the doc manager before tearing down.
	materialDocManager.UnRegisterAllMaterialViews();

	// Free the material property definition lists.
	MaterialDefManager::DestroyMaterialDefLists();

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
void MEMainFrame::MV_OnMaterialSelectionChange( MaterialDoc* /*pMaterial*/ )
{
}