/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015-2021 Robert Beckebans

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

#include "Game_local.h"

/*
===============================================================================

  Environment probe for Image Based Lighting (part of PBR).

===============================================================================
*/

const idEventDef EV_Envprobe_On( "On", NULL );
const idEventDef EV_Envprobe_Off( "Off", NULL );

CLASS_DECLARATION( idEntity, idEnvProbes )
EVENT( EV_Hide,							idEnvProbes::Event_Hide )
EVENT( EV_Show,							idEnvProbes::Event_Show )
END_CLASS


/*
================
idGameEdit::ParseSpawnArgsToRenderLight

parse the light parameters
this is the canonical renderLight parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRenderEnvprobe( const idDict* args, renderEnvironmentProbe_t* renderEnvprobe )
{
	idVec3	color;

	memset( renderEnvprobe, 0, sizeof( *renderEnvprobe ) );

	args->GetVector( "origin", "", renderEnvprobe->origin );
}

/*
================
idEnvProbes::UpdateChangeableSpawnArgs
================
*/
void idEnvProbes::UpdateChangeableSpawnArgs( const idDict* source )
{
	idEntity::UpdateChangeableSpawnArgs( source );

	if( source )
	{
		source->Print();
	}

	gameEdit->ParseSpawnArgsToRenderEnvprobe( source ? source : &spawnArgs, &renderEnvprobe );

	UpdateVisuals();
}

/*
================
idEnvProbes::idEnvProbes
================
*/
idEnvProbes::idEnvProbes()
{
	memset( &renderEnvprobe, 0, sizeof( renderEnvprobe ) );
	localEnvprobeOrigin	= vec3_zero;
	localEnvprobeAxis	= mat3_identity;
	envprobeDefHandle	= -1;
}

/*
================
idEnvProbes::~idEnvProbes
================
*/
idEnvProbes::~idEnvProbes()
{
	if( envprobeDefHandle != -1 )
	{
		gameRenderWorld->FreeEnvprobeDef( envprobeDefHandle );
	}
}

/*
================
idEnvProbes::Save

archives object for save game file
================
*/
void idEnvProbes::Save( idSaveGame* savefile ) const
{
	savefile->WriteRenderEnvprobe( renderEnvprobe );

	savefile->WriteVec3( localEnvprobeOrigin );
	savefile->WriteMat3( localEnvprobeAxis );
}

/*
================
idEnvProbes::Restore

unarchives object from save game file
================
*/
void idEnvProbes::Restore( idRestoreGame* savefile )
{
	savefile->ReadRenderEnvprobe( renderEnvprobe );

	savefile->ReadVec3( localEnvprobeOrigin );
	savefile->ReadMat3( localEnvprobeAxis );

	envprobeDefHandle = -1;
}

/*
================
idEnvProbes::Spawn
================
*/
void idEnvProbes::Spawn()
{
	// do the parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRenderEnvprobe( &spawnArgs, &renderEnvprobe );

	// we need the origin and axis relative to the physics origin/axis
	localEnvprobeOrigin = ( renderEnvprobe.origin - GetPhysics()->GetOrigin() ) * GetPhysics()->GetAxis().Transpose();
	localEnvprobeAxis = GetPhysics()->GetAxis().Transpose();

	// game specific functionality, not mirrored in
	// editor or dmap light parsing

	envprobeDefHandle = -1;		// no static version yet

	PostEventMS( &EV_PostSpawn, 0 );

	UpdateVisuals();
}

/*
================
idEnvProbes::PresentEnvprobeDefChange
================
*/
void idEnvProbes::PresentEnvprobeDefChange()
{
	// let the renderer apply it to the world
	if( ( envprobeDefHandle != -1 ) )
	{
		gameRenderWorld->UpdateEnvprobeDef( envprobeDefHandle, &renderEnvprobe );
	}
	else
	{
		envprobeDefHandle = gameRenderWorld->AddEnvprobeDef( &renderEnvprobe );
	}
}

/*
================
idEnvProbes::Present
================
*/
void idEnvProbes::Present()
{
	// don't present to the renderer if the entity hasn't changed
	if( !( thinkFlags & TH_UPDATEVISUALS ) )
	{
		return;
	}

	// add the model
	idEntity::Present();

	// current transformation
	renderEnvprobe.origin  = GetPhysics()->GetOrigin() + GetPhysics()->GetAxis() * localEnvprobeOrigin;

	// update the renderLight and renderEntity to render the light and flare
	PresentEnvprobeDefChange();
}

/*
================
idEnvProbes::Think
================
*/
void idEnvProbes::Think()
{
	RunPhysics();
	Present();
}

/*
================
idEnvProbes::FreeEnvprobeDef
================
*/
void idEnvProbes::FreeEnvprobeDef()
{
	if( envprobeDefHandle != -1 )
	{
		gameRenderWorld->FreeEnvprobeDef( envprobeDefHandle );
		envprobeDefHandle = -1;
	}
}

/*
================
idEnvProbes::SaveState
================
*/
void idEnvProbes::SaveState( idDict* args )
{
	int i, c = spawnArgs.GetNumKeyVals();
	for( i = 0; i < c; i++ )
	{
		const idKeyValue* pv = spawnArgs.GetKeyVal( i );
		if( pv->GetKey().Find( "editor_", false ) >= 0 || pv->GetKey().Find( "parse_", false ) >= 0 )
		{
			continue;
		}
		args->Set( pv->GetKey(), pv->GetValue() );
	}
}

/*
===============
idEnvProbes::ShowEditingDialog
===============
*/
void idEnvProbes::ShowEditingDialog()
{
}

/*
================
idEnvProbes::Event_Hide
================
*/
void idEnvProbes::Event_Hide()
{
	Hide();
}

/*
================
idEnvProbes::Event_Show
================
*/
void idEnvProbes::Event_Show()
{
	Show();
}

/*
================
idEnvProbes::ClientPredictionThink
================
*/
void idEnvProbes::ClientPredictionThink()
{
	Think();
}

/*
================
idEnvProbes::WriteToSnapshot
================
*/
void idEnvProbes::WriteToSnapshot( idBitMsg& msg ) const
{
	GetPhysics()->WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );
}

/*
================
idEnvProbes::ReadFromSnapshot
================
*/
void idEnvProbes::ReadFromSnapshot( const idBitMsg& msg )
{
	GetPhysics()->ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );

	if( msg.HasChanged() )
	{
		PresentEnvprobeDefChange();
	}
}
