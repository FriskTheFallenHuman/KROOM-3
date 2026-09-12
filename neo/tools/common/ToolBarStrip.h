/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software
Copyright (C) 2021 Harrie van Ginneken

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

#ifndef __TOOLBARSTRIP_H__
#define __TOOLBARSTRIP_H__

#include <wx/wx.h>
#include <wx/image.h>
#include <vector>

class rvToolbarImageStrip
{
public:
	rvToolbarImageStrip();
	~rvToolbarImageStrip();

	bool Load( const char* relativePath, int iconWidth = 16, int iconHeight = 16, const wxColour& transparentKey = wxColour() );
	bool LoadFromMemory( const void* data, int size, int iconWidth = 16, int iconHeight = 16, const wxColour& transparentKey = wxColour() );

	int GetIconCount()  const
	{
		return ( int )m_icons.size();
	}
	int GetIconWidth()  const
	{
		return m_iconWidth;
	}
	int GetIconHeight() const
	{
		return m_iconHeight;
	}
	bool IsOk()         const
	{
		return !m_icons.empty();
	}

	// Index 0-based, reading the grid left-to-right, top-to-bottom.
	// Returns an invalid bitmap if index is out of range.
	const wxBitmap& GetIcon( int index ) const;

	wxImageList* CreateImageList() const;

private:
	void SliceIntoIcons();
	bool DecodeBMP( const void* data, int size, wxImage& out );

	wxImage                   m_strip;
	std::vector<wxBitmap>     m_icons;
	int                       m_iconWidth;
	int                       m_iconHeight;
};

#endif /* !__TOOLBARSTRIP_H__ */