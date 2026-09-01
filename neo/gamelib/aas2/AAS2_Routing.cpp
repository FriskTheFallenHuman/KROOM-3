/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2026 Justin Marshall(justinmarshall20@gmail.com)

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

#include "AAS2_Local.h"

#include <queue>

/*
============
routeNode_t
============
*/
struct routeNode_t
{
	int areaNum;
	int travelTime;
	bool operator<( const routeNode_t& other ) const
	{
		return travelTime > other.travelTime;
	}
};

/*
============
PointTravelTime
============
*/
static int PointTravelTime( const idVec3& from, const idVec3& to )
{
	return idMath::ClampInt( 1, 65535, static_cast<int>( ( to - from ).Length() ) + 1 );
}

/*
============
AreaTransitionTime
============
*/
static int AreaTransitionTime( const idAAS2File& file, int toAreaNum, const idVec3& fromPoint, const aas2Reachability_t& reach )
{
	// A reachability's stored time describes only movement across the portal.
	// Shared-edge walk reaches have identical start/end points and therefore a
	// time of one. Include travel inside both areas, otherwise Dijkstra minimizes
	// polygon count and produces severe zigzags on an uneven triangulation.
	const int portalTime = reach.travelTime > 0 ? reach.travelTime :
						   PointTravelTime( reach.start, reach.end );
	const idVec3& destinationCenter = file.AreaCenter( toAreaNum );
	const int toPortal = PointTravelTime( fromPoint, reach.start );
	const int fromPortal = PointTravelTime( reach.end, destinationCenter );
	if( toPortal > INT_MAX - portalTime ||
			toPortal + portalTime > INT_MAX - fromPortal )
	{
		return INT_MAX;
	}
	return toPortal + portalTime + fromPortal;
}

/*
============
idAAS2RuntimeLocal::RouteToGoalAreaChain
============
*/
bool idAAS2RuntimeLocal::RouteToGoalAreaChain( int startAreaNum, const idVec3& origin, int goalAreaNum, int travelFlags, int& travelTime, aas2Reachability_t** route, int maxRoute, int& routeCount ) const
{
	travelTime = 0;
	routeCount = 0;

	if( route != NULL ) for( int i = 0; i < maxRoute; ++i )
		{
			route[i] = NULL;
		}

	if( file == NULL || startAreaNum <= 0 || goalAreaNum <= 0 || startAreaNum >= file->GetNumAreas() || goalAreaNum >= file->GetNumAreas() )
	{
		return false;
	}

	if( startAreaNum == goalAreaNum )
	{
		return true;
	}

	const int areaCount = file->GetNumAreas();
	std::vector<int> distance( areaCount, INT_MAX );
	std::vector<aas2Reachability_t*> incomingReach( areaCount, NULL );
	std::priority_queue<routeNode_t> pending;
	distance[startAreaNum] = 0;
	pending.push( { startAreaNum, 0 } );

	// AAS2 is a surface mesh and points on triangulation seams can belong to
	// several coplanar areas.  Seed all of them into the same search.  Picking
	// one by hash order creates local routing loops when the actor crosses a
	// portal but localization returns a different overlapping triangle.
	int startCandidates[16];
	const float pointHeight = file->GetSettings().boundingBoxes[0][1].z -
							  file->GetSettings().boundingBoxes[0][0].z + file->GetSettings().maxStepHeight;
	const int numStartCandidates = file->PointAreaCandidates(
									   origin, pointHeight, startCandidates, 16 );

	for( int i = 0; i < numStartCandidates; ++i )
	{
		const int candidate = startCandidates[i];
		if( candidate <= 0 || candidate >= areaCount ||
				file->GetArea( candidate ).IsDisabled() || distance[candidate] == 0 )
		{
			continue;
		}

		distance[candidate] = 0;
		pending.push( { candidate, 0 } );
	}

	while( !pending.empty() )
	{
		const routeNode_t current = pending.top();
		pending.pop();

		if( current.travelTime != distance[current.areaNum] )
		{
			continue;
		}

		if( current.areaNum == goalAreaNum )
		{
			travelTime = current.travelTime;
			if( route != NULL && maxRoute > 0 )
			{
				std::vector<aas2Reachability_t*> reverseRoute;
				int routeArea = goalAreaNum;

				while( routeArea > 0 && routeArea < areaCount && incomingReach[routeArea] != NULL )
				{
					aas2Reachability_t* step = incomingReach[routeArea];
					reverseRoute.push_back( step );
					routeArea = step->fromAreaNum;
				}

				const int available = static_cast<int>( reverseRoute.size() );
				routeCount = available < maxRoute ? available : maxRoute;
				for( int i = 0; i < routeCount; ++i )
				{
					route[i] = reverseRoute[available - 1 - i];
				}
			}
			return true;
		}

		const aas2Area_t& area = file->GetArea( current.areaNum );
		const idVec3& routePoint = incomingReach[current.areaNum] == NULL ?
			origin : file->AreaCenter( current.areaNum );

		for( aas2Reachability_t* candidate = area.reach; candidate != NULL; candidate = candidate->next )
		{
			if( !TravelAllowed( *candidate, travelFlags ) )
			{
				continue;
			}
			const int destination = candidate->toAreaNum;
			if( destination <= 0 || destination >= areaCount || file->GetArea( destination ).IsDisabled() )
			{
				continue;
			}
			const int cost = AreaTransitionTime( *file, destination,
												 routePoint, *candidate );
			if( current.travelTime <= INT_MAX - cost && current.travelTime + cost < distance[destination] )
			{
				distance[destination] = current.travelTime + cost;
				incomingReach[destination] = candidate;
				pending.push( { destination, distance[destination] } );
			}
		}
	}
	return false;
}

/*
============
idAAS2RuntimeLocal::RouteToGoalArea
============
*/
bool idAAS2RuntimeLocal::RouteToGoalArea( int startAreaNum, const idVec3& origin, int goalAreaNum, int travelFlags, int& travelTime, aas2Reachability_t** reach ) const
{
	aas2Reachability_t* firstReach = NULL;
	int routeCount = 0;
	const bool found = RouteToGoalAreaChain( startAreaNum, origin, goalAreaNum,
					   travelFlags, travelTime, &firstReach, 1, routeCount );
	if( reach != NULL )
	{
		*reach = routeCount > 0 ? firstReach : NULL;
	}
	return found;
}

/*
============
idAAS2RuntimeLocal::TravelTimeToGoalArea
============
*/
int idAAS2RuntimeLocal::TravelTimeToGoalArea( int areaNum, const idVec3& origin, int goalAreaNum, int travelFlags ) const
{
	int time;
	return RouteToGoalArea( areaNum, origin, goalAreaNum, travelFlags, time, NULL ) ? time : 0;
}

/*
============
idAAS2RuntimeLocal::AreaBlockedByQueryObstacles
============
*/
bool idAAS2RuntimeLocal::AreaBlockedByQueryObstacles( int areaNum, const idAAS2Obstacle* obstacles, int numObstacles ) const
{
	if( obstacles == NULL || numObstacles <= 0 )
	{
		return false;
	}

	const idBounds bounds = file->AreaBounds( areaNum );

	for( int i = 0; i < numObstacles; ++i ) if( bounds.IntersectsBounds( obstacles[i].expAbsBounds ) )
		{
			return true;
		}
	return false;
}

/*
============
idAAS2RuntimeLocal::FindNearestGoal
============
*/
bool idAAS2RuntimeLocal::FindNearestGoal( idAAS2Goal& goal, int startAreaNum, const idVec3& origin, const idVec3& target, int travelFlags,
		idAAS2Obstacle* obstacles, int numObstacles, idAAS2Callback& callback ) const
{
	if( file == NULL || startAreaNum <= 0 || startAreaNum >= file->GetNumAreas() )
	{
		return false;
	}

	const int areaCount = file->GetNumAreas();
	std::vector<int> distance( areaCount, INT_MAX );
	std::priority_queue<routeNode_t> pending;
	distance[startAreaNum] = 0;
	pending.push( { startAreaNum, 0 } );
	int bestArea = 0;
	float bestTargetDistance = idMath::INFINITUM;

	while( !pending.empty() )
	{
		const routeNode_t current = pending.top();
		pending.pop();

		if( current.travelTime != distance[current.areaNum] )
		{
			continue;
		}

		if( !AreaBlockedByQueryObstacles( current.areaNum, obstacles, numObstacles ) && callback.TestArea( this, current.areaNum ) )
		{
			const float targetDistance = ( file->AreaCenter( current.areaNum ) - target ).LengthSqr();
			if( targetDistance < bestTargetDistance )
			{
				bestTargetDistance = targetDistance;
				bestArea = current.areaNum;
			}
			if( current.travelTime > 0 && bestArea != 0 )
			{
				break;
			}
		}

		const aas2Area_t& area = file->GetArea( current.areaNum );
		const idVec3& routePoint = current.areaNum == startAreaNum ?
								   origin : file->AreaCenter( current.areaNum );
		for( aas2Reachability_t* candidate = area.reach; candidate != NULL; candidate = candidate->next )
		{
			if( !TravelAllowed( *candidate, travelFlags ) )
			{
				continue;
			}
			const int destination = candidate->toAreaNum;
			if( destination <= 0 || destination >= areaCount || file->GetArea( destination ).IsDisabled() ||
					AreaBlockedByQueryObstacles( destination, obstacles, numObstacles ) )
			{
				continue;
			}
			const int cost = AreaTransitionTime( *file, destination,
												 routePoint, *candidate );
			if( current.travelTime <= INT_MAX - cost && current.travelTime + cost < distance[destination] )
			{
				distance[destination] = current.travelTime + cost;
				pending.push( { destination, distance[destination] } );
			}
		}
	}

	if( bestArea == 0 )
	{
		return false;
	}

	goal.areaNum = bestArea;
	goal.origin = file->AreaCenter( bestArea );
	file->PushPointIntoAreaNum( bestArea, goal.origin );
	return true;
}
