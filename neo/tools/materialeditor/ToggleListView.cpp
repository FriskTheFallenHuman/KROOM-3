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

#include "ToggleListView.h"

/*
================
LoadBitmap
================
*/
static wxBitmap LoadBitmap( const char* relativePath )
{
	if( !relativePath )
	{
		return wxBitmap();
	}

	void* buffer = NULL;
	int len = fileSystem->ReadFile( relativePath, &buffer, NULL );
	if( len <= 0 || buffer == NULL )
	{
		return wxBitmap();
	}

	wxMemoryInputStream stream( buffer, len );
	wxImage image;
	bool ok = image.LoadFile( stream, wxBITMAP_TYPE_ANY );
	fileSystem->FreeFile( buffer );

	if( !ok || !image.IsOk() )
	{
		return wxBitmap();
	}

	return wxBitmap( image );
}

/*
================
ToggleListView::ToggleListView
================
*/
ToggleListView::ToggleListView( wxWindow* parent )
	: wxVListBox( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
				  wxBORDER_SIMPLE | wxLB_SINGLE )
	, m_iconWidth( 22 )
	, m_itemHeight( 22 )
	, m_textOffset( 6 )
{
	Bind( wxEVT_LEFT_DOWN, &ToggleListView::OnLeftDown, this );
	Bind( wxEVT_KEY_DOWN, &ToggleListView::OnKeyDown, this );

	SetItemCount( 0 );
}

ToggleListView::~ToggleListView()
{
}

/*
================
ToggleListView::SetToggleIcons
================
*/
void ToggleListView::SetToggleIcons( const char* disabled, const char* on, const char* off )
{
	m_onIcon = LoadBitmap( on );
	m_offIcon = LoadBitmap( off );
	m_disabledIcon = LoadBitmap( disabled );
}

/*
================
ToggleListView::SetToggleState
================
*/
void ToggleListView::SetToggleState( int index, int toggleState, bool notify )
{
	if( index < 0 || index >= ( int )m_items.size() )
	{
		return;
	}

	int oldState = m_items[index].state;
	m_items[index].state = toggleState;

	RefreshRow( ( size_t )index );

	if( notify && oldState != toggleState )
	{
		OnStateChanged( index, toggleState );
	}
}

/*
================
ToggleListView::GetToggleState
================
*/
int ToggleListView::GetToggleState( int index )
{
	if( index < 0 || index >= ( int )m_items.size() )
	{
		return TOGGLE_STATE_DISABLED;
	}
	return m_items[index].state;
}

/*
================
ToggleListView::AddItem
================
*/
int ToggleListView::AddItem( const wxString& text, int state )
{
	Item_t item;
	item.text  = text;
	item.state = state;
	m_items.push_back( item );

	SetItemCount( ( int )m_items.size() );
	return ( int )m_items.size() - 1;
}

/*
================
ToggleListView::InsertItem
================
*/
void ToggleListView::InsertItem( int index, const wxString& text, int state )
{
	if( index < 0 )
	{
		index = 0;
	}
	if( index > ( int )m_items.size() )
	{
		index = ( int )m_items.size();
	}

	Item_t item;
	item.text  = text;
	item.state = state;
	m_items.insert( m_items.begin() + index, item );

	SetItemCount( ( int )m_items.size() );
}

/*
================
ToggleListView::DeleteItem
================
*/
void ToggleListView::DeleteItem( int index )
{
	if( index < 0 || index >= ( int )m_items.size() )
	{
		return;
	}

	m_items.erase( m_items.begin() + index );
	SetItemCount( ( int )m_items.size() );
	RefreshAll();
}

/*
================
ToggleListView::DeleteAllItems
================
*/
void ToggleListView::DeleteAllItems()
{
	m_items.clear();
	SetItemCount( 0 );
	RefreshAll();
}

/*
================
ToggleListView::SetItemText
================
*/
void ToggleListView::SetItemText( int index, const wxString& text )
{
	if( index < 0 || index >= ( int )m_items.size() )
	{
		return;
	}

	m_items[index].text = text;
	RefreshRow( ( size_t )index );
}

/*
================
ToggleListView::GetItemText
================
*/
wxString ToggleListView::GetItemText( int index ) const
{
	if( index < 0 || index >= ( int )m_items.size() )
	{
		return wxString();
	}
	return m_items[index].text;
}

/*
================
ToggleListView::HitTest
================
*/
int ToggleListView::HitTest( const wxPoint& pt ) const
{
	for( size_t i = 0; i < m_items.size(); i++ )
	{
		wxRect r = GetItemRect( i );
		if( r.Contains( pt ) )
		{
			return ( int )i;
		}
	}
	return -1;
}

/*
================
ToggleListView::OnMeasureItem
================
*/
wxCoord ToggleListView::OnMeasureItem( size_t /*n*/ ) const
{
	return m_itemHeight;
}

/*
================
ToggleListView::OnDrawItem
================
*/
void ToggleListView::OnDrawItem( wxDC& dc, const wxRect& rect, size_t n ) const
{
	if( n >= m_items.size() )
	{
		return;
	}

	const Item_t& item = m_items[n];

	const bool selected = IsSelected( ( int )n );

	const wxColour bgColour = selected
							  ? wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT )
							  : wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
	const wxColour fgColour = selected
							  ? wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT )
							  : wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

	dc.SetBrush( wxBrush( bgColour ) );
	dc.SetPen( *wxTRANSPARENT_PEN );
	dc.DrawRectangle( rect );

	wxRect iconRect( rect.x, rect.y, m_iconWidth, rect.height );

	dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE ) ) );
	dc.DrawRectangle( iconRect );

	// Draw a subtle 3D border around the icon cell.
	dc.SetBrush( *wxTRANSPARENT_BRUSH );
	dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW ) ) );
	dc.DrawLine( iconRect.x, iconRect.y, iconRect.x, iconRect.y + iconRect.height );
	dc.DrawLine( iconRect.x, iconRect.y, iconRect.x + iconRect.width, iconRect.y );
	dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_3DHILIGHT ) ) );
	dc.DrawLine( iconRect.x + iconRect.width - 1, iconRect.y + 1, iconRect.x + iconRect.width - 1, iconRect.y + iconRect.height - 1 );
	dc.DrawLine( iconRect.x + 1, iconRect.y + iconRect.height - 1, iconRect.x + iconRect.width - 1, iconRect.y + iconRect.height - 1 );

	const wxBitmap* icon = NULL;
	switch( item.state )
	{
		case TOGGLE_STATE_ON:
			if( m_onIcon.IsOk() )
			{
				icon = &m_onIcon;
			}
			break;
		case TOGGLE_STATE_OFF:
			if( m_offIcon.IsOk() )
			{
				icon = &m_offIcon;
			}
			break;
		case TOGGLE_STATE_DISABLED:
		default:
			if( m_disabledIcon.IsOk() )
			{
				icon = &m_disabledIcon;
			}
			break;
	}

	if( icon )
	{
		int iconX = iconRect.x + ( iconRect.width  - icon->GetWidth() ) / 2;
		int iconY = iconRect.y + ( iconRect.height - icon->GetHeight() ) / 2;
		dc.DrawBitmap( *icon, iconX, iconY, true );
	}
	else
	{
		const int cx = iconRect.x + iconRect.width / 2;
		const int cy = iconRect.y + iconRect.height / 2;
		const int r  = 4;

		switch( item.state )
		{
			case TOGGLE_STATE_ON:
				dc.SetBrush( wxBrush( *wxGREEN ) );
				dc.SetPen( *wxBLACK_PEN );
				dc.DrawCircle( cx, cy, r );
				break;
			case TOGGLE_STATE_OFF:
				dc.SetBrush( *wxTRANSPARENT_BRUSH );
				dc.SetPen( *wxBLACK_PEN );
				dc.DrawCircle( cx, cy, r );
				break;
			case TOGGLE_STATE_DISABLED:
			default:
				dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) ) );
				dc.SetPen( *wxBLACK_PEN );
				dc.DrawCircle( cx, cy, r );
				break;
		}
	}

	// Text
	wxRect textRect = rect;
	textRect.x     += m_iconWidth + m_textOffset;
	textRect.width -= m_iconWidth + m_textOffset;

	dc.SetTextForeground( fgColour );
	dc.SetClippingRegion( textRect );
	dc.DrawLabel( item.text, textRect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
	dc.DestroyClippingRegion();
}

/*
================
ToggleListView::OnLeftDown
================
*/
void ToggleListView::OnLeftDown( wxMouseEvent& event )
{
	SetFocus();

	const int index = VirtualHitTest( event.GetY() );
	if( index != wxNOT_FOUND && event.GetX() < m_iconWidth )
	{
		const int state = GetToggleState( index );
		if( state != TOGGLE_STATE_DISABLED )
		{
			if( state == TOGGLE_STATE_ON )
			{
				SetToggleState( index, TOGGLE_STATE_OFF, true );
			}
			else
			{
				SetToggleState( index, TOGGLE_STATE_ON, true );
			}
			return;
		}
	}

	if( index != wxNOT_FOUND )
	{
		OnRowLeftDown( index, event.GetPosition() );
	}

	event.Skip();
}

/*
================
ToggleListView::OnKeyDown
================
*/
void ToggleListView::OnKeyDown( wxKeyEvent& event )
{
	event.Skip();
}