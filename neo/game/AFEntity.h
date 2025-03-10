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

#ifndef __GAME_AFENTITY_H__
#define __GAME_AFENTITY_H__


/*
===============================================================================

idMultiModelAF

Entity using multiple separate visual models animated with a single
articulated figure. Only used for debugging!

===============================================================================
*/
const int GIB_DELAY = 200;  // only gib this often to keep performace hits when blowing up several mobs

class idMultiModelAF : public idEntity
{
public:
	CLASS_PROTOTYPE( idMultiModelAF );

	void					Spawn();
	~idMultiModelAF();

	virtual void			Think();
	virtual void			Present();

protected:
	idPhysics_AF			physicsObj;

	void					SetModelForId( int id, const idStr& modelName );

private:
	idList<idRenderModel*>	modelHandles;
	idList<int>				modelDefHandles;
};


/*
===============================================================================

idChain

Chain hanging down from the ceiling. Only used for debugging!

===============================================================================
*/

class idChain : public idMultiModelAF
{
public:
	CLASS_PROTOTYPE( idChain );

	void					Spawn();

protected:
	void					BuildChain( const idStr& name, const idVec3& origin, float linkLength, float linkWidth, float density, int numLinks, bool bindToWorld = true );
};


/*
===============================================================================

idAFAttachment

===============================================================================
*/

class idAFAttachment : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idAFAttachment );

	idAFAttachment();
	virtual					~idAFAttachment();

	void					Spawn();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	void					SetBody( idEntity* bodyEnt, const char* headModel, jointHandle_t attachJoint );
	void					ClearBody();
	idEntity* 				GetBody() const;

	virtual void			Think();

	virtual void			Hide();
	virtual void			Show();

	void					PlayIdleAnim( int blendTime );

	virtual void			GetImpactInfo( idEntity* ent, int id, const idVec3& point, impactInfo_t* info );
	virtual void			ApplyImpulse( idEntity* ent, int id, const idVec3& point, const idVec3& impulse );
	virtual void			AddForce( idEntity* ent, int id, const idVec3& point, const idVec3& force );

	virtual	void			Damage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location );
	virtual void			AddDamageEffect( const trace_t& collision, const idVec3& velocity, const char* damageDefName );

	void					SetCombatModel();
	idClipModel* 			GetCombatModel() const;
	virtual void			LinkCombat();
	virtual void			UnlinkCombat();

protected:
	idEntity* 				body;
	idClipModel* 			combatModel;	// render model for hit detection of head
	int						idleAnim;
	jointHandle_t			attachJoint;
};


/*
===============================================================================

idAFEntity_Base

===============================================================================
*/

class idAFEntity_Base : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idAFEntity_Base );

	idAFEntity_Base();
	virtual					~idAFEntity_Base();

	void					Spawn();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	virtual void			Think();
	virtual void			AddDamageEffect( const trace_t& collision, const idVec3& velocity, const char* damageDefName );
	virtual void			GetImpactInfo( idEntity* ent, int id, const idVec3& point, impactInfo_t* info );
	virtual void			ApplyImpulse( idEntity* ent, int id, const idVec3& point, const idVec3& impulse );
	virtual void			AddForce( idEntity* ent, int id, const idVec3& point, const idVec3& force );
	virtual bool			Collide( const trace_t& collision, const idVec3& velocity );
	virtual bool			GetPhysicsToVisualTransform( idVec3& origin, idMat3& axis );
	virtual bool			UpdateAnimationControllers();
	virtual void			FreeModelDef();

	virtual bool			LoadAF();
	bool					IsActiveAF() const
	{
		return af.IsActive();
	}
	const char* 			GetAFName() const
	{
		return af.GetName();
	}
	idPhysics_AF* 			GetAFPhysics()
	{
		return af.GetPhysics();
	}

	void					SetCombatModel();
	idClipModel* 			GetCombatModel() const;
	// contents of combatModel can be set to 0 or re-enabled (mp)
	void					SetCombatContents( bool enable );
	virtual void			LinkCombat();
	virtual void			UnlinkCombat();

	int						BodyForClipModelId( int id ) const;

	void					SaveState( idDict& args ) const;
	void					LoadState( const idDict& args );

	void					AddBindConstraints();
	void					RemoveBindConstraints();

	virtual void			ShowEditingDialog();

	static void				DropAFs( idEntity* ent, const char* type, idList<idEntity*>* list );

protected:
	idAF					af;				// articulated figure
	idClipModel* 			combatModel;	// render model for hit detection
	int						combatModelContents;
	idVec3					spawnOrigin;	// spawn origin
	idMat3					spawnAxis;		// rotation axis used when spawned
	int						nextSoundTime;	// next time this can make a sound

	void					Event_SetConstraintPosition( const char* name, const idVec3& pos );
};

/*
===============================================================================

idAFEntity_Gibbable

===============================================================================
*/

extern const idEventDef		EV_Gib;
extern const idEventDef		EV_Gibbed;

class idAFEntity_Gibbable : public idAFEntity_Base
{
public:
	CLASS_PROTOTYPE( idAFEntity_Gibbable );

	idAFEntity_Gibbable();
	~idAFEntity_Gibbable();

	void					Spawn();
	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );
	virtual void			Present();
	virtual	void			Damage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location );
	virtual void			SpawnGibs( const idVec3& dir, const char* damageDefName );

protected:
	idRenderModel* 			skeletonModel;
	int						skeletonModelDefHandle;
	bool					gibbed;

	virtual void			Gib( const idVec3& dir, const char* damageDefName );
	void					InitSkeletonModel();

	void					Event_Gib( const char* damageDefName );
};

/*
===============================================================================

	idAFEntity_Generic

===============================================================================
*/

class idAFEntity_Generic : public idAFEntity_Gibbable
{
public:
	CLASS_PROTOTYPE( idAFEntity_Generic );

	idAFEntity_Generic();
	~idAFEntity_Generic();

	void					Spawn();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	virtual void			Think();
	void					KeepRunningPhysics()
	{
		keepRunningPhysics = true;
	}

private:
	void					Event_Activate( idEntity* activator );

	bool					keepRunningPhysics;
};


/*
===============================================================================

idAFEntity_WithAttachedHead

===============================================================================
*/

class idAFEntity_WithAttachedHead : public idAFEntity_Gibbable
{
public:
	CLASS_PROTOTYPE( idAFEntity_WithAttachedHead );

	idAFEntity_WithAttachedHead();
	~idAFEntity_WithAttachedHead();

	void					Spawn();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	void					SetupHead();

	virtual void			Think();

	virtual void			Hide();
	virtual void			Show();
	virtual void			ProjectOverlay( const idVec3& origin, const idVec3& dir, float size, const char* material );

	virtual void			LinkCombat();
	virtual void			UnlinkCombat();

protected:
	virtual void			Gib( const idVec3& dir, const char* damageDefName );

private:
	idEntityPtr<idAFAttachment>	head;

	void					Event_Gib( const char* damageDefName );
	void					Event_Activate( idEntity* activator );
};


/*
===============================================================================

idAFEntity_Vehicle

===============================================================================
*/

class idAFEntity_Vehicle : public idAFEntity_Base
{
public:
	CLASS_PROTOTYPE( idAFEntity_Vehicle );

	idAFEntity_Vehicle();

	void					Spawn();
	void					Use( idPlayer* player );

protected:
	idPlayer* 				player;
	jointHandle_t			eyesJoint;
	jointHandle_t			steeringWheelJoint;
	float					wheelRadius;
	float					steerAngle;
	float					steerSpeed;
	const idDeclParticle* 	dustSmoke;

	float					GetSteerAngle();
};


/*
===============================================================================

idAFEntity_VehicleSimple

===============================================================================
*/

class idAFEntity_VehicleSimple : public idAFEntity_Vehicle
{
public:
	CLASS_PROTOTYPE( idAFEntity_VehicleSimple );

	idAFEntity_VehicleSimple();
	~idAFEntity_VehicleSimple();

	void					Spawn();
	virtual void			Think();

protected:
	idClipModel* 			wheelModel;
	idAFConstraint_Suspension* 	suspension[4];
	jointHandle_t			wheelJoints[4];
	float					wheelAngles[4];
};


/*
===============================================================================

idAFEntity_VehicleFourWheels

===============================================================================
*/

class idAFEntity_VehicleFourWheels : public idAFEntity_Vehicle
{
public:
	CLASS_PROTOTYPE( idAFEntity_VehicleFourWheels );

	idAFEntity_VehicleFourWheels();

	void					Spawn();
	virtual void			Think();

protected:
	idAFBody* 				wheels[4];
	idAFConstraint_Hinge* 	steering[2];
	jointHandle_t			wheelJoints[4];
	float					wheelAngles[4];
};


/*
===============================================================================

idAFEntity_VehicleSixWheels

===============================================================================
*/

class idAFEntity_VehicleSixWheels : public idAFEntity_Vehicle
{
public:
	CLASS_PROTOTYPE( idAFEntity_VehicleSixWheels );

	idAFEntity_VehicleSixWheels();

	void					Spawn();
	virtual void			Think();

private:
	idAFBody* 				wheels[6];
	idAFConstraint_Hinge* 	steering[4];
	jointHandle_t			wheelJoints[6];
	float					wheelAngles[6];
};


/*
===============================================================================

idAFEntity_SteamPipe

===============================================================================
*/

class idAFEntity_SteamPipe : public idAFEntity_Base
{
public:
	CLASS_PROTOTYPE( idAFEntity_SteamPipe );

	idAFEntity_SteamPipe();
	~idAFEntity_SteamPipe();

	void					Spawn();
	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	virtual void			Think();

private:
	int						steamBody;
	float					steamForce;
	float					steamUpForce;
	idForce_Constant		force;
	renderEntity_t			steamRenderEntity;
	qhandle_t				steamModelDefHandle;

	void					InitSteamRenderEntity();
};


/*
===============================================================================

idAFEntity_ClawFourFingers

===============================================================================
*/

class idAFEntity_ClawFourFingers : public idAFEntity_Base
{
public:
	CLASS_PROTOTYPE( idAFEntity_ClawFourFingers );

	idAFEntity_ClawFourFingers();

	void					Spawn();
	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

private:
	idAFConstraint_Hinge* 	fingers[4];

	void					Event_SetFingerAngle( float angle );
	void					Event_StopFingers();
};

#endif /* !__GAME_AFENTITY_H__ */
