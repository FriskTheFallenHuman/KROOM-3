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

#include <wx/dcbuffer.h>

#include "AutoHideStrip.h"

// Horizontal padding inside each tab, on each end.
static const int kTabPadding = 8;

// Vertical text rotation.
static const double kVerticalTextAngle = -90.0;

// How much lighter the hover background is vs. the strip background.
static const int kHoverLighten = 30;

/*
================
Lighten
================
*/
static wxColour Lighten( const wxColour& c, int amount )
{
	return wxColour(
			   ( unsigned char )wxMin( 255, ( int )c.Red() + amount ),
			   ( unsigned char )wxMin( 255, ( int )c.Green() + amount ),
			   ( unsigned char )wxMin( 255, ( int )c.Blue() + amount ) );
}

/*
================
AutoHideStrip::AutoHideStrip
================
*/
AutoHideStrip::AutoHideStrip( wxWindow* parent, Orientation orient )
	: wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE )
	, m_orient( orient )
	, m_hoverIndex( -1 )
{
	SetBackgroundStyle( wxBG_STYLE_PAINT );
	SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

	// Slightly smaller font than the parent.
	wxFont f = GetFont();
	if( f.GetPointSize() > 8 )
	{
		f.SetPointSize( f.GetPointSize() - 1 );
	}
	SetFont( f );

	Bind( wxEVT_PAINT, &AutoHideStrip::OnPaint, this );
	Bind( wxEVT_LEFT_DOWN, &AutoHideStrip::OnLeftDown, this );
	Bind( wxEVT_MOTION, &AutoHideStrip::OnMouseMove, this );
	Bind( wxEVT_LEAVE_WINDOW, &AutoHideStrip::OnLeaveWindow, this );
	Bind( wxEVT_SIZE, &AutoHideStrip::OnSize, this );
}

/*
================
AutoHideStrip::~AutoHideStrip
================
*/
AutoHideStrip::~AutoHideStrip()
{
}

/*
================
AutoHideStrip::AddItem
================
*/
void AutoHideStrip::AddItem( wxWindow* paneWindow, const wxString& label )
{
	Item item;
	item.window = paneWindow;
	item.label = label;
	item.length = 0;
	m_items.push_back( item );
}

/*
================
AutoHideStrip::Clear
================
*/
void AutoHideStrip::Clear()
{
	m_items.clear();
	m_hoverIndex = -1;
}

/*
================
AutoHideStrip::Rebuild
================
*/
void AutoHideStrip::Rebuild()
{
	const int thickness = GetThickness();

	if( m_orient == Bottom )
	{
		SetMinSize( wxSize( -1, thickness ) );
		SetMaxSize( wxSize( -1, thickness ) );
	}
	else
	{
		SetMinSize( wxSize( thickness, -1 ) );
		SetMaxSize( wxSize( thickness, -1 ) );
	}

	RecalcTabLengths();
	Refresh();
}

/*
================
AutoHideStrip::RecalcTabLengths
================
*/
void AutoHideStrip::RecalcTabLengths()
{
	wxClientDC dc( this );
	dc.SetFont( GetFont() );

	for( size_t i = 0; i < m_items.size(); ++i )
	{
		const wxSize sz = dc.GetTextExtent( m_items[i].label );
		m_items[i].length = sz.x + 2 * kTabPadding;
	}
}

/*
================
AutoHideStrip::TabRect
================
*/
wxRect AutoHideStrip::TabRect( int index ) const
{
	if( index < 0 || index >= ( int )m_items.size() )
	{
		return wxRect();
	}

	int offset = 0;
	for( int i = 0; i < index; ++i )
	{
		offset += m_items[i].length;
	}

	const wxSize sz = GetClientSize();
	const int len = m_items[index].length;

	if( m_orient == Bottom )
	{
		if( offset >= sz.x )
		{
			return wxRect();
		}
		const int visible = wxMin( len, sz.x - offset );
		return wxRect( offset, 0, visible, sz.y );
	}
	else
	{
		if( offset >= sz.y )
		{
			return wxRect();
		}
		const int visible = wxMin( len, sz.y - offset );
		return wxRect( 0, offset, sz.x, visible );
	}
}

/*
================
AutoHideStrip::TabIndexAt
================
*/
int AutoHideStrip::TabIndexAt( const wxPoint& pt ) const
{
	for( size_t i = 0; i < m_items.size(); ++i )
	{
		const wxRect r = TabRect( ( int )i );
		if( !r.IsEmpty() && r.Contains( pt ) )
		{
			return ( int )i;
		}
	}
	return -1;
}

/*
================
AutoHideStrip::OnSize
================
*/
void AutoHideStrip::OnSize( wxSizeEvent& event )
{
	Refresh();
	event.Skip();
}

/*
================
AutoHideStrip::OnPaint
================
*/
void AutoHideStrip::OnPaint( wxPaintEvent& )
{
	wxAutoBufferedPaintDC dc( this );

	dc.SetBackground( wxBrush( GetBackgroundColour() ) );
	dc.Clear();

	if( m_items.empty() )
	{
		return;
	}

	dc.SetFont( GetFont() );

	for( size_t i = 0; i < m_items.size(); ++i )
	{
		const wxRect r = TabRect( ( int )i );
		if( r.IsEmpty() )
		{
			continue;
		}
		DrawTab( dc, ( int )i, r, ( int )i == m_hoverIndex );
	}
}

/*
================
AutoHideStrip::DrawTab
================
*/
void AutoHideStrip::DrawTab( wxDC& dc, int index, const wxRect& rect, bool hovered )
{
	const Item& item = m_items[index];

	// Background.
	wxColour bg = GetBackgroundColour();
	if( hovered )
	{
		bg = Lighten( bg, kHoverLighten );
	}

	dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW ) ) );
	dc.SetBrush( wxBrush( bg ) );
	dc.DrawRectangle( rect );

	dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
	dc.SetClippingRegion( rect );

	if( m_orient == Bottom )
	{
		// Horizonta.
		dc.DrawLabel( item.label, rect, wxALIGN_CENTER );
	}
	else
	{
		// Vertical, reading top-to-bottom.
		const wxSize textSize = dc.GetTextExtent( item.label );

		const int baselineX = rect.x + ( rect.width + textSize.y ) / 2;
		const int startY = rect.y + kTabPadding;

		dc.DrawRotatedText( item.label, baselineX, startY, kVerticalTextAngle );
	}

	dc.DestroyClippingRegion();
}

/*
================
AutoHideStrip::OnLeftDown
================
*/
void AutoHideStrip::OnLeftDown( wxMouseEvent& event )
{
	const int idx = TabIndexAt( event.GetPosition() );

	if( idx >= 0 && idx < ( int )m_items.size() && m_onTabClick )
	{
		m_onTabClick( m_items[idx].window );
		return;
	}

	event.Skip();
}

/*
================
AutoHideStrip::OnMouseMove
================
*/
void AutoHideStrip::OnMouseMove( wxMouseEvent& event )
{
	const int idx = TabIndexAt( event.GetPosition() );

	if( idx != m_hoverIndex )
	{
		m_hoverIndex = idx;
		Refresh();
	}

	event.Skip();
}

/*
================
AutoHideStrip::OnLeaveWindow
================
*/
void AutoHideStrip::OnLeaveWindow( wxMouseEvent& event )
{
	if( m_hoverIndex != -1 )
	{
		m_hoverIndex = -1;
		Refresh();
	}

	event.Skip();
}