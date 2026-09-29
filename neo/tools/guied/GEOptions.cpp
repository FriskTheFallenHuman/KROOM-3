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

/*
================
rvGEOptions::rvGEOptions
================
*/
rvGEOptions::rvGEOptions()
	: m_modified( false )
	, m_lastOptionsPage( 0 )
	, mGridColor( 0.2f, 0.2f, 1.0f, 1.0f )
	, mGridWidth( 10 )
	, mGridHeight( 10 )
	, mGridSnap( false )
	, mGridVisible( false )
	, mWorkspaceColor( 0.0f, 0.0f, 0.0f, 1.0f )
	, mSelectionColor( 0.5f, 0.5f, 1.0f, 1.0f )
	, mNavigatorVisible( true )
	, mPropertiesVisible( true )
	, mTransformerVisible( true )
	, mStatusBarVisible( true )
	, mIgnoreDesktopSelect( true )
{
	for( int i = 0; i < 16; ++i )
	{
		mCustomColors[i] = *wxWHITE;
	}
}

/*
================
rvGEOptions::Init
================
*/
void rvGEOptions::Init()
{
	rvRegistryOptions::Init( "guieditor.cfg", "GUIEditor" );
}

/*
================
rvGEOptions::Save

Writes the options to the registry so they can later be read using the Load method
================
*/
bool rvGEOptions::Save()
{
	SetLong( "lastOptionsPage", m_lastOptionsPage );

	SetVec4( "gridColor", mGridColor );
	SetLong( "gridWidth",  mGridWidth );
	SetLong( "gridHeight", mGridHeight );
	SetBool( "gridSnap",    mGridSnap );
	SetBool( "gridVisible", mGridVisible );

	SetBool( "navigatorVisible",   mNavigatorVisible );
	SetBool( "PropertiesVisible",  mPropertiesVisible );
	SetBool( "transformerVisible", mTransformerVisible );
	SetBool( "statusBarVisible",   mStatusBarVisible );

	SetVec4( "selectionColor", mSelectionColor );
	SetVec4( "workspaceColor", mWorkspaceColor );
	SetBool( "ignoreDesktopSelect", mIgnoreDesktopSelect );

	for( int i = 0; i < 16; ++i )
	{
		SetString( va( "customcol%d", i ), mCustomColors[i].GetAsString( wxC2S_HTML_SYNTAX ) );
	}

	m_modified = false;

	return rvRegistryOptions::Save();
}

/*
================
rvGEOptions::Load

Loads previsouly saved options from the registry
================
*/
bool rvGEOptions::Load()
{
	if( !rvRegistryOptions::Load() )
	{
		return false;
	}

	m_lastOptionsPage = GetLong( "lastOptionsPage", 0 );

	mGridColor   = GetVec4( "gridColor", idVec4( 0.2f, 0.2f, 1.0f, 1.0f ) );
	mGridWidth   = GetLong( "gridWidth", 10 );
	mGridHeight  = GetLong( "gridHeight", 10 );
	mGridSnap    = GetBool( "gridSnap", false );
	mGridVisible = GetBool( "gridVisible", false );

	mNavigatorVisible   = GetBool( "navigatorVisible", true );
	mPropertiesVisible  = GetBool( "PropertiesVisible", true );
	mTransformerVisible = GetBool( "transformerVisible", true );
	mStatusBarVisible   = GetBool( "statusBarVisible", true );

	mSelectionColor = GetVec4( "selectionColor", idVec4( 0.5f, 0.5f, 1.0f, 1.0f ) );
	mWorkspaceColor = GetVec4( "workspaceColor", idVec4( 0.0f, 0.0f, 0.0f, 1.0f ) );
	mIgnoreDesktopSelect = GetBool( "ignoreDesktopSelect", true );

	for( int i = 0; i < 16; ++i )
	{
		const char* s = GetString( va( "customcol%d", i ), "#FFFFFF" );
		mCustomColors[i] = wxColour( s );
	}

	return true;
}

/*
================
rvGEOptions::SetCustomColors
================
*/
void rvGEOptions::SetCustomColors( const wxColour colours[16] )
{
	for( int i = 0; i < 16; ++i )
	{
		mCustomColors[i] = colours[i];
	}
}

/*
================
rvGEOptions::ConvertColor
================
*/
void rvGEOptions::ConvertColor( const wxColour& src, idVec4& dest )
{
	dest.x = src.Red()   / 255.0f;
	dest.y = src.Green() / 255.0f;
	dest.z = src.Blue()  / 255.0f;
	dest.w = 1.0f;
}

/*
================
rvGEOptions::SnapRectToGrid

Snap the rectangle to the grid
================
*/
void rvGEOptions::SnapRectToGrid( idRectangle& rect, bool snapLeft, bool snapTop, bool snapWidth, bool snapHeight )
{
	if( snapLeft )
	{
		float offset = ( int )( rect.x + GetGridWidth() / 2 ) / GetGridWidth() * GetGridWidth();
		offset -= rect.x;
		rect.x += offset;
		rect.w -= offset;
	}

	if( snapWidth )
	{
		float offset = ( int )( rect.x + rect.w + GetGridWidth() / 2 ) / GetGridWidth() * GetGridWidth();
		offset -= rect.x;
		offset -= rect.w;
		rect.w += offset;
	}

	if( snapTop )
	{
		float offset = ( int )( rect.y + GetGridHeight() / 2 ) / GetGridHeight() * GetGridHeight();
		offset -= rect.y;
		rect.y += offset;
		rect.h -= offset;
	}

	if( snapHeight )
	{
		float offset = ( int )( rect.y + rect.h + GetGridHeight() / 2 ) / GetGridHeight() * GetGridHeight();
		offset -= rect.y;
		offset -= rect.h;
		rect.h += offset;
	}
}