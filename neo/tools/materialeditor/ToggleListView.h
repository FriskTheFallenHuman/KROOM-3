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

#ifndef __TOGGLELISTVIEW_H__
#define __TOGGLELISTVIEW_H__

#include <wx/mstream.h>
#include <wx/wx.h>
#include <wx/vlbox.h>
#include <wx/bitmap.h>
#include <vector>

/**
 * A single-column owner-drawn list with a toggle icon in front of each row.
 */
class ToggleListView : public wxVListBox
{
public:
	enum
	{
		TOGGLE_STATE_DISABLED = 0,
		TOGGLE_STATE_ON,
		TOGGLE_STATE_OFF
	};

	ToggleListView( wxWindow* parent );
	virtual ~ToggleListView();

	void SetToggleIcons( const char* disabled, const char* on, const char* off );

	void SetToggleState( int index, int toggleState, bool notify = false );
	int  GetToggleState( int index );

	int      AddItem( const wxString& text, int state = TOGGLE_STATE_DISABLED );
	void     InsertItem( int index, const wxString& text, int state );
	void     DeleteItem( int index );
	void     DeleteAllItems();
	void     SetItemText( int index, const wxString& text );
	wxString GetItemText( int index ) const;
	int      GetItemCount() const
	{
		return ( int )m_items.size();
	}

	int      HitTest( const wxPoint& pt ) const;

protected:
	// wxVListBox overrides.
	virtual void    OnDrawItem( wxDC& dc, const wxRect& rect, size_t n ) const override;
	virtual wxCoord OnMeasureItem( size_t n ) const override;

	// Childs implements these.
	virtual void    OnStateChanged( int index, int toggleState ) {}
	virtual void    OnRowLeftDown( int index, const wxPoint& pt ) {}

	// Events.
	void OnLeftDown( wxMouseEvent& event );
	void OnKeyDown( wxKeyEvent& event );

private:
	struct Item_t
	{
		wxString    text;
		int         state;
	};

	std::vector<Item_t> m_items;

	wxBitmap    m_onIcon;
	wxBitmap    m_offIcon;
	wxBitmap    m_disabledIcon;

	int         m_iconWidth;    // width of the icon column
	int         m_itemHeight;   // height of each row
	int         m_textOffset;   // horizontal gap between icon and text
};

#endif /* !__TOGGLELISTVIEW_H__ */