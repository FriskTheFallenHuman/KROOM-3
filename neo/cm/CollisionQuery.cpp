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

	Collision query execution boundary.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "CollisionModel_local.h"
#include "CollisionQuery.h"

namespace
{

static const int CM_QUERY_SCRATCH_SLOTS = 2;

struct cm_queryScratchSlot_t
{
	ALIGN16( cm_traceWork_t traceWork );
	cm_queryCache_t queryCache;
	bool inUse;

	cm_queryScratchSlot_t() : inUse( false )
	{
	}
};

static thread_local cm_queryScratchSlot_t cm_queryScratchSlots[CM_QUERY_SCRATCH_SLOTS];

}

/*
==================
cm_queryScratchScope_t::cm_queryScratchScope_t
==================
*/
cm_queryScratchScope_t::cm_queryScratchScope_t() :
	traceWork( NULL ),
	queryCache( NULL ),
	slotIndex( -1 )
{
	for( int i = 0; i < CM_QUERY_SCRATCH_SLOTS; ++i )
	{
		if( !cm_queryScratchSlots[i].inUse )
		{
			cm_queryScratchSlots[i].inUse = true;
			traceWork = &cm_queryScratchSlots[i].traceWork;
			queryCache = &cm_queryScratchSlots[i].queryCache;
			slotIndex = i;
			break;
		}
	}

	// Two slots cover the normal query plus the optional debug start-solid
	// query.  Retain a safe path for deeper re-entry instead of sharing state.
	if( traceWork == NULL )
	{
		traceWork = static_cast<cm_traceWork_t*>( Mem_Alloc16( sizeof( *traceWork ), TAG_COLLISION ) );
		queryCache = new( TAG_COLLISION ) cm_queryCache_t;
	}

	memset( traceWork, 0, sizeof( *traceWork ) );
	queryCache->Reset();
	traceWork->queryCache = queryCache;
}

/*
==================
cm_queryScratchScope_t::~cm_queryScratchScope_t
==================
*/
cm_queryScratchScope_t::~cm_queryScratchScope_t()
{
	if( slotIndex >= 0 )
	{
		cm_queryScratchSlots[slotIndex].inUse = false;
		return;
	}

	delete queryCache;
	Mem_Free16( traceWork );
}

/*
==================
cm_queryScratchScope_t::cmQueryParms_t
==================
*/
cmQueryParms_t::cmQueryParms_t() :
	type( CM_QUERY_TRANSLATION ),
	rotation( NULL ),
	depth( 0.0f ),
	traceModel( NULL ),
	traceModelAxis( mat3_identity ),
	contentsMask( -1 ),
	model( 0 ),
	modelAxis( mat3_identity ),
	contacts( NULL ),
	maxContacts( 0 )
{
}

/*
==================
cm_queryScratchScope_t::idCollisionQuery
==================
*/
idCollisionQuery::idCollisionQuery() :
	contents( 0 ),
	numContacts( 0 ),
	completed( false )
{
	memset( &trace, 0, sizeof( trace ) );
	trace.fraction = 1.0f;
	trace.endAxis = mat3_identity;
}

/*
==================
cm_queryScratchScope_t::Translation
==================
*/
idCollisionQuery idCollisionQueryExecute::Translation( idCollisionModelManagerLocal& manager, const cmQueryParms_t& parms )
{
	idCollisionQuery query;
	manager.TranslationInternal( &query.trace, parms.start, parms.end, parms.traceModel,
								 parms.traceModelAxis, parms.contentsMask, parms.model, parms.modelOrigin,
								 parms.modelAxis, NULL, 0, NULL );
	query.completed = true;
	return query;
}

/*
==================
cm_queryScratchScope_t::Rotation
==================
*/
idCollisionQuery idCollisionQueryExecute::Rotation( idCollisionModelManagerLocal& manager, const cmQueryParms_t& parms )
{
	idCollisionQuery query;
	if( parms.rotation != NULL )
	{
		manager.RotationInternal( &query.trace, parms.start, *parms.rotation, parms.traceModel,
								  parms.traceModelAxis, parms.contentsMask, parms.model, parms.modelOrigin, parms.modelAxis );
	}
	query.completed = true;
	return query;
}

/*
==================
cm_queryScratchScope_t::Contents
==================
*/
idCollisionQuery idCollisionQueryExecute::Contents( idCollisionModelManagerLocal& manager, const cmQueryParms_t& parms )
{
	idCollisionQuery query;
	query.contents = manager.ContentsInternal( parms.start, parms.traceModel, parms.traceModelAxis,
					 parms.contentsMask, parms.model, parms.modelOrigin, parms.modelAxis );
	query.completed = true;
	return query;
}

/*
==================
cm_queryScratchScope_t::Contacts
==================
*/
idCollisionQuery idCollisionQueryExecute::Contacts( idCollisionModelManagerLocal& manager, const cmQueryParms_t& parms )
{
	idCollisionQuery query;
	query.numContacts = manager.ContactsInternal( parms.contacts, parms.maxContacts, parms.start,
						parms.direction, parms.depth, parms.traceModel, parms.traceModelAxis, parms.contentsMask,
						parms.model, parms.modelOrigin, parms.modelAxis );
	query.completed = true;
	return query;
}
