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

#include "StageView.h"
#include "MaterialPropTreeView.h"
#include "MaterialDef.h"
#include "MaterialDoc.h"

#include <wx/msgdlg.h>

/*
================
StageView::StageView
================
*/
StageView::StageView( wxWindow* parent )
	: ToggleListView( parent )
	, m_propView( NULL )
	, currentMaterial( NULL )
	, m_dragIndex( -1 )
	, m_dropIndex( -1 )
	, m_dragging( false )
	, internalChange( false )
{
	SetToggleIcons( "editors/gfx/me_disabled_icon.ico", "editors/gfx/me_on_icon.ico", "editors/gfx/me_off_icon.ico" );

	Bind( wxEVT_LISTBOX, &StageView::OnSelectionChanged, this );
	Bind( wxEVT_RIGHT_DOWN, &StageView::OnItemRightClick, this );
	Bind( wxEVT_MOTION, &StageView::OnMouseMotion, this );
	Bind( wxEVT_LEFT_UP, &StageView::OnLeftUp, this );
	Bind( wxEVT_MOUSE_CAPTURE_LOST, &StageView::OnCaptureLost, this );
	Bind( wxEVT_KEY_DOWN, &StageView::OnKeyDown, this );

	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnRenameStage();
	}, ID_STAGEPOPUP_RENAMESTAGE );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnAddStage();
	}, ID_STAGEPOPUP_ADDSTAGE );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnAddBumpmapStage();
	}, ID_STAGEPOPUP_ADDBUMPMAP );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnAddDiffuseStage();
	}, ID_STAGEPOPUP_ADDDIFFUSEMAP );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnAddSpecularStage();
	}, ID_STAGEPOPUP_ADDSPECULAR );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnDeleteStage();
	}, ID_STAGEPOPUP_DELETESTAGE );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnDeleteAllStages();
	}, ID_STAGEPOPUP_DELETEALLSTAGES );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnCopy();
	}, ID_STAGEPOPUP_COPY );
	Bind( wxEVT_MENU, [this]( wxCommandEvent& )
	{
		OnPaste();
	}, ID_STAGEPOPUP_PASTE );
}

/*
================
StageView::~StageView
================
*/
StageView::~StageView()
{
}

/*
================
StageView::RefreshStageList
================
*/
void StageView::RefreshStageList()
{
	int selectedItem = GetSelection();

	DeleteAllItems();

	if( currentMaterial )
	{
		// Row 0: the top-level Material entry.  Disabled toggle.
		AddItem( "Material", TOGGLE_STATE_DISABLED );

		const int stageCount = currentMaterial->GetStageCount();
		for( int i = 0; i < stageCount; i++ )
		{
			const char* name = currentMaterial->GetAttribute( i, "name" );
			AddItem( name ? name : "Stage",
					 currentMaterial->IsStageEnabled( i )
					 ? TOGGLE_STATE_ON
					 : TOGGLE_STATE_OFF );
		}

		SetSelection( selectedItem >= 0 ? selectedItem : 0 );

		// Refresh the property grid for the newly selected row.
		wxCommandEvent dummy;
		OnSelectionChanged( dummy );
	}
}

/*
================
StageView::OnSelectionChanged
================
*/
void StageView::OnSelectionChanged( wxCommandEvent& /*event*/ )
{
	if( !m_propView )
	{
		return;
	}

	const int sel = GetSelection();
	if( sel == wxNOT_FOUND )
	{
		m_propView->SetPropertyListType( -1 );
		return;
	}

	int type = -1;

	if( sel == 0 )
	{
		type = MaterialDefManager::MATERIAL_DEF_MATERIAL;
	}
	else if( currentMaterial )
	{
		const int stageType = currentMaterial->GetAttributeInt( sel - 1, "stagetype" );
		switch( stageType )
		{
			case MaterialDoc::STAGE_TYPE_NORMAL:
				type = MaterialDefManager::MATERIAL_DEF_STAGE;
				break;
			case MaterialDoc::STAGE_TYPE_SPECIALMAP:
				type = MaterialDefManager::MATERIAL_DEF_SPECIAL_STAGE;
				break;
		}
	}

	m_propView->SetPropertyListType( type, sel - 1 );
}

/*
================
StageView::OnStateChanged
================
*/
void StageView::OnStateChanged( int index, int toggleState )
{
	if( !currentMaterial || index <= 0 )
	{
		return;
	}

	if( toggleState == TOGGLE_STATE_ON )
	{
		currentMaterial->EnableStage( index - 1, true );
	}
	else if( toggleState == TOGGLE_STATE_OFF )
	{
		currentMaterial->EnableStage( index - 1, false );
	}
}

/*
================
StageView::OnRowLeftDown
================
*/
void StageView::OnRowLeftDown( int index, const wxPoint& pt )
{
	if( index <= 0 )
	{
		return;  // Material row is not draggable.
	}

	m_dragIndex = index;
	m_dropIndex = -1;
	m_dragging  = false;
	m_dragStart = pt;

	CaptureMouse();
}

/*
================
StageView::OnMouseMotion
================
*/
void StageView::OnMouseMotion( wxMouseEvent& event )
{
	if( HasCapture() && m_dragIndex > 0 )
	{
		if( !m_dragging && ( abs( event.GetY() - m_dragStart.y ) > 4 || abs( event.GetX() - m_dragStart.x ) > 4 ) )
		{
			m_dragging = true;
		}

		if( m_dragging )
		{
			int hover = VirtualHitTest( event.GetY() );
			if( hover != m_dropIndex )
			{
				m_dropIndex = hover;

				// Refresh the whole list to clear any old drop marker.
				RefreshAll();
			}
		}
	}

	event.Skip();
}


/*
================
StageView::OnLeftUp
================
*/
void StageView::OnLeftUp( wxMouseEvent& event )
{
	if( HasCapture() )
	{
		ReleaseMouse();
	}

	if( m_dragging && m_dragIndex > 0 && m_dropIndex > 0 &&
			m_dropIndex != m_dragIndex )
	{
		DropItemOnList();
	}

	m_dragging  = false;
	m_dragIndex = -1;
	m_dropIndex = -1;

	event.Skip();
}

/*
================
StageView::OnCaptureLost
================
*/
void StageView::OnCaptureLost( wxMouseCaptureLostEvent& /*event*/ )
{
	m_dragging  = false;
	m_dragIndex = -1;
	m_dropIndex = -1;
}

/*
================
StageView::DropItemOnList
================
*/
void StageView::DropItemOnList()
{
	if( !currentMaterial )
	{
		return;
	}

	const int from = m_dragIndex - 1;
	const int to   = m_dropIndex - 1;

	if( from < 0 || to < 0 )
	{
		return;
	}

	internalChange = true;
	currentMaterial->MoveStage( from, to );
	internalChange = false;
}

/*
================
StageView::OnItemRightClick
================
*/
void StageView::OnItemRightClick( wxMouseEvent& event )
{
	if( !materialDocManager->GetCurrentMaterialDoc() )
	{
		return;
	}

	const wxPoint pt = event.GetPosition();

	// Select the row under the cursor, if any.
	const int index = VirtualHitTest( pt.y );
	if( index != wxNOT_FOUND )
	{
		SetSelection( index );
	}

	ShowContextMenu( pt );
}

/*
================
StageView::ShowContextMenu
================
*/
void StageView::ShowContextMenu( const wxPoint& pt )
{
	wxMenu menu;

	const int sel = GetSelection();
	const bool haveStageSelected = ( sel > 0 && currentMaterial != NULL );

	// Rename
	{
		wxMenuItem* mi = menu.Append( ID_STAGEPOPUP_RENAMESTAGE, "Rename" );
		bool canRename = false;
		if( haveStageSelected )
		{
			canRename = currentMaterial->GetAttributeInt( sel - 1, "stagetype" ) ==
						MaterialDoc::STAGE_TYPE_NORMAL;
		}
		mi->Enable( canRename );
	}

	menu.Append( ID_STAGEPOPUP_ADDSTAGE, "Add Stage" );

	{
		wxMenuItem* mi = menu.Append( ID_STAGEPOPUP_ADDBUMPMAP, "Add Bumpmap Stage" );
		if( currentMaterial )
		{
			mi->Enable( currentMaterial->FindStage(
							MaterialDoc::STAGE_TYPE_SPECIALMAP, "bumpmap" ) < 0 );
		}
		else
		{
			mi->Enable( false );
		}
	}
	{
		wxMenuItem* mi = menu.Append( ID_STAGEPOPUP_ADDDIFFUSEMAP, "Add Diffuse Stage" );
		if( currentMaterial )
		{
			mi->Enable( currentMaterial->FindStage(
							MaterialDoc::STAGE_TYPE_SPECIALMAP, "diffusemap" ) < 0 );
		}
		else
		{
			mi->Enable( false );
		}
	}
	{
		wxMenuItem* mi = menu.Append( ID_STAGEPOPUP_ADDSPECULAR, "Add Specular Stage" );
		if( currentMaterial )
		{
			mi->Enable( currentMaterial->FindStage(
							MaterialDoc::STAGE_TYPE_SPECIALMAP, "specularmap" ) < 0 );
		}
		else
		{
			mi->Enable( false );
		}
	}

	menu.AppendSeparator();

	{
		wxMenuItem* mi = menu.Append( ID_STAGEPOPUP_COPY, "Copy" );
		mi->Enable( haveStageSelected );
	}
	{
		wxMenuItem* mi = menu.Append( ID_STAGEPOPUP_PASTE, "Paste" );
		mi->Enable( materialDocManager->IsCopyStage() );
	}

	menu.AppendSeparator();

	{
		wxMenuItem* mi = menu.Append( ID_STAGEPOPUP_DELETESTAGE, "Delete" );
		mi->Enable( haveStageSelected );
	}
	{
		wxMenuItem* mi = menu.Append( ID_STAGEPOPUP_DELETEALLSTAGES, "Delete All Stages" );
		mi->Enable( currentMaterial && currentMaterial->GetStageCount() > 0 );
	}

	PopupMenu( &menu, pt );
}

/*
================
StageView::OnRenameStage
================
*/
void StageView::OnRenameStage()
{
	const int sel = GetSelection();
	if( sel > 0 )
	{
		const wxString currentName = GetItemText( sel );
		wxTextEntryDialog dlg( this, "Stage name:", "Rename Stage", currentName );
		if( dlg.ShowModal() == wxID_OK )
		{
			idStr newName = dlg.GetValue().c_str();
			newName.ToLower();
			if( newName.Length() > 0 && currentMaterial )
			{
				internalChange = true;
				currentMaterial->SetAttribute( sel - 1, "name", newName.c_str() );
				internalChange = false;

				SetItemText( sel, newName.c_str() );
			}
		}
	}
}

/*
================
StageView::OnDeleteStage
================
*/
void StageView::OnDeleteStage()
{
	const int sel = GetSelection();
	if( sel > 0 )
	{
		int result = wxMessageBox( "Are you sure you want to delete this stage?",
								   "Delete?", wxICON_QUESTION | wxYES_NO, this );
		if( result == wxYES )
		{
			MaterialDoc* material = materialDocManager->GetCurrentMaterialDoc();
			if( material )
			{
				material->RemoveStage( sel - 1 );
			}
		}
	}
}

/*
================
StageView::OnDeleteAllStages
================
*/
void StageView::OnDeleteAllStages()
{
	int result = wxMessageBox( "Are you sure you want to delete all stages?",
							   "Delete?", wxICON_QUESTION | wxYES_NO, this );
	if( result == wxYES )
	{
		MaterialDoc* material = materialDocManager->GetCurrentMaterialDoc();
		if( material )
		{
			material->ClearStages();
		}
	}
}

/*
================
StageView::OnAddStage
================
*/
void StageView::OnAddStage()
{
	MaterialDoc* material = materialDocManager->GetCurrentMaterialDoc();
	if( !material )
	{
		return;
	}

	idStr name = va( "Stage %d", material->GetStageCount() + 1 );
	material->AddStage( MaterialDoc::STAGE_TYPE_NORMAL, name.c_str() );
}

/*
================
StageView::OnAddBumpmapStage
================
*/
void StageView::OnAddBumpmapStage()
{
	MaterialDoc* material = materialDocManager->GetCurrentMaterialDoc();
	if( material )
	{
		material->AddStage( MaterialDoc::STAGE_TYPE_SPECIALMAP, "bumpmap" );
	}
}

/*
================
StageView::OnAddDiffuseStage
================
*/
void StageView::OnAddDiffuseStage()
{
	MaterialDoc* material = materialDocManager->GetCurrentMaterialDoc();
	if( material )
	{
		material->AddStage( MaterialDoc::STAGE_TYPE_SPECIALMAP, "diffusemap" );
	}
}

/*
================
StageView::OnAddSpecularStage
================
*/
void StageView::OnAddSpecularStage()
{
	MaterialDoc* material = materialDocManager->GetCurrentMaterialDoc();
	if( material )
	{
		material->AddStage( MaterialDoc::STAGE_TYPE_SPECIALMAP, "specularmap" );
	}
}

/*
================
StageView::OnCopy
================
*/
void StageView::OnCopy()
{
	const int sel = GetSelection();
	if( sel > 0 && currentMaterial )
	{
		materialDocManager->CopyStage( currentMaterial, sel - 1 );
	}
}

/*
================
StageView::OnPaste
================
*/
void StageView::OnPaste()
{
	if( !materialDocManager->IsCopyStage() )
	{
		return;
	}

	MaterialDoc* material = materialDocManager->GetCurrentMaterialDoc();
	if( !material )
	{
		return;
	}

	int type;
	idStr name;
	materialDocManager->GetCopyStageInfo( type, name );

	const int existingIndex = material->FindStage( type, name );

	if( type != MaterialDoc::STAGE_TYPE_SPECIALMAP || existingIndex == -1 )
	{
		materialDocManager->PasteStage( material );
	}
	else
	{
		int result = wxMessageBox( wxString::Format( "Do you want to replace the '%s' stage?", name.c_str() ), "Replace?", wxICON_QUESTION | wxYES_NO, this );
		if( result == wxYES )
		{
			material->RemoveStage( existingIndex );
			materialDocManager->PasteStage( material );
		}
	}
}

/*
================
StageView::OnKeyDown
================
*/
void StageView::OnKeyDown( wxKeyEvent& event )
{
	if( event.GetKeyCode() == WXK_DELETE )
	{
		OnDeleteStage();
		return;
	}

	if( event.GetKeyCode() == WXK_F2 )
	{
		OnRenameStage();
		return;
	}

	event.Skip();
}

/*
================
StageView::MV_OnMaterialSelectionChange
================
*/
void StageView::MV_OnMaterialSelectionChange( MaterialDoc* pMaterial )
{
	currentMaterial = pMaterial;
	RefreshStageList();
}

/*
================
StageView::MV_OnMaterialStageAdd
================
*/
void StageView::MV_OnMaterialStageAdd( MaterialDoc* pMaterial, int stageNum )
{
	if( pMaterial != currentMaterial )
	{
		return;
	}

	const idStr name = pMaterial->GetAttribute( stageNum, "name" );
	InsertItem( stageNum + 1, name.c_str(), TOGGLE_STATE_ON );
	RefreshAll();
}

/*
================
StageView::MV_OnMaterialStageDelete
================
*/
void StageView::MV_OnMaterialStageDelete( MaterialDoc* pMaterial, int stageNum )
{
	if( pMaterial != currentMaterial )
	{
		return;
	}
	DeleteItem( stageNum + 1 );
}

/*
================
StageView::MV_OnMaterialStageMove
================
*/
void StageView::MV_OnMaterialStageMove( MaterialDoc* pMaterial, int from, int to )
{
	if( pMaterial != currentMaterial || internalChange )
	{
		return;
	}

	RefreshStageList();
}

/*
================
StageView::MV_OnMaterialAttributeChanged
================
*/
void StageView::MV_OnMaterialAttributeChanged( MaterialDoc* pMaterial, int stage, const char* attribName )
{
	if( !internalChange && pMaterial == currentMaterial && stage >= 0 && attribName && !strcmp( attribName, "name" ) )
	{
		const char* name = pMaterial->GetAttribute( stage, attribName );
		SetItemText( stage + 1, name ? name : "Stage" );
	}
}

/*
================
StageView::MV_OnMaterialSaved
================
*/
void StageView::MV_OnMaterialSaved( MaterialDoc* pMaterial )
{
	// Saving a material re-enables all of its stages.
	if( pMaterial == currentMaterial )
	{
		for( int i = 1; i < GetItemCount(); i++ )
		{
			SetToggleState( i, TOGGLE_STATE_ON );
		}
	}
}

/*
================
StageView::CanCopy
================
*/
bool StageView::CanCopy()
{
	return GetSelection() > 0;
}

/*
================
StageView::CanPaste
================
*/
bool StageView::CanPaste()
{
	return materialDocManager->IsCopyStage();
}

/*
================
StageView::CanCut
================
*/
bool StageView::CanCut()
{
	// Cut is not supported for stages.
	return false;
}

/*
================
StageView::CanDelete
================
*/
bool StageView::CanDelete()
{
	return GetSelection() > 0;
}

/*
================
StageView::CanRename
================
*/
bool StageView::CanRename()
{
	const int sel = GetSelection();
	if( sel <= 0 || !currentMaterial )
	{
		return false;
	}

	return currentMaterial->GetAttributeInt( sel - 1, "stagetype" ) == MaterialDoc::STAGE_TYPE_NORMAL;
}