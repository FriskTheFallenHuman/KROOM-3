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

static wxCriticalSection s_registryLock;
static std::unordered_map< idWindow*, rvGEWindowWrapper* > s_registry;

rvGEWindowWrapper::rvGEWindowWrapper( idWindow* window, EWindowType type )
{
	assert( window );

	m_window = window;

	m_flippedHorz = false;
	m_flippedVert = false;
	m_hidden = false;
	m_deleted = false;
	m_selected = false;
	m_expanded = false;
	m_oldVisible = false;
	m_moveable = true;
	m_type = type;

	// Refine the type based on the actual C++ class of the window. The
	// caller's hint is only a fallback for the base idWindow case.
	if( dynamic_cast< idEditWindow* >( window ) )
	{
		m_type = WT_EDIT;
	}
	else if( dynamic_cast< idListWindow* >( window ) )
	{
		m_type = WT_LIST;
	}
	else if( dynamic_cast< idBindWindow* >( window ) )
	{
		m_type = WT_BIND;
	}
	else if( dynamic_cast< idRenderWindow* >( window ) )
	{
		m_type = WT_RENDER;
	}
	else if( dynamic_cast< idChoiceWindow* >( window ) )
	{
		m_type = WT_CHOICE;
	}
	else
	{
		m_type = WT_NORMAL;
	}

	// Register with the lookup table.
	{
		wxCriticalSectionLocker lock( s_registryLock );
		s_registry[ window ] = this;
	}

	// Seed the state dict with the window's name so the property grid
	// always has something to show.
	SetStateKey( "name", m_window->GetName(), false );
}

/*
================
rvGEWindowWrapper::~rvGEWindowWrapper

Unregisters from the lookup table. The idWindow itself is owned by the
idUserInterfaceLocal, not by the wrapper.
================
*/
rvGEWindowWrapper::~rvGEWindowWrapper()
{
	wxCriticalSectionLocker lock( s_registryLock );
	s_registry.erase( m_window );
}

/*
================
rvGEWindowWrapper::GetWrapper

Static method that returns the window wrapper for the given window class
================
*/
rvGEWindowWrapper* rvGEWindowWrapper::GetWrapper( idWindow* window )
{
	if( !window )
	{
		return NULL;
	}

	wxCriticalSectionLocker lock( s_registryLock );

	std::unordered_map< idWindow*, rvGEWindowWrapper* >::iterator it =
		s_registry.find( window );

	if( it == s_registry.end() )
	{
		return NULL;
	}

	return it->second;
}

/*
================
rvGEWindowWrapper::ClearAll

Wipes every wrapper from the registry without deleting them. Callers that
own the wrappers are responsible for deleting them first; this exists
purely to prevent stale lookups surviving a shutdown/reload boundary.
================
*/
void rvGEWindowWrapper::ClearAll()
{
	wxCriticalSectionLocker lock( s_registryLock );
	s_registry.clear();
}

/*
================
rvGEWindowWrapper::UpdateRect

Updates the gui editor's representation of the window rectangle from the
windows rectangle
================
*/
void rvGEWindowWrapper::UpdateRect()
{
	idVec4 rect;
	idWinRectangle* winrect;

	winrect = dynamic_cast< idWinRectangle* >( m_window->GetWinVarByName( "rect" ) );
	assert( winrect );

	rect = winrect->ToVec4();

	m_flippedHorz = false;
	m_flippedVert = false;

	if( rect[2] < 0 )
	{
		m_flippedHorz = true;
		rect[2] *= -1;
	}

	if( rect[3] < 0 )
	{
		m_flippedVert = true;
		rect[3] *= -1;
	}

	m_clientRect = rect;

	CalcScreenRect();

	const char* rstr = m_state.GetString( "rect", "0,0,0,0" );
	m_moveable = !IsExpression( rstr );
}

/*
================
rvGEWindowWrapper::CalcScreenRect

Calculates the screen rectangle from the client rectangle by running through
the parent windows and adding their offsets
================
*/
void rvGEWindowWrapper::CalcScreenRect()
{
	idWindow* parent;

	m_screenRect = m_clientRect;

	if( NULL != ( parent = m_window->GetParent() ) )
	{
		rvGEWindowWrapper* wrapper = GetWrapper( parent );
		assert( wrapper );

		m_screenRect[0] += wrapper->GetScreenRect()[0];
		m_screenRect[1] += wrapper->GetScreenRect()[1];
	}

	// Since our screen rectangle has changed we need to tell all our our
	// children to update their screen rectangles as well.
	int i;
	int count;
	rvGEWindowWrapper* pwrapper;

	pwrapper = rvGEWindowWrapper::GetWrapper( m_window );

	for( count = pwrapper->GetChildCount(), i = 0; i < count; i++ )
	{
		rvGEWindowWrapper* wrapper;

		wrapper = rvGEWindowWrapper::GetWrapper( pwrapper->GetChild( i ) );

		// Usually we assert if there is no wrapper but since this method is
		// called when the wrappers are being attached to the windows there
		// may be no wrappers for any of the children.
		if( !wrapper )
		{
			continue;
		}

		wrapper->CalcScreenRect();
	}
}

/*
================
rvGEWindowWrapper::SetRect

Sets the wrapper's rectangle and the attached windows rectangle as well
================
*/
void rvGEWindowWrapper::SetRect( idRectangle& rect )
{
	const char* s;

	m_clientRect = rect;
	CalcScreenRect();

	s = va( "%d,%d,%d,%d",
			( int )( rect.x + 0.5f ),
			( int )( rect.y + 0.5f ),
			( int )( ( rect.w + 0.5f ) * ( m_flippedHorz ? -1 : 1 ) ),
			( int )( ( rect.h + 0.5f ) * ( m_flippedVert ? -1 : 1 ) ) );

	m_state.Set( "rect", s );

	UpdateWindowState();
}

/*
================
rvGEWindowWrapper::SetHidden

Sets the wrappers hidden state
================
*/
void rvGEWindowWrapper::SetHidden( bool h )
{
	m_hidden = h;

	UpdateWindowState();
}

/*
================
rvGEWindowWrapper::SetDeleted

Sets the deleted state of the wrapper which in turns sets whether or
not the window is visible
================
*/
void rvGEWindowWrapper::SetDeleted( bool del )
{
	m_deleted = del;

	UpdateWindowState();
}

/*
================
rvGEWindowWrapper::SetState

Sets the window state from the given dictionary
================
*/
void rvGEWindowWrapper::SetState( const idDict& dict )
{
	m_state.Clear();
	m_state.Copy( dict );

	// Push the window state to the window itself
	UpdateWindowState();

	// Update the internal rectangle since it may have changed in the state
	UpdateRect();
}

/*
================
rvGEWindowWrapper::SetStateKey

Sets the given state key and updates the
================
*/
void rvGEWindowWrapper::SetStateKey( const char* key, const char* value, bool update )
{
	m_state.Set( key, value );

	if( update )
	{
		UpdateWindowState();

		// Make sure the rectangle gets updated if its changing
		if( !idStr::Icmp( key, "rect" ) )
		{
			UpdateRect();
		}
	}
}

/*
================
rvGEWindowWrapper::DeleteStateKey

Sets the given state key and updates the
================
*/
void rvGEWindowWrapper::DeleteStateKey( const char* key )
{
	if( !idStr::Icmp( key, "rect" ) || !idStr::Icmp( key, "name" ) )
	{
		return;
	}

	m_state.Delete( key );
}

/*
================
rvGEWindowWrapper::UpdateWindowState

Updates the windows real state with wrappers internal state.  Visibility is
handled specially
================
*/
void rvGEWindowWrapper::UpdateWindowState()
{
	idStr realVisible;
	bool tempVisible;

	realVisible = m_state.GetString( "visible", "1" );
	tempVisible = atoi( realVisible ) ? true : false;
	if( tempVisible != m_oldVisible )
	{
		m_hidden = !tempVisible;
		m_oldVisible = tempVisible;
	}

	tempVisible = !m_deleted && !m_hidden;

	// Temporarily change the visible state so we can hide/unhide the window
	m_state.Set( "visible", tempVisible ? "1" : "0" );

	m_window->UpdateFromDictionary( m_state );

	// Put the visible state back to the real value
	m_state.Set( "visible", realVisible );
}

/*
================
rvGEWindowWrapper::WindowFromPoint

Returns the topmost window under the given point
================
*/
idWindow* rvGEWindowWrapper::WindowFromPoint( float x, float y, bool visibleOnly )
{
	int count;
	int i;
	rvGEWindowWrapper* pwrapper;

	// If the window isnt visible then skip it
	if( visibleOnly && ( m_hidden || m_deleted ) )
	{
		return NULL;
	}

	// Now check our children next
	pwrapper = GetWrapper( m_window );
	count = pwrapper->GetChildCount();

	for( i = count - 1; i >= 0; i-- )
	{
		rvGEWindowWrapper* wrapper;
		idWindow* child;

		child = pwrapper->GetChild( i );
		assert( child );

		wrapper = rvGEWindowWrapper::GetWrapper( child );
		if( !wrapper )
		{
			continue;
		}

		if( ( child = wrapper->WindowFromPoint( x, y ) ) )
		{
			return child;
		}
	}

	// We have to check this last because a child could be out outside of the
	// parents rectangle and we still want it selectable.
	if( !m_screenRect.Contains( x, y ) )
	{
		return NULL;
	}

	return m_window;
}

/*
================
rvGEWindowWrapper::StringToWindowType

Converts the given string to a window type
================
*/
rvGEWindowWrapper::EWindowType rvGEWindowWrapper::StringToWindowType( const char* string )
{
	if( !idStr::Icmp( string, "windowDef" ) )
	{
		return WT_NORMAL;
	}
	else if( !idStr::Icmp( string, "editDef" ) )
	{
		return WT_EDIT;
	}
	else if( !idStr::Icmp( string, "choiceDef" ) )
	{
		return WT_CHOICE;
	}
	else if( !idStr::Icmp( string, "sliderDef" ) )
	{
		return WT_SLIDER;
	}
	else if( !idStr::Icmp( string, "bindDef" ) )
	{
		return WT_BIND;
	}
	else if( !idStr::Icmp( string, "listDef" ) )
	{
		return WT_LIST;
	}
	else if( !idStr::Icmp( string, "renderDef" ) )
	{
		return WT_RENDER;
	}
	else if( !idStr::Icmp( string, "htmlDef" ) )
	{
		return WT_HTML;
	}

	return WT_UNKNOWN;
}

/*
================
rvGEWindowWrapper::WindowTypeToString

Converts the given window type to a string
================
*/
const char* rvGEWindowWrapper::WindowTypeToString( EWindowType type )
{
	static const char* typeNames[] =
	{
		"Unknown", "windowDef", "editDef", "htmlDef", "choiceDef",
		"sliderDef", "bindDef", "listDef", "renderDef"
	};

	return typeNames[( int ) type];
}

/*
================
rvGEWindowWrapper::GetChildCount

Returns the number of children the window being wrapped has
================
*/
int rvGEWindowWrapper::GetChildCount()
{
	if( !CanHaveChildren() )
	{
		return 0;
	}

	return m_window->GetChildCount();
}

/*
================
rvGEWindowWrapper::EnumChildren

Enumerates over the child windows while properly ignoring any that
are not wrapped.
================
*/
bool rvGEWindowWrapper::EnumChildren( PFNENUMCHILDRENPROC proc, void* data )
{
	int count;
	int i;

	if( !CanHaveChildren() )
	{
		return false;
	}

	for( count = GetChildCount(), i = 0; i < count; i++ )
	{
		if( !proc( rvGEWindowWrapper::GetWrapper( GetChild( i ) ), data ) )
		{
			return false;
		}
	}

	return true;
}

/*
================
rvGEWindowWrapper::CanHaveChildren

Returns true if the window is allowed to have child windows
================
*/
bool rvGEWindowWrapper::CanHaveChildren()
{
	if( m_type == WT_HTML || m_type == WT_LIST )
	{
		return false;
	}

	return true;
}

/*
================
rvGEWindowWrapper::GetDepth

Returns the depth of the wrapped window
================
*/
int rvGEWindowWrapper::GetDepth()
{
	idWindow* parent;
	int depth;

	for( depth = 0, parent = m_window->GetParent(); parent; parent = parent->GetParent(), depth++ )
		;

	return depth;
}

/*
================
rvGEWindowWrapper::Expand

Expand the window in the navigator and all of its parents too
================
*/
bool rvGEWindowWrapper::Expand()
{
	bool result;

	if( m_window->GetParent() )
	{
		result = rvGEWindowWrapper::GetWrapper( m_window->GetParent() )->Expand();
	}
	else
	{
		result = false;
	}

	if( m_expanded || !CanHaveChildren() || !GetChildCount() )
	{
		return result;
	}

	m_expanded = true;

	return true;
}

/*
================
rvGEWindowWrapper::Collapse

Returns the depth of the wrapped window
================
*/
bool rvGEWindowWrapper::Collapse()
{
	bool result;

	if( m_window->GetParent() )
	{
		result = rvGEWindowWrapper::GetWrapper( m_window->GetParent() )->Expand();
	}
	else
	{
		result = false;
	}

	if( !m_expanded )
	{
		return result;
	}

	m_expanded = false;

	return true;
}

/*
================
rvGEWindowWrapper::Finish

After a the windwo wrapper is attached to a window and the window is parsed
the finish method is called to finish up any last details
================
*/
void rvGEWindowWrapper::Finish()
{
	m_oldVisible = ( bool ) * dynamic_cast< idWinBool* >( m_window->GetWinVarByName( "visible" ) );
	m_hidden = m_oldVisible ? false : true;
	UpdateRect();
}

/*
================
rvGEWindowWrapper::VerfiyStateKey
================
*/
bool rvGEWindowWrapper::VerfiyStateKey( const char* name, const char* value, idStr* result )
{
	idStr old;
	bool failed;
	idParser src( value, strlen( value ), "",
				  LEXFL_NOSTRINGCONCAT |
				  LEXFL_ALLOWMULTICHARLITERALS |
				  LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	// Save the current value
	old = m_state.GetString( name );
	failed = false;

	// Try to parse in the value
	try
	{
		if( !m_window->ParseInternalVar( name, &src ) )
		{
			// Kill the old register since the parse reg entry will add a new one
			if( !m_window->ParseRegEntry( name, &src ) )
			{
				failed = true;
			}
		}
	}
	catch( idException& )
	{
		failed = true;
	}

	// Restore the old value
	idParser src2( old, old.Length(), "",
				   LEXFL_NOFATALERRORS |
				   LEXFL_NOSTRINGCONCAT |
				   LEXFL_ALLOWMULTICHARLITERALS |
				   LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
	if( !m_window->ParseInternalVar( name, &src2 ) )
	{
		if( !m_window->ParseRegEntry( name, &src2 ) )
		{
		}
	}

	// Check to see if the old value matches the new value
	idStr before;
	idStr after;

	before = value;
	before.StripTrailingWhitespace();
	src.GetStringFromMarker( after );
	after.StripTrailingWhitespace();

	if( result )
	{
		*result = after;
	}

	if( failed || before.Cmp( after ) )
	{
		return false;
	}

	return true;
}