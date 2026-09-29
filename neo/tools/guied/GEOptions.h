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

#ifndef __GEOPTIONS_H__
#define __GEOPTIONS_H__

#ifndef __REGISTRYOPTIONS_H__
	#include "../common/RegistryOptions.h"
#endif

class idRectangle;

class rvGEOptions : public rvRegistryOptions
{
public:
	static const int MAX_MRU_SIZE = rvRegistryOptions::MAX_MRU_SIZE;

	rvGEOptions();
	virtual ~rvGEOptions() {}

	void			Init();
	bool			Save();
	bool			Load();

	// Selection / workspace colors
	void			SetSelectionColor( const idVec4& color );
	void			SetSelectionColor( const wxColour& color );
	idVec4&			GetSelectionColor();

	void			SetWorkspaceColor( const idVec4& color );
	idVec4&			GetWorkspaceColor();

	// Grid
	void			SetGridColor( const idVec4& color );
	void			SetGridColor( const wxColour& color );
	idVec4&			GetGridColor();

	void			SetGridWidth( int width );
	int				GetGridWidth();
	void			SetGridHeight( int height );
	int				GetGridHeight();
	void			SetGridVisible( bool vis );
	bool			GetGridVisible();
	void			SetGridSnap( bool snap );
	bool			GetGridSnap();

	// Window visibility
	void			SetNavigatorVisible( bool vis );
	bool			GetNavigatorVisible();
	void			SetPropertiesVisible( bool vis );
	bool			GetPropertiesVisible();
	void			SetTransformerVisible( bool vis );
	bool			GetTransformerVisible();
	void			SetStatusBarVisible( bool vis );
	bool			GetStatusBarVisible();

	// Misc
	void			SetLastOptionsPage( int page );
	int				GetLastOptionsPage();
	void			SetIgnoreDesktopSelect( bool ignore );
	bool			GetIgnoreDesktopSelect();

	// Colors.
	void			SetCustomColors( const wxColour colours[16] );
	wxColour*		GetCustomColors();

	// Snapping.
	void			SnapRectToGrid( idRectangle& rect, bool snapLeft = true, bool snapTop = true, bool snapWidth = true, bool snapHeight = true );

protected:
	void			SetModified( bool mod )
	{
		m_modified = mod;
	}

	void			ConvertColor( const wxColour& src, idVec4& dest );

	bool m_modified;
	int m_lastOptionsPage;

	idVec4 mGridColor;
	int mGridWidth;
	int mGridHeight;
	bool mGridSnap;
	bool mGridVisible;

	idVec4 mWorkspaceColor;
	idVec4 mSelectionColor;

	bool mNavigatorVisible;
	bool mPropertiesVisible;
	bool mTransformerVisible;
	bool mStatusBarVisible;
	bool mIgnoreDesktopSelect;

	wxColour mCustomColors[16];
};

ID_INLINE void rvGEOptions::SetGridWidth( int width )
{
	mGridWidth = width;
	SetModified( true );
}

ID_INLINE int rvGEOptions::GetGridWidth()
{
	return mGridWidth;
}

ID_INLINE void rvGEOptions::SetGridHeight( int height )
{
	mGridHeight = height;
	SetModified( true );
}

ID_INLINE int rvGEOptions::GetGridHeight()
{
	return mGridHeight;
}

ID_INLINE void rvGEOptions::SetGridSnap( bool snap )
{
	mGridSnap = snap;
	SetModified( true );
}

ID_INLINE bool rvGEOptions::GetGridSnap()
{
	return mGridSnap;
}

ID_INLINE void rvGEOptions::SetGridVisible( bool vis )
{
	mGridVisible = vis;
	SetModified( true );
}

ID_INLINE bool rvGEOptions::GetGridVisible()
{
	return mGridVisible;
}

ID_INLINE void rvGEOptions::SetStatusBarVisible( bool vis )
{
	mStatusBarVisible = vis;
	SetModified( true );
}

ID_INLINE bool rvGEOptions::GetStatusBarVisible()
{
	return mStatusBarVisible;
}

ID_INLINE void rvGEOptions::SetGridColor( const wxColour& color )
{
	ConvertColor( color, mGridColor );
	SetModified( true );
}

ID_INLINE void rvGEOptions::SetGridColor( const idVec4& color )
{
	mGridColor = color;
	SetModified( true );
}

ID_INLINE idVec4& rvGEOptions::GetGridColor()
{
	return mGridColor;
}

ID_INLINE void rvGEOptions::SetSelectionColor( const idVec4& color )
{
	mSelectionColor = color;
	SetModified( true );
}

ID_INLINE void rvGEOptions::SetSelectionColor( const wxColour& color )
{
	ConvertColor( color, mSelectionColor );
	SetModified( true );
}

ID_INLINE idVec4& rvGEOptions::GetSelectionColor()
{
	return mSelectionColor;
}

ID_INLINE void rvGEOptions::SetWorkspaceColor( const idVec4& color )
{
	mWorkspaceColor = color;
	SetModified( true );
}

ID_INLINE idVec4& rvGEOptions::GetWorkspaceColor()
{
	return mWorkspaceColor;
}

ID_INLINE void rvGEOptions::SetNavigatorVisible( bool vis )
{
	mNavigatorVisible = vis;
	SetModified( true );
}

ID_INLINE bool rvGEOptions::GetNavigatorVisible()
{
	return mNavigatorVisible;
}

ID_INLINE void rvGEOptions::SetPropertiesVisible( bool vis )
{
	mPropertiesVisible = vis;
	SetModified( true );
}

ID_INLINE bool rvGEOptions::GetPropertiesVisible()
{
	return mPropertiesVisible;
}

ID_INLINE void rvGEOptions::SetTransformerVisible( bool vis )
{
	mTransformerVisible = vis;
	SetModified( true );
}

ID_INLINE bool rvGEOptions::GetTransformerVisible()
{
	return mTransformerVisible;
}

ID_INLINE int rvGEOptions::GetLastOptionsPage()
{
	return m_lastOptionsPage;
}

ID_INLINE void rvGEOptions::SetLastOptionsPage( int page )
{
	m_lastOptionsPage = page;
}

ID_INLINE void rvGEOptions::SetIgnoreDesktopSelect( bool ignore )
{
	mIgnoreDesktopSelect = ignore;
	SetModified( true );
}

ID_INLINE bool rvGEOptions::GetIgnoreDesktopSelect()
{
	return mIgnoreDesktopSelect;
}

ID_INLINE wxColour* rvGEOptions::GetCustomColors()
{
	return mCustomColors;
}

#endif // _GEOPTIONS_H_
