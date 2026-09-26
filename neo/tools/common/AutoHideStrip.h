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

#ifndef __AUTOHIDESTRIP_H__
#define __AUTOHIDESTRIP_H__

#include <wx/wx.h>
#include <functional>
#include <vector>

/**
 * A thin edge strip showing tabs for hidden dock panes.
 */
class AutoHideStrip : public wxPanel
{
public:
	enum Orientation
	{
		Left,
		Right,
		Bottom
	};

	typedef std::function<void( wxWindow* paneWindow )> TabClickHandler;

	AutoHideStrip( wxWindow* parent, Orientation orient );
	virtual ~AutoHideStrip();

	// Add a hidden pane to the strip.
	void AddItem( wxWindow* paneWindow, const wxString& label );

	// Remove all items.
	void Clear();

	// Recompute sizes and repaint.
	void Rebuild();

	// How many tabs are currently on the strip.
	size_t GetItemCount() const
	{
		return m_items.size();
	}

	// Invoked when the user clicks a tab.
	void SetTabClickHandler( TabClickHandler handler )
	{
		m_onTabClick = handler;
	}

	// Thickness of the strip in pixels.
	static int GetThickness()
	{
		return 22;
	}

protected:
	void OnPaint( wxPaintEvent& event );
	void OnLeftDown( wxMouseEvent& event );
	void OnMouseMove( wxMouseEvent& event );
	void OnLeaveWindow( wxMouseEvent& event );
	void OnSize( wxSizeEvent& event );

private:
	struct Item
	{
		wxWindow* window;
		wxString  label;
		int       length;
	};

	// Hit rectangle for a tab..
	wxRect TabRect( int index ) const;

	// Index of the tab at pt.
	int    TabIndexAt( const wxPoint& pt ) const;

	// Measures each label and saves it.
	void   RecalcTabLengths();

	// Paints one tab.
	void   DrawTab( wxDC& dc, int index, const wxRect& rect, bool hovered );

	std::vector<Item> m_items;
	Orientation       m_orient;
	int               m_hoverIndex;
	TabClickHandler   m_onTabClick;
};

#endif /* !__AUTOHIDESTRIP_H__ */