/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson
Copyright (C) 2020-2023 Robert Beckebans

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

#include "ImGuiTools_local.h"

extern idCVar g_editEntityMode;

idImGuiEditorLocal::idImGuiEditorLocal( idImGuiSystem* system_ )
	: system( system_ ), releaseMouse( false ), rightMouseActive( false )
{
}

idImGuiEditorLocal::~idImGuiEditorLocal()
{
}

void idImGuiEditorLocal::RegisterWindow( idImGuiWindow& window )
{
	for( int i = 0; i < windows.Num(); ++i )
	{
		if( windows[i] == &window )
		{
			return;
		}
	}

	windows.Append( &window );
	system->RegisterDockWindow( window.GetWindowName(), window.GetDockRegion() );
}

void idImGuiEditorLocal::ReleaseMouse( bool doRelease )
{
	releaseMouse = doRelease;
}

void idImGuiEditorLocal::SetRightMouseActive( bool active )
{
	rightMouseActive = active;
}

bool idImGuiEditorLocal::AreEditorsActive() const
{
	return cvarSystem->GetCVarInteger( "g_editEntityMode" ) > 0 || com_editors != 0;
}

bool idImGuiEditorLocal::IsMouseRelease() const
{
	return AreEditorsActive() && releaseMouse && !rightMouseActive;
}

bool idImGuiEditorLocal::IsFreeCameraActive() const
{
	for( int i = 0; i < windows.Num(); ++i )
	{
		if( windows[i]->IsShown() && windows[i]->IsFreeCameraActive() )
		{
			return true;
		}
	}
	return false;
}

void idImGuiEditorLocal::DrawWindows()
{
	for( int i = 0; i < windows.Num(); ++i )
	{
		if( windows[i]->IsShown() )
		{
			windows[i]->Draw();
		}
	}
}

void idImGuiEditorLocal::InitTool( const toolFlag_t tool, const idDict* dict, idEntity* entity )
{
	if( tool & EDITOR_SOUND )
	{
		SoundEditorInit( dict, entity );
	}
	else if( tool & EDITOR_LIGHT )
	{
		LightEditorInit( dict, entity );
	}
	else if( tool & EDITOR_PARTICLE )
	{
		ParticleEditorInit( dict, entity );
	}
	else if( tool & EDITOR_AF )
	{
		AFEditorInit(); // TODO: dict ?
	}
	else if( tool & EDITOR_PDA )
	{
		//PDAEditorInit( dict );
	}
	else if( tool & EDITOR_SCRIPT )
	{
		ScriptEditorInit( dict );
	}
	else if( tool & EDITOR_DECL )
	{
		DeclBrowserInit();
	}
}