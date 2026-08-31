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

	Collision query worker submission and result collection.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "CollisionModel_local.h"
#include "CollisionQueryJobManager.h"

idCVar cm_parallelCollision( "cm_parallelCollision", "1", CVAR_GAME | CVAR_BOOL, "run independent positioned collision-model queries on worker threads" );
idCVar cm_parallelCollisionMinModels( "cm_parallelCollisionMinModels", "4", CVAR_GAME | CVAR_INTEGER, "minimum positioned collision models required before collision jobs are submitted", 2, 64 );
idCVar cm_asyncWorldCollision( "cm_asyncWorldCollision", "1", CVAR_GAME | CVAR_BOOL, "overlap sufficiently expensive world collision queries with game broadphase work" );
idCVar cm_asyncWorldCollisionMinTranslation( "cm_asyncWorldCollisionMinTranslation", "64", CVAR_GAME | CVAR_FLOAT, "minimum translation distance before an individual world query is submitted to a worker", 0.0f, 4096.0f );
idCVar cm_asyncWorldCollisionMinRotation( "cm_asyncWorldCollisionMinRotation", "15", CVAR_GAME | CVAR_FLOAT, "minimum rotation angle before an individual world query is submitted to a worker", 0.0f, 180.0f );

static const int CM_MAX_BATCHED_COLLISION_MODELS = 4096;

static void CM_ExecuteQueryJob( void* data )
{
	cmExecuteQueryJob_t* job = static_cast<cmExecuteQueryJob_t*>( data );
	switch( job->parms.type )
	{
		case CM_QUERY_TRANSLATION:
			*job->traceResult = idCollisionQueryExecute::Translation( *job->manager, job->parms ).trace;
			break;
		case CM_QUERY_ROTATION:
			*job->traceResult = idCollisionQueryExecute::Rotation( *job->manager, job->parms ).trace;
			break;
		case CM_QUERY_CONTENTS:
			*job->integerResult = idCollisionQueryExecute::Contents( *job->manager, job->parms ).contents;
			break;
		case CM_QUERY_CONTACTS:
			*job->integerResult = idCollisionQueryExecute::Contacts( *job->manager, job->parms ).numContacts;
			break;
	}
}

REGISTER_PARALLEL_JOB( CM_ExecuteQueryJob, "CM_ExecuteQueryJob" );

/*
==================
idCollisionQueryJobManager::idCollisionQueryJobManager
==================
*/
idCollisionQueryJobManager::idCollisionQueryJobManager() :
	jobList( NULL ),
	jobs( 64 )
{
}

/*
==================
idCollisionQueryJobManager::BeginJob
==================
*/
bool idCollisionQueryJobManager::BeginJob( void ( *function )( void* ), void* job,
		bool allowParallel )
{
	const bool runParallel = allowParallel && cm_parallelCollision.GetBool() &&
							 parallelJobManager != NULL;
	if( !runParallel )
	{
		function( job );
		return false;
	}

	if( jobList == NULL )
	{
		jobList = parallelJobManager->AllocJobList( JOBLIST_UTILITY,
				  JOBLIST_PRIORITY_MEDIUM, CM_MAX_BATCHED_COLLISION_MODELS, 0, NULL );
	}
	assert( !jobList->IsSubmitted() );
	jobList->AddJob( function, job );
	jobList->Submit();
	return true;
}

/*
==================
idCollisionQueryJobManager::BeginTranslation
==================
*/
bool idCollisionQueryJobManager::BeginTranslation( idCollisionModelManagerLocal& manager,
		trace_t* result, const idVec3& start, const idVec3& end, const idTraceModel* trm,
		const idMat3& trmAxis, int contentMask, cmHandle_t model,
		const idVec3& modelOrigin, const idMat3& modelAxis, bool allowParallel )
{
	jobs.SetNum( 1 );
	cmExecuteQueryJob_t& job = jobs[0];
	job.manager = &manager;
	job.traceResult = result;
	job.integerResult = NULL;
	job.parms.type = CM_QUERY_TRANSLATION;
	job.parms.start = start;
	job.parms.end = end;
	job.parms.traceModel = trm;
	job.parms.traceModelAxis = trmAxis;
	job.parms.contentsMask = contentMask;
	job.parms.model = model;
	job.parms.modelOrigin = modelOrigin;
	job.parms.modelAxis = modelAxis;
	const float minDistance = cm_asyncWorldCollisionMinTranslation.GetFloat();
	const bool asyncWorthwhile = cm_asyncWorldCollision.GetBool() &&
								 ( end - start ).LengthSqr() >= minDistance * minDistance;
	return BeginJob( CM_ExecuteQueryJob, &job, allowParallel && asyncWorthwhile );
}


/*
==================
idCollisionQueryJobManager::BeginRotation
==================
*/
bool idCollisionQueryJobManager::BeginRotation( idCollisionModelManagerLocal& manager,
		trace_t* result, const idVec3& start, const idRotation& rotation,
		const idTraceModel* trm, const idMat3& trmAxis, int contentMask, cmHandle_t model,
		const idVec3& modelOrigin, const idMat3& modelAxis, bool allowParallel )
{
	jobs.SetNum( 1 );
	cmExecuteQueryJob_t& job = jobs[0];
	job.manager = &manager;
	job.traceResult = result;
	job.integerResult = NULL;
	job.parms.type = CM_QUERY_ROTATION;
	job.parms.start = start;
	job.parms.rotation = &rotation;
	job.parms.traceModel = trm;
	job.parms.traceModelAxis = trmAxis;
	job.parms.contentsMask = contentMask;
	job.parms.model = model;
	job.parms.modelOrigin = modelOrigin;
	job.parms.modelAxis = modelAxis;
	const bool asyncWorthwhile = cm_asyncWorldCollision.GetBool() &&
								 idMath::Fabs( rotation.GetAngle() ) >= cm_asyncWorldCollisionMinRotation.GetFloat();
	return BeginJob( CM_ExecuteQueryJob, &job, allowParallel && asyncWorthwhile );
}

/*
==================
idCollisionQueryJobManager::ExecuteJobs
==================
*/
bool idCollisionQueryJobManager::ExecuteJobs( void ( *function )( void* ), void* jobs,
		int jobStride, int numJobs, bool allowParallel )
{
	const bool runParallel = allowParallel && cm_parallelCollision.GetBool() &&
							 parallelJobManager != NULL && numJobs >= cm_parallelCollisionMinModels.GetInteger() &&
							 numJobs <= CM_MAX_BATCHED_COLLISION_MODELS;

	if( runParallel )
	{
		if( jobList == NULL )
		{
			jobList = parallelJobManager->AllocJobList( JOBLIST_UTILITY,
					  JOBLIST_PRIORITY_MEDIUM, CM_MAX_BATCHED_COLLISION_MODELS, 0, NULL );
		}
		for( int i = 0; i < numJobs; ++i )
		{
			jobList->AddJob( function, static_cast<byte*>( jobs ) + i * jobStride );
		}
		jobList->Submit();
		jobList->Wait();
	}
	else
	{
		for( int i = 0; i < numJobs; ++i )
		{
			function( static_cast<byte*>( jobs ) + i * jobStride );
		}
	}
	return runParallel;
}

/*
==================
idCollisionQueryJobManager::Translation
==================
*/
bool idCollisionQueryJobManager::Translation( idCollisionModelManagerLocal& manager,
		trace_t* results, const int numModels, const idVec3& start, const idVec3& end,
		const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
		const idPositionedCollisionModel* positionedModels, bool allowParallel )
{
	jobs.SetNum( numModels );
	for( int i = 0; i < numModels; ++i )
	{
		cmExecuteQueryJob_t& job = jobs[i];
		job.manager = &manager;
		job.traceResult = &results[i];
		job.integerResult = NULL;
		job.parms.type = CM_QUERY_TRANSLATION;
		job.parms.start = start;
		job.parms.end = end;
		job.parms.traceModel = trm;
		job.parms.traceModelAxis = trmAxis;
		job.parms.contentsMask = contentMask;
		job.parms.model = positionedModels[i].model;
		job.parms.modelOrigin = positionedModels[i].origin;
		job.parms.modelAxis = positionedModels[i].axis;
	}
	return ExecuteJobs( CM_ExecuteQueryJob, jobs.Ptr(), sizeof( jobs[0] ), numModels, allowParallel );
}

/*
==================
idCollisionQueryJobManager::TranslationQueries
==================
*/
bool idCollisionQueryJobManager::TranslationQueries( idCollisionModelManagerLocal& manager,
		idCollisionTranslationQuery* queries, int numQueries, bool allowParallel )
{
	jobs.SetNum( numQueries );
	for( int i = 0; i < numQueries; ++i )
	{
		cmExecuteQueryJob_t& job = jobs[i];
		job.manager = &manager;
		job.traceResult = queries[i].result;
		job.integerResult = NULL;
		job.parms.type = CM_QUERY_TRANSLATION;
		job.parms.start = queries[i].start;
		job.parms.end = queries[i].end;
		job.parms.traceModel = queries[i].traceModel;
		job.parms.traceModelAxis = queries[i].traceModelAxis;
		job.parms.contentsMask = queries[i].contentsMask;
		job.parms.model = queries[i].model;
		job.parms.modelOrigin = queries[i].modelOrigin;
		job.parms.modelAxis = queries[i].modelAxis;
	}
	return ExecuteJobs( CM_ExecuteQueryJob, jobs.Ptr(), sizeof( jobs[0] ), numQueries, allowParallel );
}

/*
==================
idCollisionQueryJobManager::Rotation
==================
*/
bool idCollisionQueryJobManager::Rotation( idCollisionModelManagerLocal& manager,
		trace_t* results, const int numModels, const idVec3& start, const idRotation& rotation,
		const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
		const idPositionedCollisionModel* positionedModels, bool allowParallel )
{
	jobs.SetNum( numModels );
	for( int i = 0; i < numModels; ++i )
	{
		cmExecuteQueryJob_t& job = jobs[i];
		job.manager = &manager;
		job.traceResult = &results[i];
		job.integerResult = NULL;
		job.parms.type = CM_QUERY_ROTATION;
		job.parms.start = start;
		job.parms.rotation = &rotation;
		job.parms.traceModel = trm;
		job.parms.traceModelAxis = trmAxis;
		job.parms.contentsMask = contentMask;
		job.parms.model = positionedModels[i].model;
		job.parms.modelOrigin = positionedModels[i].origin;
		job.parms.modelAxis = positionedModels[i].axis;
	}
	return ExecuteJobs( CM_ExecuteQueryJob, jobs.Ptr(), sizeof( jobs[0] ), numModels, allowParallel );
}

/*
==================
idCollisionQueryJobManager::RotationQueries
==================
*/
bool idCollisionQueryJobManager::RotationQueries( idCollisionModelManagerLocal& manager,
		idCollisionRotationQuery* queries, int numQueries, bool allowParallel )
{
	jobs.SetNum( numQueries );
	for( int i = 0; i < numQueries; ++i )
	{
		cmExecuteQueryJob_t& job = jobs[i];
		job.manager = &manager;
		job.traceResult = queries[i].result;
		job.integerResult = NULL;
		job.rotationStorage = queries[i].rotation;
		job.parms.type = CM_QUERY_ROTATION;
		job.parms.start = queries[i].start;
		job.parms.rotation = &job.rotationStorage;
		job.parms.traceModel = queries[i].traceModel;
		job.parms.traceModelAxis = queries[i].traceModelAxis;
		job.parms.contentsMask = queries[i].contentsMask;
		job.parms.model = queries[i].model;
		job.parms.modelOrigin = queries[i].modelOrigin;
		job.parms.modelAxis = queries[i].modelAxis;
	}
	return ExecuteJobs( CM_ExecuteQueryJob, jobs.Ptr(), sizeof( jobs[0] ), numQueries, allowParallel );
}

/*
==================
idCollisionQueryJobManager::Contents
==================
*/
bool idCollisionQueryJobManager::Contents( idCollisionModelManagerLocal& manager, int* results,
		const int numModels, const idVec3& start, const idTraceModel* trm,
		const idMat3& trmAxis, int contentMask,
		const idPositionedCollisionModel* positionedModels, bool allowParallel )
{
	jobs.SetNum( numModels );
	for( int i = 0; i < numModels; ++i )
	{
		cmExecuteQueryJob_t& job = jobs[i];
		job.manager = &manager;
		job.traceResult = NULL;
		job.integerResult = &results[i];
		job.parms.type = CM_QUERY_CONTENTS;
		job.parms.start = start;
		job.parms.end = start;
		job.parms.traceModel = trm;
		job.parms.traceModelAxis = trmAxis;
		job.parms.contentsMask = contentMask;
		job.parms.model = positionedModels[i].model;
		job.parms.modelOrigin = positionedModels[i].origin;
		job.parms.modelAxis = positionedModels[i].axis;
	}
	return ExecuteJobs( CM_ExecuteQueryJob, jobs.Ptr(), sizeof( jobs[0] ), numModels, allowParallel );
}

/*
==================
idCollisionQueryJobManager::Contacts
==================
*/
bool idCollisionQueryJobManager::Contacts( idCollisionModelManagerLocal& manager,
		contactInfo_t* contacts, int* numContacts, const int maxContactsPerModel,
		const int numModels, const idVec3& start, const idVec6& dir, const float depth,
		const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
		const idPositionedCollisionModel* positionedModels, bool allowParallel )
{
	jobs.SetNum( numModels );
	for( int i = 0; i < numModels; ++i )
	{
		cmExecuteQueryJob_t& job = jobs[i];
		job.manager = &manager;
		job.traceResult = NULL;
		job.integerResult = &numContacts[i];
		job.parms.type = CM_QUERY_CONTACTS;
		job.parms.start = start;
		job.parms.direction = dir;
		job.parms.depth = depth;
		job.parms.traceModel = trm;
		job.parms.traceModelAxis = trmAxis;
		job.parms.contentsMask = contentMask;
		job.parms.model = positionedModels[i].model;
		job.parms.modelOrigin = positionedModels[i].origin;
		job.parms.modelAxis = positionedModels[i].axis;
		job.parms.contacts = contacts + i * maxContactsPerModel;
		job.parms.maxContacts = maxContactsPerModel;
	}
	return ExecuteJobs( CM_ExecuteQueryJob, jobs.Ptr(), sizeof( jobs[0] ), numModels, allowParallel );
}

/*
==================
idCollisionQueryJobManager::WaitForAllQueries
==================
*/
void idCollisionQueryJobManager::WaitForAllQueries()
{
	if( jobList != NULL && jobList->IsSubmitted() )
	{
		jobList->Wait();
	}
}

/*
==================
idCollisionQueryJobManager::Shutdown
==================
*/
void idCollisionQueryJobManager::Shutdown()
{
	jobs.Clear();
	if( jobList == NULL )
	{
		return;
	}
	WaitForAllQueries();
	if( parallelJobManager != NULL )
	{
		parallelJobManager->FreeJobList( jobList );
	}
	jobList = NULL;
}
