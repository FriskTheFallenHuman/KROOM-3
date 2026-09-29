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

#include "../../renderer/RenderContext.h"

#include "MaterialEditor.h"
#include "MEMainFrame.h"
#include "MaterialPreviewView.h"

MEMainFrame* meMainFrame = NULL;

static bool s_meActive = false;

/*
============================================

MaterialEditorApp

============================================
*/

/*
================
MaterialEditorApp::OnInit
================
*/
bool MaterialEditorApp::OnInit()
{
	if( !wxApp::OnInit() )
	{
		return false;
	}

	return true;
}

/*
============================================

MaterialEditor

============================================
*/

/*
================
GetMaterialEditorWindow
================
*/
wxWindow* GetMaterialEditorWindow()
{
	if( meMainFrame )
	{
		return meMainFrame;
	}
	return NULL;
}

/*
================
MaterialEditorInit

Initializes the material editor tool.
================
*/
void MaterialEditorInit()
{
	if( s_meActive )
	{
		return;
	}

	com_editors = EDITOR_MATERIAL;

	common->ActivateTool( true );

	if( !wxTheApp )
	{
		wxApp::SetInstance( new MaterialEditorApp() );
	}

	if( !wxInitialize() )
	{
		common->Error( "MaterialEditorInit: wxInitialize failed" );
		common->ActivateTool( false );
		return;
	}

	wxInitAllImageHandlers();

	rRenderContext.MakeCurrent();

	// Create the top-level window.
	meMainFrame = new MEMainFrame();
	meMainFrame->Show();

	rRenderContext.Disable();

	s_meActive = true;

	// hide the doom window by default
	Sys_ShowWindow( false );
}

/*
================
MaterialEditorRun

Called every frame by the doom engine to allow the material editor to process messages.
================
*/
void MaterialEditorRun()
{
	if( !wxTheApp || !s_meActive )
	{
		return;
	}

	if( !meMainFrame )
	{
		MaterialEditorShutdown();
		common->Quit();
		return;
	}

	while( wxTheApp->Pending() )
	{
		wxTheApp->Dispatch();
	}

	Sys_Sleep( 1 );
}

/*
================
MaterialEditorShutdown

Called from the engine's tool dispatch when the editor is closed.
================
*/
void MaterialEditorShutdown()
{
	if( !s_meActive )
	{
		return;
	}
	s_meActive = false;

	if( meMainFrame )
	{
		meMainFrame->Destroy();
		meMainFrame = NULL;
	}

	wxUninitialize();

	com_editors &= ~EDITOR_MATERIAL;
	common->ActivateTool( false );
}

/*
================
MaterialEditorPrintConsole
================
*/
void MaterialEditorPrintConsole( const char* msg )
{
	//meMainFrame can be null when starting immedeatly from commandline.
	if( meMainFrame && ( com_editors & EDITOR_MATERIAL ) )
	{
		meMainFrame->PrintConsoleMessage( msg );
	}
}