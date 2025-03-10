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

#ifndef __FORCE_DRAG_H__
#define __FORCE_DRAG_H__

/*
===============================================================================

	Drag force

===============================================================================
*/

class idForce_Drag : public idForce
{

public:
	CLASS_PROTOTYPE( idForce_Drag );

	idForce_Drag();
	virtual				~idForce_Drag();
	// initialize the drag force
	void				Init( float damping );
	// set physics object being dragged
	void				SetPhysics( idPhysics* physics, int id, const idVec3& p );
	// set position to drag towards
	void				SetDragPosition( const idVec3& pos );
	// get the position dragged towards
	const idVec3& 		GetDragPosition() const;
	// get the position on the dragged physics object
	const idVec3		GetDraggedPosition() const;

public: // common force interface
	virtual void		Evaluate( int time );
	virtual void		RemovePhysics( const idPhysics* phys );

private:

	// properties
	float				damping;

	// positioning
	idPhysics* 			physics;		// physics object
	int					id;				// clip model id of physics object
	idVec3				p;				// position on clip model
	idVec3				dragPosition;	// drag towards this position
};

#endif /* !__FORCE_DRAG_H__ */
