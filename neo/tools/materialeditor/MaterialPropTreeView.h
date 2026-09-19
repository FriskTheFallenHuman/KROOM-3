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

#ifndef __MATERIALPROPTREEVIEW_H__
#define __MATERIALPROPTREEVIEW_H__

#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>

#include <map>
#include <string>

#include "MaterialEditor.h"
#include "MaterialView.h"
#include "MaterialDocManager.h"
#include "MaterialDef.h"
#include "../common/RegistryOptions.h"

/**
* View that displays material and stage properties and allows the user to edit the properties.
*/
class MaterialPropTreeView : public wxPanel, public MaterialView
{

public:
	explicit			MaterialPropTreeView( wxWindow* parent );
	virtual				~MaterialPropTreeView();

	void				SetPropertyListType( int listType, int stageNum = -1 );

	void				LoadSettings();
	void				SaveSettings();

	void				SetColumn( int width );
	int					GetColumn() const;

	// MaterialView interface.
	virtual void		MV_OnMaterialChange( MaterialDoc* pMaterial ) override;

protected:
	void				OnPropertyGridChanged( wxPropertyGridEvent& event );
	void				OnPropertyGridItemCollapsed( wxPropertyGridEvent& event );
	void				OnPropertyGridItemExpanded( wxPropertyGridEvent& event );

	void				RefreshProperties();

private:
	wxPropertyGrid*		m_grid;
	MaterialDefList*	currentPropDefs;
	int					currentListType;
	int					currentStage;
	bool				internalChange;
	rvRegistryOptions	registry;
	std::map<std::string, wxPGProperty*> m_propertyMap;
};

#endif /* !__MATERIALPROPTREEVIEW_H__ */