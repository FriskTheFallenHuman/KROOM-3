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

#ifndef __GEWINDOWWRAPPER_H__
#define __GEWINDOWWRAPPER_H__

typedef bool ( *PFNENUMCHILDRENPROC )( rvGEWindowWrapper* wrapper, void* data );

class rvGEWindowWrapper
{
public:

	enum EWindowType
	{
		WT_UNKNOWN,
		WT_NORMAL,
		WT_EDIT,
		WT_HTML,
		WT_CHOICE,
		WT_SLIDER,
		WT_BIND,
		WT_LIST,
		WT_RENDER,
		WT_TRANSITION
	};

	rvGEWindowWrapper( idWindow* window, EWindowType type );
	~rvGEWindowWrapper();

	static rvGEWindowWrapper* GetWrapper( idWindow* window );


	static void ClearAll();

	idWindow* GetWindow();
	idDict& GetStateDict();
	idDict& GetVariableDict();
	idDict& GetScriptDict();
	idRectangle& GetClientRect();
	idRectangle& GetScreenRect();
	EWindowType GetWindowType();

	int GetChildCount();
	int GetDepth();
	idWindow* GetChild( int index );

	void SetRect( idRectangle& rect );
	void SetState( const idDict& dict );
	void SetStateKey( const char* key, const char* value, bool update = true );
	void DeleteStateKey( const char* key );

	bool VerfiyStateKey( const char* name, const char* value, idStr* result = NULL );

	bool IsFlippedHorz()
	{
		return m_flippedHorz;
	}
	bool IsFlippedVert()
	{
		return m_flippedVert;
	}
	bool IsHidden()
	{
		return m_hidden;
	}
	bool IsDeleted()
	{
		return m_deleted;
	}
	bool IsSelected()
	{
		return m_selected;
	}
	bool IsExpanded()
	{
		return m_expanded;
	}

	bool CanHaveChildren();
	bool CanMoveAndSize()
	{
		return m_moveable;
	}

	void SetFlippedHorz( bool f )
	{
		m_flippedHorz = f;
	}

	void SetFlippedVert( bool f )
	{
		m_flippedVert = f;
	}

	void SetHidden( bool v );
	void SetDeleted( bool v );
	void SetSelected( bool sel )
	{
		m_selected = sel;
	}

	void SetWindowType( EWindowType type )
	{
		m_type = type;
	}

	bool Expand();
	bool Collapse();

	bool EnumChildren( PFNENUMCHILDRENPROC proc, void* data );

	idWindow* WindowFromPoint( float x, float y, bool visibleOnly = true );

	void Finish();

	static EWindowType StringToWindowType( const char* string );
	static const char* WindowTypeToString( EWindowType type );

protected:

	void CalcScreenRect();
	void UpdateRect();
	void UpdateWindowState();

	idRectangle m_clientRect;
	idRectangle m_screenRect;
	idWindow* m_window;
	idDict m_state;
	idDict m_variables;
	idDict m_scripts;
	bool m_flippedHorz;
	bool m_flippedVert;
	bool m_hidden;
	bool m_deleted;
	bool m_selected;
	bool m_expanded;
	bool m_oldVisible;
	bool m_moveable;
	EWindowType m_type;
};

ID_INLINE idDict& rvGEWindowWrapper::GetStateDict()
{
	return m_state;
}

ID_INLINE idDict& rvGEWindowWrapper::GetVariableDict()
{
	return m_variables;
}

ID_INLINE idDict& rvGEWindowWrapper::GetScriptDict()
{
	return m_scripts;
}

ID_INLINE idRectangle& rvGEWindowWrapper::GetClientRect()
{
	return m_clientRect;
}

ID_INLINE idRectangle& rvGEWindowWrapper::GetScreenRect()
{
	return m_screenRect;
}

ID_INLINE rvGEWindowWrapper::EWindowType rvGEWindowWrapper::GetWindowType()
{
	return m_type;
}

ID_INLINE idWindow* rvGEWindowWrapper::GetWindow()
{
	return m_window;
}

ID_INLINE idWindow* rvGEWindowWrapper::GetChild( int index )
{
	return m_window->GetChild( index );
}

#endif /* !__GEWINDOWWRAPPER_H__ */