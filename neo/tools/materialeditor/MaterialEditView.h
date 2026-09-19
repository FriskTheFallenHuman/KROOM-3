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

#ifndef __MATERIALEDITVIEW_H__
#define __MATERIALEDITVIEW_H__

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/stc/stc.h>

#include "../common/SyntaxKeywords.h"

#include "MaterialEditor.h"
#include "MaterialView.h"
#include "MaterialDoc.h"

class StageView;
class MaterialPropTreeView;

/**
* View that contains the material edit controls. These controls include
* the stage view, the properties view and the source view.
*/
class MaterialEditView : public wxPanel, public MaterialView, public SourceModifyOwner
{
public:
	explicit MaterialEditView( wxWindow* parent );
	virtual ~MaterialEditView();

	StageView*              GetStageView();
	MaterialPropTreeView*   GetMaterialPropTreeView();
	wxSplitterWindow*       GetEditSplitter();

public: // SourceModifyOwner interface
	virtual idStr GetSourceText() override;

public: // MaterialView interface
	virtual void MV_OnMaterialSelectionChange( MaterialDoc* pMaterial ) override;
	virtual void MV_OnMaterialNameChanged( MaterialDoc* pMaterial, const char* oldName ) override;

protected:
	void OnTabChanged( wxBookCtrlEvent& event );
	void OnTextChanged( wxStyledTextEvent& event );
	void OnKeywordDwellStart( wxStyledTextEvent& event );
	void OnKeywordDwellEnd( wxStyledTextEvent& event );

	void GetMaterialSource();
	void ApplyMaterialSource();

	void SetupEditorStyles();
	void LoadKeywordsFromFile( const char* filename );

private:
	wxTextCtrl*             m_nameEdit;
	wxNotebook*             m_tabs;
	wxPanel*                m_propertiesPage;
	wxPanel*                m_textPage;
	wxSplitterWindow*       m_editSplitter;
	StageView*              m_stageView;
	MaterialPropTreeView*   m_materialPropertyView;
	wxStyledTextCtrl*       m_textView;
	SyntaxKeywords          m_keywords;

	bool                    m_sourceInit;
	bool                    m_sourceChanged;
	idStr                   m_currentMaterialName;
};

ID_INLINE StageView*              MaterialEditView::GetStageView()
{
	return m_stageView;
}

ID_INLINE MaterialPropTreeView*   MaterialEditView::GetMaterialPropTreeView()
{
	return m_materialPropertyView;
}

ID_INLINE wxSplitterWindow*       MaterialEditView::GetEditSplitter()
{
	return m_editSplitter;
}

#endif /* !__MATERIALEDITVIEW_H__ */