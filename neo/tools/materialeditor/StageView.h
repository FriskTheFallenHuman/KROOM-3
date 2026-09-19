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

#ifndef __STAGEVIEW_H__
#define __STAGEVIEW_H__

#include <wx/wx.h>
#include <wx/menu.h>

#include "MaterialEditor.h"
#include "ToggleListView.h"
#include "MaterialView.h"
#include "MaterialDocManager.h"

class MaterialPropTreeView;

/**
* View that handles managing the material stages.
 */
class StageView : public ToggleListView, public MaterialView
{

public:
	explicit StageView( wxWindow* parent );
	virtual ~StageView();

	void SetMaterialPropertyView( MaterialPropTreeView* propView )
	{
		m_propView = propView;
	}

	bool CanCopy();
	bool CanPaste();
	bool CanCut();
	bool CanDelete();
	bool CanRename();

	void RefreshStageList();

	void OnRenameStage();
	void OnDeleteStage();
	void OnDeleteAllStages();
	void OnAddStage();
	void OnAddBumpmapStage();
	void OnAddDiffuseStage();
	void OnAddSpecularStage();
	void OnCopy();
	void OnPaste();

protected: // interfaces
	// MaterialView interface.
	virtual void MV_OnMaterialSelectionChange( MaterialDoc* pMaterial ) override;
	virtual void MV_OnMaterialStageAdd( MaterialDoc* pMaterial, int stageNum ) override;
	virtual void MV_OnMaterialStageDelete( MaterialDoc* pMaterial, int stageNum ) override;
	virtual void MV_OnMaterialStageMove( MaterialDoc* pMaterial, int from, int to ) override;
	virtual void MV_OnMaterialAttributeChanged( MaterialDoc* pMaterial, int stage, const char* attribName ) override;
	virtual void MV_OnMaterialSaved( MaterialDoc* pMaterial ) override;

	// ToggleListView interface.
	virtual void OnStateChanged( int index, int toggleState ) override;
	virtual void OnRowLeftDown( int index, const wxPoint& pt ) override;

protected:
	void OnSelectionChanged( wxCommandEvent& event );
	void OnItemRightClick( wxMouseEvent& event );
	void OnMouseMotion( wxMouseEvent& event );
	void OnLeftUp( wxMouseEvent& event );
	void OnCaptureLost( wxMouseCaptureLostEvent& event );
	void OnKeyDown( wxKeyEvent& event );

	void ShowContextMenu( const wxPoint& pt );
	void DropItemOnList();

private:
	MaterialPropTreeView*   m_propView;
	MaterialDoc* currentMaterial;

	int m_dragIndex;
	int m_dropIndex;
	bool m_dragging;
	wxPoint m_dragStart;

	bool internalChange;
};

#endif /* !__STAGEVIEW_H__ */