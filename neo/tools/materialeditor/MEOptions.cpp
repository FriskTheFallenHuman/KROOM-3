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

#include "MEOptions.h"

/*
================
MEOptions::MEOptions
================
*/
MEOptions::MEOptions()
	: modified( false )
	, materialTreeWidth( 300 )
	, stageWidth( 200 )
	, previewPropertiesWidth( 300 )
	, materialEditHeight( 300 )
	, materialPropHeadingWidth( 200 )
	, previewPropHeadingWidth( 120 )
{

	Init( "materialeditor.cfg", "MaterialEditor" );
}

/*
================
MEOptions::~MEOptions
================
*/
MEOptions::~MEOptions()
{
}

/*
================
MEOptions::Save

Saves the material editor options to the registry.
================
*/
bool MEOptions::Save()
{
	SetLong( "materialTreeWidth", materialTreeWidth );
	SetLong( "stageWidth", stageWidth );
	SetLong( "previewPropertiesWidth", previewPropertiesWidth );
	SetLong( "materialEditHeight", materialEditHeight );
	SetLong( "materialPropHeadingWidth", materialPropHeadingWidth );
	SetLong( "previewPropHeadingWidth", previewPropHeadingWidth );

	return rvRegistryOptions::Save();
}

/*
================
MEOptions::Load

Loads the material editor options from the registry.
================
*/
bool MEOptions::Load()
{
	if( !rvRegistryOptions::Load() )
	{
		return false;
	}

	materialTreeWidth = ( int )GetLong( "materialTreeWidth", 300 );
	stageWidth = ( int )GetLong( "stageWidth", 200 );
	previewPropertiesWidth = ( int )GetLong( "previewPropertiesWidth", 300 );
	materialEditHeight = ( int )GetLong( "materialEditHeight", 300 );
	materialPropHeadingWidth = ( int )GetLong( "materialPropHeadingWidth", 200 );
	previewPropHeadingWidth = ( int )GetLong( "previewPropHeadingWidth",  120 );

	modified = false;

	return true;

}