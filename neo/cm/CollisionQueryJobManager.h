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

	Collision query job manager.

	Owns the worker job list and executes independent model queries. The game
	physics API remains synchronous, but model-level narrow phase work is
	submitted and joined through this idTech 5-style boundary.

===========================================================================
*/

#ifndef __COLLISIONQUERYJOBMANAGER_H__
#define __COLLISIONQUERYJOBMANAGER_H__

#include "CollisionQuery.h"

class idCollisionModelManagerLocal;
class idParallelJobList;

struct cmExecuteQueryJob_t
{
	idCollisionModelManagerLocal* manager;
	cmQueryParms_t parms;
	idRotation rotationStorage;
	trace_t* traceResult;
	int* integerResult;
};

class idCollisionQueryJobManager
{
public:
	idCollisionQueryJobManager();

	bool	BeginTranslation( idCollisionModelManagerLocal& manager, trace_t* result,
							  const idVec3& start, const idVec3& end, const idTraceModel* trm,
							  const idMat3& trmAxis, int contentMask, cmHandle_t model,
							  const idVec3& modelOrigin, const idMat3& modelAxis, bool allowParallel );
	bool	BeginRotation( idCollisionModelManagerLocal& manager, trace_t* result,
						   const idVec3& start, const idRotation& rotation, const idTraceModel* trm,
						   const idMat3& trmAxis, int contentMask, cmHandle_t model,
						   const idVec3& modelOrigin, const idMat3& modelAxis, bool allowParallel );

	bool	Translation( idCollisionModelManagerLocal& manager, trace_t* results,
						 const int numModels, const idVec3& start, const idVec3& end,
						 const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
						 const idPositionedCollisionModel* positionedModels, bool allowParallel );
	bool	TranslationQueries( idCollisionModelManagerLocal& manager,
								idCollisionTranslationQuery* queries, int numQueries, bool allowParallel );
	bool	Rotation( idCollisionModelManagerLocal& manager, trace_t* results,
					  const int numModels, const idVec3& start, const idRotation& rotation,
					  const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
					  const idPositionedCollisionModel* positionedModels, bool allowParallel );
	bool	RotationQueries( idCollisionModelManagerLocal& manager,
							 idCollisionRotationQuery* queries, int numQueries, bool allowParallel );
	bool	Contents( idCollisionModelManagerLocal& manager, int* results,
					  const int numModels, const idVec3& start, const idTraceModel* trm,
					  const idMat3& trmAxis, int contentMask,
					  const idPositionedCollisionModel* positionedModels, bool allowParallel );
	bool	Contacts( idCollisionModelManagerLocal& manager, contactInfo_t* contacts,
					  int* numContacts, const int maxContactsPerModel, const int numModels,
					  const idVec3& start, const idVec6& dir, const float depth,
					  const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
					  const idPositionedCollisionModel* positionedModels, bool allowParallel );

	void	WaitForAllQueries();
	void	Shutdown();

private:
	bool	ExecuteJobs( void ( *function )( void* ), void* jobs, int jobStride,
						 int numJobs, bool allowParallel );
	bool	BeginJob( void ( *function )( void* ), void* job, bool allowParallel );

	idParallelJobList* jobList;
	idList< cmExecuteQueryJob_t, TAG_COLLISION > jobs;
};

#endif /* !__COLLISIONQUERYJOBMANAGER_H__ */
