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

namespace
{
	constexpr int kIconW  = 16;
	constexpr int kIconH  = 16;
	constexpr int kBadgeW = 10;
	constexpr int kBadgeH = 10;

	wxImage EnsureAlpha( const wxBitmap& bmp )
	{
		wxImage img = bmp.ConvertToImage();
		if( !img.HasAlpha() )
		{
			img.InitAlpha();
			unsigned char* a = img.GetAlpha();
			const size_t n = (size_t)img.GetWidth() * (size_t)img.GetHeight();
			memset( a, 255, n );
		}
		return img;
	}

	wxBitmap NormalizeToIcon( const wxBitmap& bmp )
	{
		if( !bmp.IsOk() )
		{
			return wxBitmap();
		}

		if( bmp.GetWidth() == kIconW && bmp.GetHeight() == kIconH )
		{
			return bmp;
		}

		wxImage img = EnsureAlpha( bmp );
		img = img.Scale( kIconW, kIconH, wxIMAGE_QUALITY_HIGH );
		return wxBitmap( img, 32 );
	}

	wxBitmap CombineBadge( const wxBitmap& base, const wxBitmap& badge )
	{
		if( !base.IsOk() || !badge.IsOk() )
		{
			return wxBitmap();
		}

		wxImage ib = EnsureAlpha( base );
		wxImage ig = EnsureAlpha( badge );

		ig = ig.Scale( kBadgeW, kBadgeH, wxIMAGE_QUALITY_HIGH );

		wxImage out( kIconW, kIconH );
		out.InitAlpha();
		memset( out.GetData(), 0, (size_t)kIconW * kIconH * 3 );
		memset( out.GetAlpha(), 0, (size_t)kIconW * kIconH );
		out.Paste( ib, 0, 0 );
		out.Paste( ig, kIconW - kBadgeW, kIconH - kBadgeH );   // bottom-right

		return wxBitmap( out, 32 );
	}
}

/*
================
rvGENavigator::rvGENavigator
================
*/
rvGENavigator::rvGENavigator( wxWindow* parent )
	: wxPanel( parent, wxID_ANY )
	, m_tree( NULL )
	, m_icons( NULL )
	, m_workspace( NULL )
	, m_updatingSelection( false )
{
	const long style = wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT | wxTR_SINGLE;

	m_tree = new wxTreeCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, style );

	const wxBitmap bmpNormal = wxArtProvider::GetBitmap( wxART_NORMAL_FILE, wxART_LIST, wxSize( kIconW, kIconH ) );
	const wxBitmap bmpScripts = ImageUtils::BitmapFromRelativePath( "editors/gfx/guied_scripts.ico" );
	const wxBitmap bmpHidden  = ImageUtils::BitmapFromRelativePath( "editors/gfx/guied_nav_visible.ico" );

	m_icons = new wxImageList( kIconW, kIconH, true, ICON_COUNT );

	auto addIcon = []( wxImageList* list, const wxBitmap& bmp )
	{
		if( bmp.IsOk() )
		{
			list->Add( bmp );
			return;
		}

		// Keep indices aligned even if a load failed.
		wxImage blank( kIconW, kIconH );
		blank.InitAlpha();
		memset( blank.GetData(), 0, (size_t)kIconW * kIconH * 3 );
		memset( blank.GetAlpha(), 0, (size_t)kIconW * kIconH );
		list->Add( wxBitmap( blank, 32 ) );
	};

	const wxBitmap iconNormal  = NormalizeToIcon( bmpNormal );
	const wxBitmap iconScripts = NormalizeToIcon( bmpScripts );
	const wxBitmap iconHidden  = NormalizeToIcon( bmpHidden );

	addIcon( m_icons, iconNormal );                                     // ICON_NORMAL
	addIcon( m_icons, iconScripts );                                    // ICON_SCRIPTS
	addIcon( m_icons, iconHidden );                                     // ICON_HIDDEN
	addIcon( m_icons, CombineBadge( iconScripts, iconHidden ) );        // ICON_BOTH

	m_tree->AssignImageList( m_icons );

	m_root = m_tree->AddRoot( "Desktop" );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_tree, 1, wxEXPAND );
	SetSizer( sizer );

	m_tree->Bind( wxEVT_TREE_SEL_CHANGED, &rvGENavigator::OnSelChanged, this );
	m_tree->Bind( wxEVT_TREE_ITEM_ACTIVATED, &rvGENavigator::OnItemActivated, this );
	m_tree->Bind( wxEVT_TREE_ITEM_EXPANDED,  &rvGENavigator::OnItemExpanded,  this );
	m_tree->Bind( wxEVT_TREE_ITEM_COLLAPSED, &rvGENavigator::OnItemCollapsed, this );
	m_tree->Bind( wxEVT_TREE_ITEM_MENU, &rvGENavigator::OnItemMenu, this );
}

/*
================
rvGENavigator::~rvGENavigator
================
*/
rvGENavigator::~rvGENavigator() = default;

/*
================
rvGENavigator::SetWorkspace
================
*/
void rvGENavigator::SetWorkspace( rvGEWorkspace* workspace )
{
	m_workspace = workspace;
	Update();
}

/*
================
rvGENavigator::Refresh
================
*/
void rvGENavigator::Refresh()
{
	m_tree->Refresh();
}

/*
================
rvGENavigator::Update

Rebuilds the entire tree from the current workspace.
================
*/
void rvGENavigator::Update()
{
	m_updatingSelection = true;

	m_tree->DeleteChildren( m_root );

	if( m_workspace && m_workspace->GetInterface() && m_workspace->GetInterface()->GetDesktop() )
	{
		AddWindow( m_workspace->GetInterface()->GetDesktop(), m_root );
	}

	m_updatingSelection = false;
}

/*
================
rvGENavigator::AddWindow

Recursively inserts a window and its children into the tree.
================
*/
void rvGENavigator::AddWindow( idWindow* window, wxTreeItemId parent )
{
	rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( window );
	if( !wrapper || wrapper->IsDeleted() )
	{
		return;
	}

	const bool hasScript = wrapper->GetScriptDict().GetNumKeyVals() > 0 || wrapper->GetVariableDict().GetNumKeyVals() > 0;

	int icon = ICON_NORMAL;
	if( wrapper->IsHidden() && hasScript )
	{
		icon = ICON_BOTH;
	}
	else if( wrapper->IsHidden() )
	{
		icon = ICON_HIDDEN;
	}
	else if( hasScript )
	{
		icon = ICON_SCRIPTS;
	}

	wxString label = window->GetName();
	if( label.IsEmpty() )
	{
		label = "<unnamed>";
	}

	wxTreeItemId item = m_tree->AppendItem( parent, label, icon, icon, new rvGENavItemData( window ) );

	if( wrapper->IsSelected() )
	{
		m_tree->SelectItem( item );
		m_tree->EnsureVisible( item );
	}

	if( wrapper->GetChildCount() > 0 )
	{
		if( wrapper->IsExpanded() )
		{
			for( int i = 0; i < wrapper->GetChildCount(); ++i )
			{
				AddWindow( wrapper->GetChild( i ), item );
			}
			m_tree->Expand( item );
		}
		else
		{
			m_tree->AppendItem( item, "..." );
		}
	}
}

/*
================
rvGENavigator::UpdateSelections

Walks the tree and syncs the selection state from the wrappers.
================
*/
void rvGENavigator::UpdateSelections()
{
	if( !m_workspace )
	{
		return;
	}

	m_updatingSelection = true;

	std::function<void( wxTreeItemId )> walk = [&]( wxTreeItemId id )
	{
		wxTreeItemIdValue cookie;
		wxTreeItemId child = m_tree->GetFirstChild( id, cookie );
		while( child.IsOk() )
		{
			rvGENavItemData* data =	static_cast<rvGENavItemData*>( m_tree->GetItemData( child ) );
			if( data )
			{
				rvGEWindowWrapper* w = rvGEWindowWrapper::GetWrapper( data->GetWindow() );
				if( w )
				{
					m_tree->SelectItem( child, w->IsSelected() );
					if( w->IsSelected() )
					{
						m_tree->EnsureVisible( child );
					}
				}
			}
			walk( child );
			child = m_tree->GetNextChild( id, cookie );
		}
	};

	walk( m_root );

	m_updatingSelection = false;
}

/*
================
rvGENavigator::OnSelChanged
================
*/
void rvGENavigator::OnSelChanged( wxTreeEvent& e )
{
	if( m_updatingSelection || !m_workspace )
	{
		return;
	}

	rvGENavItemData* data =	static_cast<rvGENavItemData*>( m_tree->GetItemData( e.GetItem() ) );
	if( !data )
	{
		return;
	}

	idWindow* window = data->GetWindow();
	CallAfter( [this, window]()
	{
		if( m_workspace )
		{
			m_workspace->GetSelectionMgr().Set( window );
		}
	} );
}

/*
================
rvGENavigator::OnItemActivated

Double-click behaves exactly like the Properties context menu entry.
================
*/
void rvGENavigator::OnItemActivated( wxTreeEvent& )
{
	if( m_workspace )
	{
		m_workspace->EditSelectedProperties();
	}
}

/*
================
rvGENavigator::OnItemExpanded
================
*/
void rvGENavigator::OnItemExpanded( wxTreeEvent& e )
{
	rvGENavItemData* data =	static_cast<rvGENavItemData*>( m_tree->GetItemData( e.GetItem() ) );
	if( !data )
	{
		return;
	}

	rvGEWindowWrapper* w = rvGEWindowWrapper::GetWrapper( data->GetWindow() );
	if( w && w->CanHaveChildren() && w->GetChildCount() > 0 && !w->IsExpanded() )
	{
		w->Expand();

		m_updatingSelection = true;
		m_tree->DeleteChildren( e.GetItem() );
		for( int i = 0; i < w->GetChildCount(); ++i )
		{
			AddWindow( w->GetChild( i ), e.GetItem() );
		}
		m_updatingSelection = false;
	}
}

/*
================
rvGENavigator::OnItemCollapsed
================
*/
void rvGENavigator::OnItemCollapsed( wxTreeEvent& e )
{
	rvGENavItemData* data =	static_cast<rvGENavItemData*>( m_tree->GetItemData( e.GetItem() ) );
	if( !data )
	{
		return;
	}

	rvGEWindowWrapper* w = rvGEWindowWrapper::GetWrapper( data->GetWindow() );
	if( w && w->IsExpanded() )
	{
		w->Collapse();
	}
}

/*
================
rvGENavigator::OnItemMenu
================
*/
void rvGENavigator::OnItemMenu( wxTreeEvent& e )
{
	m_contextItem = e.GetItem();

	rvGENavItemData* data =	static_cast<rvGENavItemData*>( m_tree->GetItemData( m_contextItem ) );
	if( !data )
	{
		return;
	}

	rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( data->GetWindow() );
	if( !wrapper )
	{
		return;
	}

	const bool hidden = wrapper->IsHidden();

	wxMenu menu;
	if( hidden )
	{
		menu.Append( ID_GUIED_VIEW_UNHIDESELECTED, "&Unhide" );
	}
	else
	{
		menu.Append( ID_GUIED_VIEW_HIDESELECTED, "&Hide" );
	}
	menu.AppendSeparator();
	menu.Append( ID_GUIED_ITEM_PROPERTIES, "&Properties..." );
	menu.Append( ID_GUIED_ITEM_SCRIPTS, "&Scripts..." );

	menu.Bind( wxEVT_MENU, &rvGENavigator::OnContextHide, this, ID_GUIED_VIEW_HIDESELECTED );
	menu.Bind( wxEVT_MENU, &rvGENavigator::OnContextUnhide, this, ID_GUIED_VIEW_UNHIDESELECTED );
	menu.Bind( wxEVT_MENU, &rvGENavigator::OnContextProperties, this, ID_GUIED_ITEM_PROPERTIES );
	menu.Bind( wxEVT_MENU, &rvGENavigator::OnContextScripts, this, ID_GUIED_ITEM_SCRIPTS );

	PopupMenu( &menu );
}

/*
================
rvGENavigator::OnContextHide
================
*/
void rvGENavigator::OnContextHide( wxCommandEvent& )
{
	rvGENavItemData* data =	static_cast<rvGENavItemData*>( m_tree->GetItemData( m_contextItem ) );
	if( data && m_workspace )
	{
		m_workspace->HideWindow( data->GetWindow() );
	}
}

/*
================
rvGENavigator::OnContextUnhide
================
*/
void rvGENavigator::OnContextUnhide( wxCommandEvent& )
{
	rvGENavItemData* data =	static_cast<rvGENavItemData*>( m_tree->GetItemData( m_contextItem ) );
	if( data && m_workspace )
	{
		m_workspace->UnhideWindow( data->GetWindow() );
	}
}

/*
================
rvGENavigator::OnContextProperties
================
*/
void rvGENavigator::OnContextProperties( wxCommandEvent& )
{
	if( !m_workspace )
	{
		return;
	}

	rvGENavItemData* data =	static_cast<rvGENavItemData*>( m_tree->GetItemData( m_contextItem ) );
	if( data )
	{
		m_workspace->GetSelectionMgr().Set( data->GetWindow() );
	}

	m_workspace->EditSelectedProperties();
}

/*
================
rvGENavigator::OnContextScripts
================
*/
void rvGENavigator::OnContextScripts( wxCommandEvent& )
{
	if( !m_workspace )
	{
		return;
	}

	rvGENavItemData* data =
		static_cast<rvGENavItemData*>( m_tree->GetItemData( m_contextItem ) );
	if( data )
	{
		m_workspace->GetSelectionMgr().Set( data->GetWindow() );
	}

	m_workspace->EditSelectedScripts();
}