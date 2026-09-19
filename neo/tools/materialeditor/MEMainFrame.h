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

#ifndef __MEMAINFRAME_H__
#define __MEMAINFRAME_H__

#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/statusbr.h>
#include <wx/toolbar.h>
#include <wx/imaglist.h>

#include "MaterialEditor.h"
#include "MaterialView.h"
#include "MaterialTreeView.h"
#include "MEOptions.h"
#include "MaterialDocManager.h"

class MaterialTreeView;
class StageView;
class MaterialPropTreeView;
class MaterialPreviewView;
class MaterialPreviewPropView;
class ConsoleWidget;
class MaterialEditView;
class FindDialog;

class MEMainFrame : public wxFrame, public MaterialView
{
public:
	MEMainFrame();
	virtual ~MEMainFrame();

public: // MaterialView interface
	virtual void MV_OnMaterialSelectionChange( MaterialDoc* pMaterial ) override;

public: // MEMainFrame interface
	void PrintConsoleMessage( const char* msg );
	void CloseFind();
	void FindNext( MaterialSearchData_t* search );

	MaterialTreeView*       GetMaterialTreeView();
	MaterialEditView*       GetMaterialEditView();
	StageView*              GetStageView();
	MaterialPropTreeView*   GetMaterialPropTreeView();
	MaterialPreviewView*    GetMaterialPreviewView();
	MaterialPreviewPropView* GetMaterialPreviewPropView();
	MaterialDocManager&     GetMaterialDocManager();
	MEOptions&              GetOptions();

private:
	void					BuildMenuBar();
	void					BuildToolBar();
	void					BuildStatusBar();
	bool					BuildEditorPage( wxWindow* parent );
	bool					BuildConsolePage( wxWindow* parent );
	void					RegisterViews();

	void					OnFileExit( wxCommandEvent& event );
	void					OnFileSaveMaterial( wxCommandEvent& event );
	void					OnFileSaveFile( wxCommandEvent& event );
	void					OnFileSaveAll( wxCommandEvent& event );

	void					OnApplyMaterial( wxCommandEvent& event );
	void					OnApplyFile( wxCommandEvent& event );
	void					OnApplyAll( wxCommandEvent& event );

	void					OnEditCut( wxCommandEvent& event );
	void					OnEditCopy( wxCommandEvent& event );
	void					OnEditPaste( wxCommandEvent& event );
	void					OnEditDelete( wxCommandEvent& event );
	void					OnEditRename( wxCommandEvent& event );
	void					OnEditFind( wxCommandEvent& event );
	void					OnEditFindNext( wxCommandEvent& event );
	void					OnEditUndo( wxCommandEvent& event );
	void					OnEditRedo( wxCommandEvent& event );

	void					OnViewIncludeFile( wxCommandEvent& event );
	void					OnReloadShaders( wxCommandEvent& event );
	void					OnReloadImages( wxCommandEvent& event );

	void					OnClose( wxCloseEvent& event );

	void					OnFileOpenMaterial( wxCommandEvent& event );
	void					OnFileShowAllMaterials( wxCommandEvent& event );
	void					OnFileSaveMaterialUpdate( wxUpdateUIEvent& event );
	void					OnFileSaveFileUpdate( wxUpdateUIEvent& event );
	void					OnFileSaveAllUpdate( wxUpdateUIEvent& event );

	void					OnApplyMaterialUpdate( wxUpdateUIEvent& event );
	void					OnApplyFileUpdate( wxUpdateUIEvent& event );
	void					OnApplyAllUpdate( wxUpdateUIEvent& event );

	void					OnEditUndoUpdate( wxUpdateUIEvent& event );
	void					OnEditRedoUpdate( wxUpdateUIEvent& event );
	void					OnEditCutUpdate( wxUpdateUIEvent& event );
	void					OnEditCopyUpdate( wxUpdateUIEvent& event );
	void					OnEditPasteUpdate( wxUpdateUIEvent& event );
	void					OnEditDeleteUpdate( wxUpdateUIEvent& event );
	void					OnEditRenameUpdate( wxUpdateUIEvent& event );

private:
	wxNotebook*              m_tabs;
	wxPanel*                 m_editorPage;
	wxPanel*                 m_consolePage;

	wxSplitterWindow*        m_mainSplitter;      // editor / preview
	wxSplitterWindow*        m_editSplitter;      // tree / edit
	wxSplitterWindow*        m_previewSplitter;   // props / preview

	MaterialTreeView*        m_materialTreeView;
	MaterialEditView*        m_materialEditView;
	MaterialPreviewPropView* m_previewPropertyView;
	MaterialPreviewView*     m_materialPreviewView;
	ConsoleWidget*			 m_consoleView;

	StageView*               m_stageView;
	MaterialPropTreeView*    m_materialPropertyView;
	wxSplitterWindow*        m_materialEditSplitter;

	FindDialog*              m_find;
	MaterialSearchData_t     searchData;

	MaterialDocManager       materialDocManager;

	MEOptions                options;

	wxImageList*             m_toolbarIcons;
};

ID_INLINE MaterialTreeView*       MEMainFrame::GetMaterialTreeView()
{
	return m_materialTreeView;
}

ID_INLINE MaterialEditView*       MEMainFrame::GetMaterialEditView()
{
	return m_materialEditView;
}

ID_INLINE StageView*              MEMainFrame::GetStageView()
{
	return m_stageView;
}

ID_INLINE MaterialPropTreeView*   MEMainFrame::GetMaterialPropTreeView()
{
	return m_materialPropertyView;
}

ID_INLINE MaterialPreviewView*    MEMainFrame::GetMaterialPreviewView()
{
	return m_materialPreviewView;
}

ID_INLINE MaterialPreviewPropView* MEMainFrame::GetMaterialPreviewPropView()
{
	return m_previewPropertyView;
}

ID_INLINE MaterialDocManager&     MEMainFrame::GetMaterialDocManager()
{
	return materialDocManager;
}

ID_INLINE MEOptions&              MEMainFrame::GetOptions()
{
	return options;
}

#endif /* !__MEMAINFRAME_H__ */