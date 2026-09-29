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

#ifndef __GENAVIGATOR_H__
#define __GENAVIGATOR_H__

class rvGEWorkspace;
class idWindow;

class rvGENavItemData : public wxTreeItemData
{
public:
	explicit rvGENavItemData( idWindow* w ) : m_window( w ) {}
	idWindow* GetWindow() const
	{
		return m_window;
	}

private:
	idWindow* m_window;
};

class rvGENavigator : public wxPanel
{
public:
	explicit rvGENavigator( wxWindow* parent );
	~rvGENavigator() override;

	void Refresh();
	void SetWorkspace( rvGEWorkspace* workspace );
	void Update();
	void UpdateSelections();

private:
	void OnSelChanged( wxTreeEvent& e );
	void OnItemActivated( wxTreeEvent& e );
	void OnItemExpanded( wxTreeEvent& e );
	void OnItemCollapsed( wxTreeEvent& e );
	void OnItemMenu( wxTreeEvent& e );
	void OnContextHide( wxCommandEvent& e );
	void OnContextUnhide( wxCommandEvent& e );
	void OnContextProperties( wxCommandEvent& e );
	void OnContextScripts( wxCommandEvent& e );

	void AddWindow( idWindow* window, wxTreeItemId parent );

	enum IconIndex
	{
		ICON_NORMAL = 0,
		ICON_SCRIPTS,
		ICON_HIDDEN,
		ICON_BOTH, // For both cases
		ICON_COUNT
	};

	wxTreeCtrl* m_tree;
	wxImageList* m_icons;
	wxTreeItemId m_root;
	rvGEWorkspace* m_workspace;
	bool m_updatingSelection;

	wxTreeItemId m_contextItem;
};

#endif /* !__GENAVIGATOR_H__ */