/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "GEApp.h"

rvGEStatusBar::rvGEStatusBar( )
{
	mSimple = true;
	mZoom   = 0;
	mTriangles = 0;
}

/*
================
rvGEStatusBar::Create

Creates a new status bar
================
*/
bool rvGEStatusBar::Create( HWND parent, UINT id, bool visible )
{
	mWnd = CreateStatusWindow( WS_CHILD | WS_VISIBLE | WS_BORDER, "", parent, id );

	if( !mWnd )
	{
		return false;
	}

	Show( visible );

	return true;
}

/*
================
rvGEStatusBar::Resize

Resizes the status bar and updates the content
================
*/
void rvGEStatusBar::Resize( int width, int height )
{
	SendMessage( mWnd, WM_SIZE, 0, MAKELONG( width, height ) );

	Update( );
}

/*
================
rvGEStatusBar::Update

Updates the status bar by setting up each part's width and text
================
*/
void rvGEStatusBar::Update()
{
	RECT	rStatus;
	SIZE	zoomSize;
	SIZE	trisSize;
	int		parts[5];

	GetWindowRect( mWnd, &rStatus );

	if( mSimple )
	{
		parts[0] = -1;

		SendMessage( mWnd, SB_SETPARTS, 1, ( LONG_PTR )parts );
		SendMessage( mWnd, SB_SETTEXT, 1, ( LPARAM ) "" );
	}
	else
	{
		zoomSize.cx = 85;
		trisSize.cx = 65;

		parts[0] = ( rStatus.right - rStatus.left ) - zoomSize.cx - trisSize.cx - 40;
		parts[1] = parts[0] + trisSize.cx;
		parts[2] = parts[1] + zoomSize.cx;
		parts[3] = parts[2] + 40;
		parts[4] = -1;

		SendMessage( mWnd, SB_SETPARTS, 5, ( LONG_PTR )parts );
		SendMessage( mWnd, SB_SETTEXT, 0, ( LPARAM ) "" );
		SendMessage( mWnd, SB_SETTEXT, 1, ( LPARAM ) va( " Tris: %d", mTriangles ) );
		SendMessage( mWnd, SB_SETTEXT, 2, ( LPARAM ) va( " Zoom: %d%%", mZoom ) );
	}
}

/*
================
rvGEStatusBar::Show

Shows and hides the status bar
================
*/
void rvGEStatusBar::Show( bool visible )
{
	gApp.GetOptions().SetStatusBarVisible( visible );
	ShowWindow( mWnd, visible ? SW_SHOW : SW_HIDE );
}
