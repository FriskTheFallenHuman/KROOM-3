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

#ifndef __GAME_TARGET_H__
#define __GAME_TARGET_H__


/*
===============================================================================

idTarget

===============================================================================
*/

class idTarget : public idEntity
{
public:
	CLASS_PROTOTYPE( idTarget );
};


/*
===============================================================================

idTarget_Remove

===============================================================================
*/

class idTarget_Remove : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_Remove );

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_Show

===============================================================================
*/

class idTarget_Show : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_Show );

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_Damage

===============================================================================
*/

class idTarget_Damage : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_Damage );

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_SessionCommand

===============================================================================
*/

class idTarget_SessionCommand : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_SessionCommand );

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_EndLevel

===============================================================================
*/

class idTarget_EndLevel : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_EndLevel );

private:
	void				Event_Activate( idEntity* activator );

};


/*
===============================================================================

idTarget_WaitForButton

===============================================================================
*/

class idTarget_WaitForButton : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_WaitForButton );

	void				Think();

private:
	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

idTarget_SetGlobalShaderTime

===============================================================================
*/

class idTarget_SetGlobalShaderTime : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_SetGlobalShaderTime );

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_SetShaderParm

===============================================================================
*/

class idTarget_SetShaderParm : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_SetShaderParm );

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_SetShaderTime

===============================================================================
*/

class idTarget_SetShaderTime : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_SetShaderTime );

private:
	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

idTarget_FadeEntity

===============================================================================
*/

class idTarget_FadeEntity : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_FadeEntity );

	idTarget_FadeEntity();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Think();

private:
	idVec4				fadeFrom;
	int					fadeStart;
	int					fadeEnd;

	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

idTarget_LightFadeIn

===============================================================================
*/

class idTarget_LightFadeIn : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_LightFadeIn );

private:
	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

idTarget_LightFadeOut

===============================================================================
*/

class idTarget_LightFadeOut : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_LightFadeOut );

private:
	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

idTarget_Give

===============================================================================
*/

class idTarget_Give : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_Give );

	void				Spawn();

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_GiveEmail

===============================================================================
*/

class idTarget_GiveEmail : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_GiveEmail );

	void				Spawn();

private:
	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

idTarget_SetModel

===============================================================================
*/

class idTarget_SetModel : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_SetModel );

	void				Spawn();

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_SetInfluence

===============================================================================
*/

class idTarget_SetInfluence : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_SetInfluence );

	idTarget_SetInfluence();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Spawn();

private:
	void				Event_Activate( idEntity* activator );
	void				Event_RestoreInfluence();
	void				Event_GatherEntities();
	void				Event_Flash( float flash, int out );
	void				Event_ClearFlash( float flash );
	void				Think();

	idList<int>			lightList;
	idList<int>			guiList;
	idList<int>			soundList;
	idList<int>			genericList;
	float				flashIn;
	float				flashOut;
	float				delay;
	idStr				flashInSound;
	idStr				flashOutSound;
	idEntity* 			switchToCamera;
	idInterpolate<float>fovSetting;
	bool				soundFaded;
	bool				restoreOnTrigger;
};


/*
===============================================================================

idTarget_SetKeyVal

===============================================================================
*/

class idTarget_SetKeyVal : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_SetKeyVal );

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_SetFov

===============================================================================
*/

class idTarget_SetFov : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_SetFov );

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				Think();

private:
	idInterpolate<int>	fovSetting;

	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_SetPrimaryObjective

===============================================================================
*/

class idTarget_SetPrimaryObjective : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_SetPrimaryObjective );

private:
	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

idTarget_LockDoor

===============================================================================
*/

class idTarget_LockDoor: public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_LockDoor );

private:
	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

idTarget_CallObjectFunction

===============================================================================
*/

class idTarget_CallObjectFunction : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_CallObjectFunction );

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_LockDoor

===============================================================================
*/

class idTarget_EnableLevelWeapons : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_EnableLevelWeapons );

private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_Tip

===============================================================================
*/

class idTarget_Tip : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_Tip );

	idTarget_Tip();

	void				Spawn();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

private:
	idVec3				playerPos;

	void				Event_Activate( idEntity* activator );
	void				Event_TipOff();
	void				Event_GetPlayerPos();
};

/*
===============================================================================

idTarget_GiveSecurity

===============================================================================
*/
class idTarget_GiveSecurity : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_GiveSecurity );
private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_RemoveWeapons

===============================================================================
*/
class idTarget_RemoveWeapons : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_RemoveWeapons );
private:
	void				Event_Activate( idEntity* activator );
};


/*
===============================================================================

idTarget_LevelTrigger

===============================================================================
*/
class idTarget_LevelTrigger : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_LevelTrigger );
private:
	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

idTarget_EnableStamina

===============================================================================
*/
class idTarget_EnableStamina : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_EnableStamina );
private:
	void				Event_Activate( idEntity* activator );
};

/*
===============================================================================

idTarget_FadeSoundClass

===============================================================================
*/
class idTarget_FadeSoundClass : public idTarget
{
public:
	CLASS_PROTOTYPE( idTarget_FadeSoundClass );
private:
	void				Event_Activate( idEntity* activator );
	void				Event_RestoreVolume();
};


#endif /* !__GAME_TARGET_H__ */
