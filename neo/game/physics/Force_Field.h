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

#ifndef __FORCE_FIELD_H__
#define __FORCE_FIELD_H__

/*
===============================================================================

	Force field

===============================================================================
*/

enum forceFieldType
{
	FORCEFIELD_UNIFORM,
	FORCEFIELD_EXPLOSION,
	FORCEFIELD_IMPLOSION
};

enum forceFieldApplyType
{
	FORCEFIELD_APPLY_FORCE,
	FORCEFIELD_APPLY_VELOCITY,
	FORCEFIELD_APPLY_IMPULSE
};

class idForce_Field : public idForce
{

public:
	CLASS_PROTOTYPE( idForce_Field );

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	idForce_Field();
	virtual				~idForce_Field();
	// uniform constant force
	void				Uniform( const idVec3& force );
	// explosion from clip model origin
	void				Explosion( float force );
	// implosion towards clip model origin
	void				Implosion( float force );
	// add random torque
	void				RandomTorque( float force );
	// should the force field apply a force, velocity or impulse
	void				SetApplyType( const forceFieldApplyType type )
	{
		applyType = type;
	}
	// make the force field only push players
	void				SetPlayerOnly( bool set )
	{
		playerOnly = set;
	}
	// make the force field only push monsters
	void				SetMonsterOnly( bool set )
	{
		monsterOnly = set;
	}
	// clip model describing the extents of the force field
	void				SetClipModel( idClipModel* clipModel );

public: // common force interface
	virtual void		Evaluate( int time );

private:
	// force properties
	forceFieldType		type;
	forceFieldApplyType	applyType;
	float				magnitude;
	idVec3				dir;
	float				randomTorque;
	bool				playerOnly;
	bool				monsterOnly;
	idClipModel* 		clipModel;
};

#endif /* !__FORCE_FIELD_H__ */
