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

#include "Game_local.h"

/*
===============================================================================

  SOUND

===============================================================================
*/

const idEventDef EV_Speaker_On( "On", NULL );
const idEventDef EV_Speaker_Off( "Off", NULL );
const idEventDef EV_Speaker_Timer( "<timer>", NULL );

CLASS_DECLARATION( idEntity, idSound )
EVENT( EV_Activate,				idSound::Event_Trigger )
EVENT( EV_Speaker_On,			idSound::Event_On )
EVENT( EV_Speaker_Off,			idSound::Event_Off )
EVENT( EV_Speaker_Timer,		idSound::Event_Timer )
END_CLASS


/*
================
idSound::idSound
================
*/
idSound::idSound()
{
	lastSoundVol = 0.0f;
	soundVol = 0.0f;
	shakeTranslate.Zero();
	shakeRotate.Zero();
	random = 0.0f;
	wait = 0.0f;
	timerOn = false;
	playingUntilTime = 0;
}

/*
================
idSound::Save
================
*/
void idSound::Save( idSaveGame* savefile ) const
{
	savefile->WriteFloat( lastSoundVol );
	savefile->WriteFloat( soundVol );
	savefile->WriteFloat( random );
	savefile->WriteFloat( wait );
	savefile->WriteBool( timerOn );
	savefile->WriteVec3( shakeTranslate );
	savefile->WriteAngles( shakeRotate );
	savefile->WriteInt( playingUntilTime );
}

/*
================
idSound::Restore
================
*/
void idSound::Restore( idRestoreGame* savefile )
{
	savefile->ReadFloat( lastSoundVol );
	savefile->ReadFloat( soundVol );
	savefile->ReadFloat( random );
	savefile->ReadFloat( wait );
	savefile->ReadBool( timerOn );
	savefile->ReadVec3( shakeTranslate );
	savefile->ReadAngles( shakeRotate );
	savefile->ReadInt( playingUntilTime );
}

/*
================
idSound::Spawn
================
*/
void idSound::Spawn()
{
	spawnArgs.GetVector( "move", "0 0 0", shakeTranslate );
	spawnArgs.GetAngles( "rotate", "0 0 0", shakeRotate );
	spawnArgs.GetFloat( "random", "0", random );
	spawnArgs.GetFloat( "wait", "0", wait );

	if( ( wait > 0.0f ) && ( random >= wait ) )
	{
		random = wait - 0.001;
		gameLocal.Warning( "speaker '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ) );
	}

	soundVol		= 0.0f;
	lastSoundVol	= 0.0f;

	if( ( shakeRotate != ang_zero ) || ( shakeTranslate != vec3_zero ) )
	{
		BecomeActive( TH_THINK );
	}

	if( !refSound.waitfortrigger && ( wait > 0.0f ) )
	{
		timerOn = true;
		PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
	}
	else
	{
		timerOn = false;
	}
}

/*
================
idSound::Event_Trigger

this will toggle the idle idSound on and off
================
*/
void idSound::Event_Trigger( idEntity* activator )
{
	if( wait > 0.0f )
	{
		if( timerOn )
		{
			timerOn = false;
			CancelEvents( &EV_Speaker_Timer );
		}
		else
		{
			timerOn = true;
			DoSound( true );
			PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
		}
	}
	else
	{
		if( gameLocal.isMultiplayer )
		{
			if( refSound.referenceSound && ( gameLocal.time < playingUntilTime ) )
			{
				DoSound( false );
			}
			else
			{
				DoSound( true );
			}
		}
		else
		{
			if( refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying() )
			{
				DoSound( false );
			}
			else
			{
				DoSound( true );
			}
		}
	}
}

/*
================
idSound::Event_Timer
================
*/
void idSound::Event_Timer()
{
	DoSound( true );
	PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
}

/*
================
idSound::Think
================
*/
void idSound::Think()
{
	idAngles	ang;

	// run physics
	RunPhysics();

	// clear out our update visuals think flag since we never call Present
	BecomeInactive( TH_UPDATEVISUALS );
}

/*
===============
idSound::UpdateChangableSpawnArgs
===============
*/
void idSound::UpdateChangeableSpawnArgs( const idDict* source )
{

	idEntity::UpdateChangeableSpawnArgs( source );

	if( source )
	{
		FreeSoundEmitter( true );
		spawnArgs.Copy( *source );
		idSoundEmitter* saveRef = refSound.referenceSound;
		gameEdit->ParseSpawnArgsToRefSound( &spawnArgs, &refSound );
		refSound.referenceSound = saveRef;

		idVec3 origin;
		idMat3 axis;

		if( GetPhysicsToSoundTransform( origin, axis ) )
		{
			refSound.origin = GetPhysics()->GetOrigin() + origin * axis;
		}
		else
		{
			refSound.origin = GetPhysics()->GetOrigin();
		}

		spawnArgs.GetFloat( "random", "0", random );
		spawnArgs.GetFloat( "wait", "0", wait );

		if( ( wait > 0.0f ) && ( random >= wait ) )
		{
			random = wait - 0.001;
			gameLocal.Warning( "speaker '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ) );
		}

		if( !refSound.waitfortrigger && ( wait > 0.0f ) )
		{
			timerOn = true;
			DoSound( false );
			CancelEvents( &EV_Speaker_Timer );
			PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
		}
		else  if( !refSound.waitfortrigger && !( refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying() ) )
		{
			// start it if it isn't already playing, and we aren't waitForTrigger
			DoSound( true );
			timerOn = false;
		}
	}
}

/*
===============
idSound::SetSound
===============
*/
void idSound::SetSound( const char* sound, int channel )
{
	const idSoundShader* shader = declManager->FindSound( sound );
	if( shader != refSound.shader )
	{
		FreeSoundEmitter( true );
	}
	gameEdit->ParseSpawnArgsToRefSound( &spawnArgs, &refSound );
	refSound.shader = shader;
	// start it if it isn't already playing, and we aren't waitForTrigger
	if( !refSound.waitfortrigger && !( refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying() ) )
	{
		DoSound( true );
	}
}

/*
================
idSound::DoSound
================
*/
void idSound::DoSound( bool play )
{
	if( play )
	{
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, refSound.parms.soundShaderFlags, true, &playingUntilTime );
		playingUntilTime += gameLocal.time;
	}
	else
	{
		StopSound( SND_CHANNEL_ANY, true );
		playingUntilTime = 0;
	}
}

/*
================
idSound::Event_On
================
*/
void idSound::Event_On()
{
	if( wait > 0.0f )
	{
		timerOn = true;
		PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
	}
	DoSound( true );
}

/*
================
idSound::Event_Off
================
*/
void idSound::Event_Off()
{
	if( timerOn )
	{
		timerOn = false;
		CancelEvents( &EV_Speaker_Timer );
	}
	DoSound( false );
}

/*
===============
idSound::ShowEditingDialog
===============
*/
void idSound::ShowEditingDialog()
{
	common->InitTool( EDITOR_SOUND, &spawnArgs );
}
