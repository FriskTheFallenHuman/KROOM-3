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

#ifndef __FORCE_CONSTANT_H__
#define __FORCE_CONSTANT_H__

/*
===============================================================================

	Constant force

===============================================================================
*/

class idForce_Constant : public idForce
{

public:
	CLASS_PROTOTYPE( idForce_Constant );

	idForce_Constant();
	virtual				~idForce_Constant();


	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	// constant force
	void				SetForce( const idVec3& force );
	// set force position
	void				SetPosition( idPhysics* physics, int id, const idVec3& point );

	void				SetPhysics( idPhysics* physics );

public: // common force interface
	virtual void		Evaluate( int time );
	virtual void		RemovePhysics( const idPhysics* phys );

private:
	// force properties
	idVec3				force;
	idPhysics* 			physics;
	int					id;
	idVec3				point;
};

#endif /* !__FORCE_CONSTANT_H__ */
