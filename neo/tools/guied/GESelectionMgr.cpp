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

#include "../../renderer/RenderCommon.h"
#include "../../sys/win32/win_local.h"

#include "GEApp.h"
#include "GESelectionMgr.h"
#include "GEItemScriptsDlg.h"

#define GUIED_GRABSIZE		7
#define GUIED_CENTERSIZE	5

#ifndef NOMINMAX

	#ifndef max
		#define max(a,b)            (((a) > (b)) ? (a) : (b))
	#endif

	#ifndef min
		#define min(a,b)            (((a) < (b)) ? (a) : (b))
	#endif

#endif  /* NOMINMAX */

rvGESelectionMgr::rvGESelectionMgr( )
{
	mWorkspace = NULL;
}

/*
================
rvGESelectionMgr::SetSelection

Sets the only selection for the workspace to the given window
================
*/
void rvGESelectionMgr::Set( idWindow* window )
{
	// Get rid of any current selections
	Clear( );

	// Add this selection now
	return Add( window );
}

/*
================
rvGESelectionMgr::Add

Adds the given window to the selection list
================
*/
void rvGESelectionMgr::Add( idWindow* window, bool expand )
{
	rvGEWindowWrapper* wrapper;

	wrapper = rvGEWindowWrapper::GetWrapper( window );
	assert( wrapper );

	// If the window is already selected then dont add the selection
	if( wrapper->IsSelected( ) )
	{
		return;
	}

	wrapper->SetSelected( true );

	mSelections.Append( window );

	if( expand && wrapper->Expand( ) )
	{
		gApp.GetNavigator( ).Update( );
	}

	gApp.GetNavigator( ).UpdateSelections( );
	gApp.GetNavigator( ).Refresh( );
	gApp.GetTransformer( ).Update( );
	gApp.GetProperties().Update( );
	gApp.GetItemProperties().Update( );
	GEItescriptsDlg_Init( gApp.GetScriptWindow() );

	UpdateExpression( );
}

/*
================
rvGESelectionMgr::RemoveSelection

Removes the selection from the current workspace
================
*/
void rvGESelectionMgr::Remove( idWindow* window )
{
	rvGEWindowWrapper* wrapper;

	wrapper = rvGEWindowWrapper::GetWrapper( window );
	assert( wrapper );

	// Dont bother if the window isnt selectd already
	if( !wrapper->IsSelected( ) )
	{
		return;
	}

	GEItescriptsDlg_Apply( gApp.GetScriptWindow() );

	wrapper->SetSelected( false );

	mSelections.Remove( window );

	gApp.GetNavigator( ).UpdateSelections( );
	gApp.GetNavigator( ).Refresh( );
	gApp.GetTransformer( ).Update( );
	gApp.GetProperties().Update( );
	gApp.GetItemProperties().Update( );
	GEItescriptsDlg_Init( gApp.GetScriptWindow() );

	UpdateExpression( );
}

/*
================
rvGESelectionMgr::ClearSelections

Remove all of the current selections
================
*/
void rvGESelectionMgr::Clear()
{
	int i;

	if( mSelections.Num() > 0 )
	{
		GEItescriptsDlg_Apply( gApp.GetScriptWindow() );
	}

	for( i = 0; i < mSelections.Num( ); i ++ )
	{
		rvGEWindowWrapper::GetWrapper( mSelections[i] )->SetSelected( false );
	}

	mSelections.Clear( );

	gApp.GetNavigator( ).UpdateSelections( );
	gApp.GetNavigator( ).Refresh( );
	gApp.GetTransformer( ).Update( );
	gApp.GetProperties().Update( );
	gApp.GetItemProperties().Update( );
	GEItescriptsDlg_Init( gApp.GetScriptWindow() );

	mExpression = false;
}

/*
================
rvGESelectionMgr::Render

Render the selections including the move/size bars
================
*/
void rvGESelectionMgr::Render()
{
	if( !mSelections.Num( ) )
	{
		return;
	}

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	UpdateRectangle( );

	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	idVec4&	color = gApp.GetOptions().GetSelectionColor( );
	glColor4f( color[0], color[1], color[2], 1.0f );

	glBegin( GL_LINE_LOOP );
	glVertex2f( mRect.x, mRect.y );
	glVertex2f( mRect.x + mRect.w, mRect.y );
	glVertex2f( mRect.x + mRect.w, mRect.y + mRect.h );
	glVertex2f( mRect.x, mRect.y + mRect.h );
	glEnd( );

	glColor4f( color[0], color[1], color[2], 0.75f );

	int i;
	for( i = 0; i < mSelections.Num(); i ++ )
	{
		rvGEWindowWrapper*	wrapper;
		idRectangle			rect;

		wrapper = rvGEWindowWrapper::GetWrapper( mSelections[i] );
		assert( wrapper );

		rect = wrapper->GetScreenRect( );
		mWorkspace->WorkspaceToWindow( rect );

		if( i == 0 )
		{
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			glBegin( GL_TRIANGLES );
			glVertex2f( rect.x, rect.y );
			glVertex2f( rect.x + GUIED_GRABSIZE, rect.y );
			glVertex2f( rect.x, rect.y + GUIED_GRABSIZE );
			glEnd( );
		}

		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glBegin( GL_LINE_LOOP );
		glVertex2f( rect.x, rect.y );
		glVertex2f( rect.x + rect.w, rect.y );
		glVertex2f( rect.x + rect.w, rect.y + rect.h );
		glVertex2f( rect.x, rect.y + rect.h );
		glEnd( );

		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glBegin( GL_QUADS );
		glVertex2f( rect.x + ( rect.w - GUIED_CENTERSIZE ) / 2, rect.y + ( rect.h - GUIED_CENTERSIZE ) / 2 );
		glVertex2f( rect.x + ( rect.w + GUIED_CENTERSIZE ) / 2, rect.y + ( rect.h - GUIED_CENTERSIZE ) / 2 );
		glVertex2f( rect.x + ( rect.w + GUIED_CENTERSIZE ) / 2, rect.y + ( rect.h + GUIED_CENTERSIZE ) / 2 );
		glVertex2f( rect.x + ( rect.w - GUIED_CENTERSIZE ) / 2, rect.y + ( rect.h + GUIED_CENTERSIZE ) / 2 );
		glEnd( );
	}

	if( mExpression )
	{
		return;
	}

	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	glColor4f( color[0], color[1], color[2], 1.0f );
	glBegin( GL_QUADS );

	// Top Left
	glVertex2f( mRect.x - GUIED_GRABSIZE, mRect.y - GUIED_GRABSIZE );
	glVertex2f( mRect.x - 1, mRect.y - GUIED_GRABSIZE );
	glVertex2f( mRect.x - 1, mRect.y - 1 );
	glVertex2f( mRect.x - GUIED_GRABSIZE, mRect.y - 1 );

	// Left
	glVertex2f( mRect.x - GUIED_GRABSIZE, mRect.y + mRect.h / 2 - GUIED_GRABSIZE / 2 );
	glVertex2f( mRect.x - 1, mRect.y + mRect.h / 2 - GUIED_GRABSIZE / 2 );
	glVertex2f( mRect.x - 1, mRect.y + mRect.h / 2 + GUIED_GRABSIZE / 2 );
	glVertex2f( mRect.x - GUIED_GRABSIZE, mRect.y + mRect.h / 2 + GUIED_GRABSIZE / 2 );

	// Bototm Left
	glVertex2f( mRect.x - GUIED_GRABSIZE, mRect.y + mRect.h + 1 );
	glVertex2f( mRect.x - 1, mRect.y + mRect.h + 1 );
	glVertex2f( mRect.x - 1, mRect.y + mRect.h + GUIED_GRABSIZE );
	glVertex2f( mRect.x - GUIED_GRABSIZE, mRect.y + mRect.h + GUIED_GRABSIZE );

	// Bottom
	glVertex2f( mRect.x - GUIED_GRABSIZE / 2 + mRect.w / 2, mRect.y + mRect.h + 1 );
	glVertex2f( mRect.x + GUIED_GRABSIZE / 2 + mRect.w / 2, mRect.y + mRect.h + 1 );
	glVertex2f( mRect.x + GUIED_GRABSIZE / 2 + mRect.w / 2, mRect.y + mRect.h + GUIED_GRABSIZE );
	glVertex2f( mRect.x - GUIED_GRABSIZE / 2 + mRect.w / 2, mRect.y + mRect.h + GUIED_GRABSIZE );

	// Bottom Right
	glVertex2f( mRect.x + mRect.w + 1, mRect.y + mRect.h + 1 );
	glVertex2f( mRect.x + mRect.w + GUIED_GRABSIZE, mRect.y + mRect.h + 1 );
	glVertex2f( mRect.x + mRect.w + GUIED_GRABSIZE, mRect.y + mRect.h + GUIED_GRABSIZE );
	glVertex2f( mRect.x + mRect.w + 1, mRect.y + mRect.h + GUIED_GRABSIZE );

	// Right
	glVertex2f( mRect.x + mRect.w + 1, mRect.y + mRect.h / 2 - GUIED_GRABSIZE / 2 );
	glVertex2f( mRect.x + mRect.w + GUIED_GRABSIZE, mRect.y + mRect.h / 2 - GUIED_GRABSIZE / 2 );
	glVertex2f( mRect.x + mRect.w + GUIED_GRABSIZE, mRect.y + mRect.h / 2 + GUIED_GRABSIZE / 2 );
	glVertex2f( mRect.x + mRect.w + 1, mRect.y + mRect.h / 2 + GUIED_GRABSIZE / 2 );

	// Top Right
	glVertex2f( mRect.x + mRect.w + 1, mRect.y - GUIED_GRABSIZE );
	glVertex2f( mRect.x + mRect.w + GUIED_GRABSIZE, mRect.y - GUIED_GRABSIZE );
	glVertex2f( mRect.x + mRect.w + GUIED_GRABSIZE, mRect.y - 1 );
	glVertex2f( mRect.x + mRect.w + 1, mRect.y - 1 );

	// Top
	glVertex2f( mRect.x - GUIED_GRABSIZE / 2 + mRect.w / 2, mRect.y - GUIED_GRABSIZE );
	glVertex2f( mRect.x + GUIED_GRABSIZE / 2 + mRect.w / 2, mRect.y - GUIED_GRABSIZE );
	glVertex2f( mRect.x + GUIED_GRABSIZE / 2 + mRect.w / 2, mRect.y - 1 );
	glVertex2f( mRect.x - GUIED_GRABSIZE / 2 + mRect.w / 2, mRect.y - 1 );

	glEnd( );

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

/*
================
rvGESelectionMgr::UpdateRectangle

Update the selection rectangle from all the currently selected items.
================
*/
void rvGESelectionMgr::UpdateRectangle()
{
	int		i;
	idVec2	point;

	assert( mWorkspace );

	if( mSelections.Num( ) <= 0 )
	{
		return;
	}

	// Start with the first selections retangle
	mRect = rvGEWindowWrapper::GetWrapper( mSelections[0] )->GetScreenRect( );

	// Its easier to do the calculates with it being top left and bottom right
	// so temporarly convert width and height to right and bottom
	mRect.w += mRect.x;
	mRect.h += mRect.y;

	// Merge all the rest of the rectangles to make the actual selection rectangle
	for( i = 1; i < mSelections.Num(); i ++ )
	{
		idRectangle selRect;
		selRect = rvGEWindowWrapper::GetWrapper( mSelections[i] )->GetScreenRect( );

		mRect.w = Max( selRect.x + selRect.w, mRect.w );
		mRect.h = Max( selRect.y + selRect.h, mRect.h );
		mRect.x = Min( selRect.x, mRect.x );
		mRect.y = Min( selRect.y, mRect.y );
	}

	mRect.w -= mRect.x;
	mRect.h -= mRect.y;

	mWorkspace->WorkspaceToWindow( mRect );
}

/*
================
rvGESelectionMgr::UpdateExpression

Update whether or not the selection has an expression in it
================
*/
void rvGESelectionMgr::UpdateExpression()
{
	int i;

	mExpression = false;
	for( i = 0; i < mSelections.Num(); i ++ )
	{
		rvGEWindowWrapper* wrapper;
		wrapper = rvGEWindowWrapper::GetWrapper( mSelections[i] );
		if( wrapper && !wrapper->CanMoveAndSize( ) )
		{
			mExpression = true;
			break;
		}
	}
}

/*
================
rvGESelectionMgr::HitTest

Test to see if the given coordinate is within the selection rectangle and if it is
see what its over.
================
*/
rvGESelectionMgr::EHitTest rvGESelectionMgr::HitTest( float x, float y )
{
	if( !mSelections.Num( ) )
	{
		return HT_NONE;
	}

	UpdateRectangle( );

	// Inside the rectangle is moving
	if( mRect.Contains( x, y ) )
	{
		return mExpression ? HT_SELECT : HT_MOVE;
	}

	if( mExpression )
	{
		return HT_NONE;
	}

	// Check for top left sizing
	if( idRectangle( mRect.x - GUIED_GRABSIZE, mRect.y - GUIED_GRABSIZE, GUIED_GRABSIZE, GUIED_GRABSIZE ).Contains( x, y ) )
	{
		return HT_SIZE_TOPLEFT;
	}

	// Check for left sizing
	if( idRectangle( mRect.x - GUIED_GRABSIZE, mRect.y + mRect.h / 2 - GUIED_GRABSIZE / 2, GUIED_GRABSIZE, GUIED_GRABSIZE ).Contains( x, y ) )
	{
		return HT_SIZE_LEFT;
	}

	// Check for bottom left sizing
	if( idRectangle( mRect.x - GUIED_GRABSIZE, mRect.y + mRect.h, GUIED_GRABSIZE, GUIED_GRABSIZE ).Contains( x, y ) )
	{
		return HT_SIZE_BOTTOMLEFT;
	}

	// Check for bottom sizing
	if( idRectangle( mRect.x + mRect.w / 2 - GUIED_GRABSIZE / 2, mRect.y + mRect.h, GUIED_GRABSIZE, GUIED_GRABSIZE ).Contains( x, y ) )
	{
		return HT_SIZE_BOTTOM;
	}

	// Check for bottom right sizing
	if( idRectangle( mRect.x + mRect.w, mRect.y + mRect.h, GUIED_GRABSIZE, GUIED_GRABSIZE ).Contains( x, y ) )
	{
		return HT_SIZE_BOTTOMRIGHT;
	}

	// Check for right sizing
	if( idRectangle( mRect.x + mRect.w, mRect.y + mRect.h / 2 - GUIED_GRABSIZE / 2, GUIED_GRABSIZE, GUIED_GRABSIZE ).Contains( x, y ) )
	{
		return HT_SIZE_RIGHT;
	}

	// Check for top right sizing
	if( idRectangle( mRect.x + mRect.w, mRect.y - GUIED_GRABSIZE, GUIED_GRABSIZE, GUIED_GRABSIZE ).Contains( x, y ) )
	{
		return HT_SIZE_TOPRIGHT;
	}

	// Check for top sizing
	if( idRectangle( mRect.x + mRect.w / 2 - GUIED_GRABSIZE / 2, mRect.y - GUIED_GRABSIZE, GUIED_GRABSIZE, GUIED_GRABSIZE ).Contains( x, y ) )
	{
		return HT_SIZE_TOP;
	}

	return HT_NONE;
}

/*
================
rvGESelectionMgr::GetBottomMost

Returns the bottom most selected window.
================
*/
idWindow* rvGESelectionMgr::GetBottomMost()
{
	idWindow*	bottom;
	int			depth;
	int			i;

	depth  = 9999;
	bottom = NULL;

	// Loop through all the selections and find the bottom most window
	for( i = 0; i < mSelections.Num(); i ++ )
	{
		idWindow* parent;
		int		  tempDepth;

		// Calculate the depth of the window by iterating back through the windows parents
		for( tempDepth = 0, parent = mSelections[i]; parent; parent = parent->GetParent( ), tempDepth++ );

		// If the new depth is less than the current depth then this window is below
		if( tempDepth < depth )
		{
			depth  = tempDepth;
			bottom = mSelections[i];
		}
	}

	return bottom;
}
