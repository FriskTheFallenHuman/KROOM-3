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

#ifndef __MATERIALPREVIEWPROPVIEW_H__
#define __MATERIALPREVIEWPROPVIEW_H__

#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

#include <vector>

#include "MaterialEditor.h"
#include "MaterialView.h"
#include "MaterialDocManager.h"

class MaterialPreviewView;

class MaterialPreviewPropView : public wxPanel, public MaterialView
{
public:
	explicit MaterialPreviewPropView( wxWindow* parent );
	virtual ~MaterialPreviewPropView();

	void RegisterPreviewView( MaterialPreviewView* view );
	void InitializePropTree();

	void SetColumn( int width );
	int  GetColumn() const;

protected:
	void OnPropertyGridChanged( wxPropertyGridEvent& event );
	void OnPropertyGridRightClick( wxPropertyGridEvent& event );
	void OnAddLight( wxCommandEvent& event );
	void OnRemoveLight( wxCommandEvent& event );
	void OnBrowseModel( wxCommandEvent& event );

private:
	void BuildShaderList();
	void AddLightCategory( int lightId, const idStr& shaderName, const idVec3& color, float radius, bool allowMove );
	void AddLightCategoryDefault( int lightId );

	void RebuildLightCategories();
	void OnRemoveLightById( int lightId );

	int  LightIdForProperty( wxPGProperty* prop ) const;

	struct LightValues
	{
		idStr   shaderName;
		idVec3  color;
		float   radius;
		bool    allowMove;
	};

	wxPropertyGrid*     m_grid;
	wxButton*           m_addLightBtn;
	wxButton*           m_removeLightBtn;
	wxButton*           m_browseModelBtn;
	MaterialPreviewView* m_preview;
	int                  m_numLights;
	int                  m_columnWidth;
	wxArrayString        m_lightShaderChoices;
	std::vector<idStr>   m_lightShaderNames;
};

#endif /* !__MATERIALPREVIEWPROPVIEW_H__ */