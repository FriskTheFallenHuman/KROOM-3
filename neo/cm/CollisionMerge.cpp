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

#include "precompiled.h"
#pragma hdrstop

/*
===========================================================================

	Deterministic collision query result merging.

===========================================================================
*/

#include "CollisionMerge.h"

/*
==================
idCollisionModelManagerLocal::MergeTraceResult
==================
*/
bool idCollisionDetectionMerge::MergeTraceResult( trace_t& result, const trace_t& candidate,
		int entityNum, int physicsId )
{
	if( candidate.fraction >= result.fraction )
	{
		return false;
	}
	result = candidate;
	result.c.entityNum = entityNum;
	result.c.id = physicsId;
	return true;
}

/*
==================
idCollisionModelManagerLocal::MergeContentsResults
==================
*/
int idCollisionDetectionMerge::MergeContentsResults( int contents, const int* results,
		const idPositionedCollisionModel* models, int numModels, int contentMask )
{
	for( int i = 0; i < numModels; ++i )
	{
		if( results[i] != 0 )
		{
			contents |= models[i].contentsOverride & contentMask;
		}
	}
	return contents;
}

/*
==================
idCollisionModelManagerLocal::MergeContactsResults
==================
*/
int idCollisionDetectionMerge::MergeContactsResults( contactInfo_t* output, int maxContacts,
		int numContacts, const contactInfo_t* inputs, const int* inputCounts,
		int contactsPerModel, const idPositionedCollisionModel* models, int numModels )
{
	for( int i = 0; i < numModels && numContacts < maxContacts; ++i )
	{
		const int copyCount = Min( inputCounts[i], maxContacts - numContacts );
		const contactInfo_t* source = inputs + i * contactsPerModel;
		for( int j = 0; j < copyCount; ++j )
		{
			output[numContacts] = source[j];
			output[numContacts].entityNum = models[i].entityNum;
			output[numContacts].id = models[i].physicsId;
			numContacts++;
		}
	}
	return numContacts;
}
