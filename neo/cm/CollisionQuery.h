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

/*
===========================================================================

	Collision query boundary.

	This keeps the public Doom 3 BFG collision ABI intact while separating
	query description/execution from collision-model storage.  The layout is
	intentionally close to the idTech 5 collision flow so queries can later be
	batched without replacing the proven BFG narrow-phase implementation.

===========================================================================
*/

#ifndef __COLLISIONQUERY_H__
#define __COLLISIONQUERY_H__

#include "CollisionModel.h"

class idCollisionModelManagerLocal;

enum cmQueryType_t
{
	CM_QUERY_TRANSLATION,
	CM_QUERY_ROTATION,
	CM_QUERY_CONTENTS,
	CM_QUERY_CONTACTS
};

struct cmQueryParms_t
{
	cmQueryType_t			type;
	idVec3					start;
	idVec3					end;
	const idRotation* 		rotation;
	idVec6					direction;
	float					depth;
	const idTraceModel* 	traceModel;
	idMat3					traceModelAxis;
	int						contentsMask;
	cmHandle_t				model;
	idVec3					modelOrigin;
	idMat3					modelAxis;
	contactInfo_t* 			contacts;
	int						maxContacts;

	cmQueryParms_t();
};

// A model plus its world-space placement.  idClip currently performs this
// expansion itself; retaining it here establishes the batching boundary used
// by the idTech 5 collision manager.
struct idPositionedCollisionModel
{
	cmHandle_t	model;
	idVec3		origin;
	idMat3		axis;
	int			entityNum;
	int			physicsId;
	int			bodyId;
	int			contentsOverride;
	float		scale;
};

class idCollisionQuery
{
public:
	idCollisionQuery();

	trace_t	trace;
	int		contents;
	int		numContacts;
	bool	completed;
};

class idCollisionQueryExecute
{
public:
	static idCollisionQuery Translation( idCollisionModelManagerLocal& manager, const cmQueryParms_t& parms );
	static idCollisionQuery Rotation( idCollisionModelManagerLocal& manager, const cmQueryParms_t& parms );
	static idCollisionQuery Contents( idCollisionModelManagerLocal& manager, const cmQueryParms_t& parms );
	static idCollisionQuery Contacts( idCollisionModelManagerLocal& manager, const cmQueryParms_t& parms );
};

#endif /* !__COLLISIONQUERY_H__ */
