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

rvGEApp* gApp = NULL;

static bool s_guiedActive = false;

/*
================
rvGEWxApp
================
*/
class rvGEWxApp : public wxApp
{
public:
	bool OnInit() override
	{
		return wxApp::OnInit();
	}
};

/*
================
GUIEditorInit

Initializes the gui editor tool.
================
*/
void GUIEditorInit()
{
	if( s_guiedActive )
	{
		return;
	}

	com_editors = EDITOR_GUI;

	common->ActivateTool( true );

	if( !wxTheApp )
	{
		wxApp::SetInstance( new rvGEWxApp() );
	}

	if( !wxInitialize() )
	{
		common->Error( "GUIEditorInit: wxInitialize failed" );
		common->ActivateTool( false );
		return;
	}

	wxInitAllImageHandlers();

	rRenderContext.MakeCurrent();

	gApp = new rvGEApp();
	if( !gApp->Initialize() )
	{
		delete gApp;
		gApp = NULL;

		rRenderContext.Disable();
		wxUninitialize();
		common->ActivateTool( false );
		return;
	}

	rRenderContext.Disable();

	s_guiedActive = true;

	// Hide the doom window by default.
	Sys_ShowWindow( false );
}

/*
================
GUIEditorShutdown

Called from the engine's tool dispatch when the editor is closed.
================
*/
void GUIEditorShutdown()
{
	if( !s_guiedActive )
	{
		return;
	}
	s_guiedActive = false;

	if( gApp && !gApp->IsClosing() )
	{
		gApp->Shutdown();

		delete gApp;
		gApp = NULL;
	}

	wxUninitialize();

	com_editors &= ~EDITOR_GUI;
	common->ActivateTool( false );
}

/*
================
GUIEditorRun

Called every frame by the doom engine to allow the gui editor to process messages.
================
*/
void GUIEditorRun()
{
	if( !wxTheApp || !s_guiedActive )
	{
		return;
	}

	if( !gApp )
	{
		GUIEditorShutdown();
		common->Quit();
		return;
	}

	if( gApp->IsClosing() )
	{
		return;
	}

	while( wxTheApp->Pending() )
	{
		wxTheApp->Dispatch();
	}

	gApp->RunFrame();

	Sys_Sleep( 1 );
}

/*
================
StringFromVec4

Returns a clean string version of the given vec4
================
*/
const char* StringFromVec4( idVec4& v )
{
	return va( "%s,%s,%s,%s",
			   idStr::FloatArrayToString( &v[0], 1, 8 ),
			   idStr::FloatArrayToString( &v[1], 1, 8 ),
			   idStr::FloatArrayToString( &v[2], 1, 8 ),
			   idStr::FloatArrayToString( &v[3], 1, 8 ) );
}

/*
================
IsExpression

Returns true if the given string is an expression
================
*/
bool IsExpression( const char* s )
{
	idParser src( s, strlen( s ), "",
				  LEXFL_ALLOWMULTICHARLITERALS		|
				  LEXFL_NOSTRINGCONCAT				|
				  LEXFL_ALLOWBACKSLASHSTRINGCONCAT	|
				  LEXFL_NOFATALERRORS );

	idToken token;
	bool	needComma = false;
	bool	needNumber = false;
	while( src.ReadToken( &token ) )
	{
		switch( token.type )
		{
			case TT_NUMBER:
				needComma = true;
				needNumber = false;
				break;

			case TT_PUNCTUATION:
				if( needNumber )
				{
					return true;
				}
				if( token[0] == ',' )
				{
					if( !needComma )
					{
						return true;
					}

					needComma = false;
					break;
				}

				if( needComma )
				{
					return true;
				}

				if( token[0] == '-' )
				{
					needNumber = true;
				}
				break;

			default:
				return true;
		}
	}

	return false;
}
