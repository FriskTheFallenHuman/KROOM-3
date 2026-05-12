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

  Tonemap controller.

===============================================================================
*/

const idEventDef EV_Tonemap_Enable( "enable", NULL );
const idEventDef EV_Tonemap_Disable( "disable", NULL );

CLASS_DECLARATION( idEntity, idTonemapController )
EVENT( EV_Tonemap_Enable,   idTonemapController::Event_Enable )
EVENT( EV_Tonemap_Disable, idTonemapController::Event_Disable )
END_CLASS


tonemapState_t idTonemapController::tonemapCurrent;
tonemapState_t idTonemapController::tonemapTarget;
float    idTonemapController::tonemapBlendAlpha = 1.0f;
float    idTonemapController::tonemapBlendSpeed = 1.0f;

/*
================
idTonemapController::idTonemapController
================
*/
idTonemapController::idTonemapController()
{
}

/*
================
idTonemapController::~idTonemapController
================
*/
idTonemapController::~idTonemapController()
{
}

/*
================
idTonemapController::Spawn
================
*/
void idTonemapController::Spawn()
{
	activateRadius = spawnArgs.GetFloat( "radius", "256" );
	blendTime = spawnArgs.GetFloat( "blend_time", "2.0" );
	preset = spawnArgs.GetInt( "preset", "0" );  // 0=ACES
	exposure = spawnArgs.GetFloat( "exposure", "0.5" );
	saturation = spawnArgs.GetFloat( "saturation", "1.0" );
	contrast = spawnArgs.GetFloat( "contrast", "1.0" );
	hdrKey = spawnArgs.GetFloat( "hdr_key", "0.015" );

	active = false;
	blendAlpha = 0.0f;
	BecomeActive( TH_THINK );
}

/*
================
idTonemapController::Save
================
*/
void idTonemapController::Save( idSaveGame* savefile ) const
{
	savefile->WriteFloat( activateRadius );
	savefile->WriteFloat( blendTime );
	savefile->WriteFloat( blendAlpha );
	savefile->WriteInt( preset );
	savefile->WriteFloat( exposure );
	savefile->WriteFloat( saturation );
	savefile->WriteFloat( contrast );
	savefile->WriteFloat( hdrKey );
	savefile->WriteInt( prevPreset );
	savefile->WriteFloat( prevExposure );
	savefile->WriteFloat( prevSaturation );
	savefile->WriteFloat( prevContrast );
	savefile->WriteFloat( prevKey );
	savefile->WriteBool( active );
}

/*
================
idTonemapController::Restore
================
*/
void idTonemapController::Restore( idRestoreGame* savefile )
{
	savefile->ReadFloat( activateRadius );
	savefile->ReadFloat( blendTime );
	savefile->ReadFloat( blendAlpha );
	savefile->ReadInt( preset );
	savefile->ReadFloat( exposure );
	savefile->ReadFloat( saturation );
	savefile->ReadFloat( contrast );
	savefile->ReadFloat( hdrKey );
	savefile->ReadInt( prevPreset );
	savefile->ReadFloat( prevExposure );
	savefile->ReadFloat( prevSaturation );
	savefile->ReadFloat( prevContrast );
	savefile->ReadFloat( prevKey );
	savefile->ReadBool( active );
}

/*
================
idTonemapController::Think
================
*/
void idTonemapController::Think()
{
	float dt = MS2SEC( gameLocal.time );

	// Proximity-based auto activation (when not triggered manually)
	if( spawnArgs.GetBool( "auto_activate", "0" ) )
	{
		idEntity* player = gameLocal.GetLocalPlayer();
		if( player )
		{
			float dist = ( player->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() ).Length();
			bool inRadius = ( dist < activateRadius );

			if( inRadius && !active )
			{
				Event_Enable();
			}
			if( !inRadius && active )
			{
				Event_Disable();
			}
		}
	}

	// Advance blend
	if( tonemapBlendAlpha < 1.0f )
	{
		tonemapBlendAlpha = idMath::ClampFloat( 0.0f, 1.0f,
												tonemapBlendAlpha + dt * tonemapBlendSpeed );

		// Lerp current toward target each frame
		tonemapCurrent.exposure = Lerp( tonemapCurrent.exposure, tonemapTarget.exposure, tonemapBlendAlpha );
		tonemapCurrent.saturation = Lerp( tonemapCurrent.saturation, tonemapTarget.saturation, tonemapBlendAlpha );
		tonemapCurrent.contrast = Lerp( tonemapCurrent.contrast, tonemapTarget.contrast, tonemapBlendAlpha );
		tonemapCurrent.hdrKey = Lerp( tonemapCurrent.hdrKey, tonemapTarget.hdrKey, tonemapBlendAlpha );

		// Flip preset at the midpoint so there's no abrupt switch at the start
		if( tonemapBlendAlpha >= 0.5f )
		{
			tonemapCurrent.preset = tonemapTarget.preset;
		}
	}
	else
	{
		// Blend is done snap current to target and stop thinking
		tonemapCurrent = tonemapTarget;

		if( !active )
		{
			BecomeInactive( TH_THINK );    // dormant until triggered again
		}
	}
}

/*
================
idTonemapController::Event_Enable
================
*/
void idTonemapController::Event_Enable()
{
	if( active )
	{
		return;
	}

	active = true;

	// Snapshot what the current target is so we can restore it on disable
	prevPreset = tonemapTarget.preset;
	prevExposure = tonemapTarget.exposure;
	prevSaturation = tonemapTarget.saturation;
	prevContrast = tonemapTarget.contrast;
	prevKey = tonemapTarget.hdrKey;

	// Push our values as the new target
	tonemapTarget.preset = preset;
	tonemapTarget.exposure = exposure;
	tonemapTarget.saturation = saturation;
	tonemapTarget.contrast = contrast;
	tonemapTarget.hdrKey = hdrKey;

	// Start blend from zero (fully blending in)
	tonemapBlendAlpha = 0.0f;
	tonemapBlendSpeed = ( blendTime > 0.0f ) ? ( 1.0f / blendTime ) : 100.0f;

	BecomeActive( TH_THINK );
}

/*
================
idTonemapController::Event_Disable
================
*/
void idTonemapController::Event_Disable()
{
	if( !active )
	{
		return;
	}

	active = false;

	// Restore the previous state as the new target
	tonemapTarget.preset = prevPreset;
	tonemapTarget.exposure = prevExposure;
	tonemapTarget.saturation = prevSaturation;
	tonemapTarget.contrast = prevContrast;
	tonemapTarget.hdrKey = prevKey;

	// Start blend from wherever we currently are (blending back out)
	// Invert alpha so blend-out takes the same time as blend-in
	tonemapBlendAlpha = 1.0f - tonemapBlendAlpha;
	tonemapBlendSpeed = ( blendTime > 0.0f ) ? ( 1.0f / blendTime ) : 100.0f;

	// Keep thinking until blend finishes, then go dormant
}

/*
================
idGameLocal::GetActiveTonemapState

Called from renderer backend before tonemap pass

Returns true if a tonemap controller is currently blending in or active.
================
*/
bool idGameLocal::GetActiveTonemapState( int& preset, float& exposure, float& saturation, float& contrast, float& hdrKey )
{
	// Only return true if a controller is currently active or in the middle of blending
	if( !idTonemapController::IsControllerActive() )
	{
		return false;
	}

	preset = idTonemapController::tonemapCurrent.preset;
	exposure = idTonemapController::tonemapCurrent.exposure;
	saturation = idTonemapController::tonemapCurrent.saturation;
	contrast = idTonemapController::tonemapCurrent.contrast;
	hdrKey = idTonemapController::tonemapCurrent.hdrKey;

	return true;
}

/*
================
idTonemapController::WriteToSnapshot
================
*/
void idTonemapController::WriteToSnapshot( idBitMsg& msg ) const
{
	GetPhysics()->WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );

	msg.WriteByte( active ? 1 : 0 );
	msg.WriteFloat( blendTime );
	msg.WriteLong( preset );
	msg.WriteFloat( exposure );
	msg.WriteFloat( saturation );
	msg.WriteFloat( contrast );
	msg.WriteFloat( hdrKey );
}

/*
================
idTonemapController::ReadFromSnapshot
================
*/
void idTonemapController::ReadFromSnapshot( const idBitMsg& msg )
{
	GetPhysics()->ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );

	bool wasActive = active;
	active = msg.ReadByte() != 0;
	blendTime = msg.ReadFloat();
	preset = msg.ReadLong();
	exposure = msg.ReadFloat();
	saturation = msg.ReadFloat();
	contrast = msg.ReadFloat();
	hdrKey = msg.ReadFloat();

	// If the active state changed on the snapshot, fire the appropriate event
	if( active != wasActive )
	{
		if( active )
		{
			Event_Enable();
		}
		else
		{
			Event_Disable();
		}
	}
}

/*
================
idTonemapController::ClientPredictionThink
================
*/
void idTonemapController::ClientPredictionThink()
{
	// Client-side prediction mirrors the server's Think logic
	Think();
}

/*
================
idTonemapController::ClientReceiveEvent
================
*/
bool idTonemapController::ClientReceiveEvent( int event, int time, const idBitMsg& msg )
{
	// Events are handled through snapshot data, so delegate to base class
	return idEntity::ClientReceiveEvent( event, time, msg );
}