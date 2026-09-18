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

/*
============
AAS2PathLogSample
============
*/
static bool AAS2PathLogSample( int requiredLevel )
{
	if( aas2_debugPathing.GetInteger() < requiredLevel )
	{
		return false;
	}
	static int interval = -1;
	static int count = 0;
	const int nowInterval = sys->GetMilliseconds() / 1000;
	if( interval != nowInterval )
	{
		interval = nowInterval;
		count = 0;
	}
	return count++ < 64;
}

/*
============
InitObstacleRoute
============
*/
static void InitObstacleRoute( idAAS2Path& path, int areaNum, const idVec3& origin, const idVec3& goal )
{
	path.obstacleRoute.numAreas = 1;
	path.obstacleRoute.endAlignDir.Zero();
	path.obstacleRoute.endTurnRadius = 0.0f;
	for( int i = 0; i < 11; ++i )
	{
		path.obstacleRoute.areas[i].areaNum = areaNum;
		path.obstacleRoute.areas[i].start = origin;
		path.obstacleRoute.areas[i].end = origin;
	}
	path.obstacleRoute.areas[0].end = goal;
	path.obstacleGoal = goal;
	path.obstacleAreaNum = areaNum;
}

/*
============
EdgePathSplitPoint
============
*/
static bool EdgePathSplitPoint( const idAAS2File& file, int edgeNum, const idVec3& planeNormal, float planeDistance, idVec3& split, bool& coplanar )
{
	coplanar = false;
	if( edgeNum <= 0 || edgeNum >= file.GetNumEdges() )
	{
		return false;
	}

	const aas2Edge_t& edge = file.GetEdge( edgeNum );
	const idVec3& first = file.GetVertex( edge.vertexNum[0] );
	const idVec3& second = file.GetVertex( edge.vertexNum[1] );
	const float firstDistance = first * planeNormal - planeDistance;
	const float secondDistance = second * planeNormal - planeDistance;
	const float epsilon = 0.01f;

	if( idMath::Fabs( firstDistance ) <= epsilon && idMath::Fabs( secondDistance ) <= epsilon )
	{
		coplanar = true;
		return true;
	}

	if( ( firstDistance < -epsilon && secondDistance < -epsilon ) ||
			( firstDistance > epsilon && secondDistance > epsilon ) )
	{
		return false;
	}

	const float denominator = firstDistance - secondDistance;
	if( idMath::Fabs( denominator ) <= 1.0e-8f )
	{
		return false;
	}

	const float fraction = idMath::ClampFloat( 0.0f, 1.0f, firstDistance / denominator );
	split = first + ( second - first ) * fraction;
	return true;
}

/*
============
FloorPathSplitPoint
============
*/
static bool FloorPathSplitPoint( const idAAS2File& file, int areaNum, const idVec3& pathNormal, float pathDistance, const idVec3& frontNormal, float frontDistance, bool closest, idVec3& bestSplit )
{
	if( areaNum <= 0 || areaNum >= file.GetNumAreas() )
	{
		return false;
	}
	const aas2Area_t& area = file.GetArea( areaNum );
	float bestDistance = closest ? idMath::INFINITUM : -0.1f;
	bool found = false;
	for( int i = 0; i < area.numEdges; ++i )
	{
		const int edgeNum = idMath::Abs( file.GetEdgeIndex( area.firstEdge + i ) );
		idVec3 split;
		bool coplanar = false;
		if( !EdgePathSplitPoint( file, edgeNum, pathNormal, pathDistance, split, coplanar ) )
		{
			continue;
		}
		if( coplanar )
		{
			const aas2Edge_t& edge = file.GetEdge( edgeNum );
			for( int endpoint = 0; endpoint < 2; ++endpoint )
			{
				const idVec3& point = file.GetVertex( edge.vertexNum[endpoint] );
				const float distance = point * frontNormal - frontDistance;
				if( closest ? ( distance >= -0.1f && distance < bestDistance ) : ( distance > bestDistance ) )
				{
					bestDistance = distance;
					bestSplit = point;
					found = true;
				}
			}
		}
		else
		{
			const float distance = split * frontNormal - frontDistance;
			if( closest ? ( distance >= -0.1f && distance < bestDistance ) : ( distance > bestDistance ) )
			{
				bestDistance = distance;
				bestSplit = split;
				found = true;
			}
		}
	}
	return found;
}

/*
============
idAAS2RuntimeLocal::WalkPathValidFromArea
============
*/
bool idAAS2RuntimeLocal::WalkPathValidFromArea( int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags, idVec3& endPos, int& endAreaNum ) const
{
	endPos = origin;
	endAreaNum = areaNum;

	if( areaNum <= 0 || areaNum >= file->GetNumAreas() ||
			file->GetArea( areaNum ).IsDisabled() )
	{
		return false;
	}

	const idVec3 delta = goalOrigin - origin;
	if( delta.LengthSqr() <= 1.0e-6f )
	{
		endPos = goalOrigin;
		return goalAreaNum == 0 || areaNum == goalAreaNum;
	}

	idVec3 frontNormal = delta;
	frontNormal.Normalize();
	idVec3 pathNormal = delta.Cross( file->GetSettings().gravityDir );
	if( pathNormal.Normalize() == 0.0f )
	{
		return false;
	}

	const float pathDistance = pathNormal * origin;
	const float farDistance = frontNormal * goalOrigin;
	float frontDistance = frontNormal * origin;
	int currentArea = areaNum;
	int recentAreas[4] = { areaNum, areaNum, areaNum, areaNum };
	int recentIndex = 0;
	const float maxStepSqr = Square( file->GetSettings().maxStepHeight + 0.1f );
	const idBounds& agentBounds = file->GetSettings().boundingBoxes[0];
	const float radiusX = idMath::Fabs( agentBounds[0].x ) > idMath::Fabs( agentBounds[1].x )
						  ? idMath::Fabs( agentBounds[0].x ) : idMath::Fabs( agentBounds[1].x );
	const float radiusY = idMath::Fabs( agentBounds[0].y ) > idMath::Fabs( agentBounds[1].y )
						  ? idMath::Fabs( agentBounds[0].y ) : idMath::Fabs( agentBounds[1].y );
	const float agentRadius = radiusX > radiusY ? radiusX : radiusY;
	float explicitGap = agentRadius < file->GetSettings().maxStepHeight
						? agentRadius : file->GetSettings().maxStepHeight;
	if( explicitGap < 0.2f )
	{
		explicitGap = 0.2f;
	}

	const float explicitGapSqr = Square( explicitGap + 0.1f );
	for( int iteration = 0; iteration < file->GetNumAreas(); ++iteration )
	{
		if( !FloorPathSplitPoint( *file, currentArea, pathNormal, pathDistance,
								  frontNormal, frontDistance, false, endPos ) )
		{
			endPos = origin;
		}
		endAreaNum = currentArea;
		if( endPos * frontNormal >= farDistance - 0.5f || currentArea == goalAreaNum )
		{
			endPos = goalOrigin;
			return true;
		}

		frontDistance = frontNormal * endPos;
		aas2Reachability_t* selectedReach = NULL;
		for( aas2Reachability_t* reach = file->GetArea( currentArea ).reach;
				reach != NULL; reach = reach->next )
		{
			if( reach->travelType != TFL_WALK || !TravelAllowed( *reach, travelFlags ) )
			{
				continue;
			}
			const int nextArea = reach->toAreaNum;
			if( nextArea <= 0 || nextArea >= file->GetNumAreas() ||
					file->GetArea( nextArea ).IsDisabled() ||
					( file->GetArea( nextArea ).flags & AAS2_AREA_LEDGE ) )
			{
				continue;
			}
			bool repeated = false;
			for( int i = 0; i < 4; ++i ) if( recentAreas[i] == nextArea )
				{
					repeated = true;
					break;
				}
			if( repeated )
			{
				continue;
			}
			idVec3 nextSplit;
			if( !FloorPathSplitPoint( *file, nextArea, pathNormal, pathDistance,
									  frontNormal, frontDistance, true, nextSplit ) )
			{
				continue;
			}
			const idVec3 between = endPos - nextSplit;
			const idVec3 gravityDelta = file->GetSettings().gravityDir *
										( between * file->GetSettings().gravityDir );
			if( gravityDelta.LengthSqr() > maxStepSqr )
			{
				continue;
			}
			const idVec3 horizontalDelta = between - gravityDelta;
			const float allowedGapSqr = reach->edgeNum != 0 ? Square( 0.2f ) : explicitGapSqr;
			if( horizontalDelta.LengthSqr() > allowedGapSqr )
			{
				continue;
			}
			selectedReach = reach;
			break;
		}
		if( selectedReach == NULL )
		{
			return false;
		}
		recentAreas[recentIndex] = currentArea;
		recentIndex = ( recentIndex + 1 ) & 3;
		currentArea = selectedReach->toAreaNum;
	}
	return false;
}

/*
============
idAAS2RuntimeLocal::WalkPathValid
============
*/
bool idAAS2RuntimeLocal::WalkPathValid( int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags, idVec3& endPos, int& endAreaNum ) const
{
	if( file == NULL )
	{
		endPos = goalOrigin;
		endAreaNum = 0;
		return true;
	}
	int candidates[16];
	const float pointHeight = file->GetSettings().boundingBoxes[0][1].z -
							  file->GetSettings().boundingBoxes[0][0].z + file->GetSettings().maxStepHeight;
	int candidateCount = file->PointAreaCandidates( origin, pointHeight, candidates, 16 );
	bool haveRequestedArea = false;

	for( int i = 0; i < candidateCount; ++i ) if( candidates[i] == areaNum )
		{
			haveRequestedArea = true;
		}

	if( !haveRequestedArea && candidateCount < 16 )
	{
		for( int i = candidateCount; i > 0; --i )
		{
			candidates[i] = candidates[i - 1];
		}
		candidates[0] = areaNum;
		++candidateCount;
	}

	idVec3 bestEnd = origin;
	int bestArea = areaNum;
	float bestProgress = -idMath::INFINITUM;
	const idVec3 direction = goalOrigin - origin;

	for( int i = 0; i < candidateCount; ++i )
	{
		idVec3 candidateEnd;
		int candidateArea;
		if( WalkPathValidFromArea( candidates[i], origin, goalAreaNum,
								   goalOrigin, travelFlags, candidateEnd, candidateArea ) )
		{
			endPos = candidateEnd;
			endAreaNum = candidateArea;
			return true;
		}

		const float progress = ( candidateEnd - origin ) * direction;
		if( progress > bestProgress )
		{
			bestProgress = progress;
			bestEnd = candidateEnd;
			bestArea = candidateArea;
		}
	}
	endPos = bestEnd;
	endAreaNum = bestArea;
	return false;
}

/*
============
idAAS2RuntimeLocal::SubSampleWalkPath
============
*/
idVec3 idAAS2RuntimeLocal::SubSampleWalkPath( int areaNum, const idVec3& origin, int pathAreaNum, const idVec3& pathStart, const idVec3& pathEnd, int travelFlags, int& endAreaNum ) const
{
	float step = 0.25f;
	float fraction = 0.5f;
	idVec3 acceptedPoint = pathStart;
	endAreaNum = pathAreaNum;
	const idVec3 delta = pathEnd - pathStart;
	const float lengthSqr = delta.LengthSqr();

	for( int iteration = 0; iteration < 8 && step * lengthSqr > 16.0f;
			++iteration, step *= 0.5f )
	{
		const idVec3 point = pathStart + delta * fraction;
		idVec3 traceEnd;
		int traceArea = pathAreaNum;
		if( WalkPathValid( areaNum, origin, 0, point,
						   travelFlags, traceEnd, traceArea ) )
		{
			acceptedPoint = point;
			endAreaNum = traceArea;
			fraction += step;
		}
		else
		{
			fraction -= step;
		}
	}
	return acceptedPoint;
}

/*
============
idAAS2RuntimeLocal::WalkPathToGoal
============
*/
bool idAAS2RuntimeLocal::WalkPathToGoal( idAAS2Path& path, int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags ) const
{
	path.type = AAS2_PATHTYPE_WALK;
	path.moveGoal = origin;
	path.moveAreaNum = areaNum;
	path.secondaryGoal = origin;
	path.reachability = NULL;

	InitObstacleRoute( path, areaNum, origin, goalOrigin );

	if( file == NULL )
	{
		return false;
	}

	if( areaNum == goalAreaNum )
	{
		if( AAS2PathLogSample( 2 ) )
			common->Printf(
				"[AAS2DBG] walk same-area=%d from=(%.1f %.1f %.1f) to=(%.1f %.1f %.1f)\n",
				areaNum, origin.x, origin.y, origin.z,
				goalOrigin.x, goalOrigin.y, goalOrigin.z );

		path.moveGoal = goalOrigin;
		path.moveAreaNum = goalAreaNum;
		return true;
	}

	int routeTime;
	aas2Reachability_t* route[10];
	int routeCount = 0;
	const bool routeFound = RouteToGoalAreaChain( areaNum, origin, goalAreaNum, travelFlags, routeTime, route, 10, routeCount );

	if( routeFound && routeCount > 0 )
	{
		path.obstacleRoute.numAreas = 1;
		path.obstacleRoute.areas[0].areaNum = areaNum;
		path.obstacleRoute.areas[0].start = origin;
		for( int i = 0; i < routeCount && i < 10; ++i )
		{
			path.obstacleRoute.areas[i].end = route[i]->start;
			path.obstacleRoute.areas[i + 1].areaNum = route[i]->toAreaNum;
			path.obstacleRoute.areas[i + 1].start = route[i]->end;
			path.obstacleRoute.areas[i + 1].end = route[i]->end;
			path.obstacleRoute.numAreas = i + 2;
		}
		const int last = path.obstacleRoute.numAreas - 1;
		if( route[routeCount - 1]->toAreaNum == goalAreaNum )
		{
			path.obstacleRoute.areas[last].end = goalOrigin;
		}
		path.obstacleGoal = path.obstacleRoute.areas[last].end;
		path.obstacleAreaNum = path.obstacleRoute.areas[last].areaNum;
	}

	if( routeFound && routeCount == 0 )
	{
		path.moveGoal = goalOrigin;
		path.moveAreaNum = goalAreaNum;
		return true;
	}

	if( !routeFound )
	{
		if( AAS2PathLogSample( 1 ) )
		{
			int outgoing = 0, usable = 0, disabledTargets = 0;
			for( aas2Reachability_t* candidate = file->GetArea( areaNum ).reach;
					candidate != NULL; candidate = candidate->next )
			{
				++outgoing;
				if( candidate->toAreaNum > 0 && candidate->toAreaNum < file->GetNumAreas() &&
						file->GetArea( candidate->toAreaNum ).IsDisabled() )
				{
					++disabledTargets;
				}
				else if( TravelAllowed( *candidate, travelFlags ) )
				{
					++usable;
				}
			}

			common->Printf( "[AAS2DBG] walk NO-ROUTE area=%d goal=%d flags=%x "
							"goalDisabled=%d outgoing=%d usable=%d disabledTargets=%d "
							"from=(%.1f %.1f %.1f) to=(%.1f %.1f %.1f)\n",
							areaNum, goalAreaNum, travelFlags,
							file->GetArea( goalAreaNum ).IsDisabled() ? 1 : 0,
							outgoing, usable, disabledTargets,
							origin.x, origin.y, origin.z, goalOrigin.x, goalOrigin.y, goalOrigin.z );
		}
		return false;
	}

	aas2Reachability_t* reach = route[0];
	aas2Reachability_t* firstSelectedReach = reach;
	const int firstRouteTime = routeTime;
	path.moveGoal = reach->start;
	path.moveAreaNum = areaNum;
	path.secondaryGoal = reach->end;

	if( reach->travelType == BIT( 3 ) )
	{
		path.type = AAS2_PATHTYPE_WALKOFFLEDGE;
		path.reachability = reach;
	}
	else if( reach->travelType == BIT( 4 ) )
	{
		path.type = AAS2_PATHTYPE_BARRIERJUMP;
		path.reachability = reach;
	}
	else if( reach->travelType == BIT( 5 ) )
	{
		path.type = AAS2_PATHTYPE_JUMP;
		path.reachability = reach;
	}
	else
	{
		// Native AAS2 looks ahead through a short run of ordinary walk reaches and
		// steers to the furthest point connected by a valid floor segment.  Without
		// this, actors visit every triangulation portal and visibly zigzag.
		int walkReachCount = 0;
		while( walkReachCount < routeCount && route[walkReachCount]->travelType == TFL_WALK )
		{
			++walkReachCount;
		}
		idVec3 validEnd;
		int validArea = areaNum;
		if( walkReachCount == routeCount && route[walkReachCount - 1]->toAreaNum == goalAreaNum &&
				WalkPathValid( areaNum, origin,
							   goalAreaNum, goalOrigin, travelFlags, validEnd, validArea ) )
		{
			path.moveGoal = goalOrigin;
			path.moveAreaNum = goalAreaNum;
		}
		else
		{
			int foundIndex = -1;
			for( int i = walkReachCount - 1; i >= 0; --i )
			{
				if( WalkPathValid( areaNum, origin, route[i]->toAreaNum,
								   route[i]->end, travelFlags, validEnd, validArea ) )
				{
					foundIndex = i;
					break;
				}
			}

			if( foundIndex >= 0 )
			{
				path.moveGoal = route[foundIndex]->end;
				path.moveAreaNum = route[foundIndex]->toAreaNum;
				path.secondaryGoal = route[foundIndex]->end;
				if( foundIndex + 1 < routeCount )
				{
					path.moveGoal = SubSampleWalkPath( areaNum, origin,
													   route[foundIndex]->toAreaNum, route[foundIndex]->end,
													   route[foundIndex + 1]->start, travelFlags, path.moveAreaNum );
				}
				else if( route[foundIndex]->toAreaNum == goalAreaNum )
				{
					path.moveGoal = SubSampleWalkPath( areaNum, origin,
													   route[foundIndex]->toAreaNum, route[foundIndex]->end,
													   goalOrigin, travelFlags, path.moveAreaNum );
				}
			}
		}
	}

	if( AAS2PathLogSample( 2 ) )
		common->Printf(
			"[AAS2DBG] walk route area=%d goal=%d first=%d>%d type=%x time=%d "
			"from=(%.1f %.1f %.1f) reach=(%.1f %.1f %.1f)>(%.1f %.1f %.1f) "
			"moveArea=%d move=(%.1f %.1f %.1f)\n",
			areaNum, goalAreaNum, firstSelectedReach->fromAreaNum, firstSelectedReach->toAreaNum,
			firstSelectedReach->travelType, firstRouteTime,
			origin.x, origin.y, origin.z,
			firstSelectedReach->start.x, firstSelectedReach->start.y, firstSelectedReach->start.z,
			firstSelectedReach->end.x, firstSelectedReach->end.y, firstSelectedReach->end.z,
			path.moveAreaNum, path.moveGoal.x, path.moveGoal.y, path.moveGoal.z );
	return true;
}

/*
============
idAAS2RuntimeLocal::FlyPathValid
============
*/
bool idAAS2RuntimeLocal::FlyPathValid( int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags, idVec3& endPos, int& endAreaNum ) const
{
	endPos = origin;
	endAreaNum = areaNum;
	if( file == NULL )
	{
		endPos = goalOrigin;
		endAreaNum = 0;
		return true;
	}

	// AAS2 is a surface graph, so flying validity is a 3D corridor projected
	// onto its navigable surfaces. Unlike walking, height above the floor and
	// floor step/slope changes do not invalidate the segment.
	const idVec3 delta = goalOrigin - origin;
	const int samples = idMath::ClampInt( 1, 2048, static_cast<int>( delta.Length() / 16.0f ) + 1 );
	const float maxFlyHeight = 65536.0f;
	for( int i = 1; i <= samples; ++i )
	{
		const float fraction = static_cast<float>( i ) / samples;
		const idVec3 point = origin + delta * fraction;
		const int sampleArea = file->PointAreaNumMaxHeight( point, maxFlyHeight );
		if( sampleArea <= 0 || file->GetArea( sampleArea ).IsDisabled() )
		{
			endPos = origin + delta * ( static_cast<float>( i - 1 ) / samples );
			return false;
		}
		endPos = point;
		endAreaNum = sampleArea;
	}
	( void )goalAreaNum;
	( void )travelFlags;
	return true;
}

/*
============
idAAS2RuntimeLocal::FlyPathToGoal
============
*/
bool idAAS2RuntimeLocal::FlyPathToGoal( idAAS2Path& path, int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags ) const
{
	path.type = AAS2_PATHTYPE_FLY;
	path.moveGoal = origin;
	path.moveAreaNum = areaNum;
	path.secondaryGoal = origin;
	path.reachability = NULL;

	InitObstacleRoute( path, areaNum, origin, goalOrigin );

	if( file == NULL )
	{
		path.moveGoal = goalOrigin;
		return true;
	}

	if( areaNum <= 0 || goalAreaNum <= 0 )
	{
		return false;
	}

	idVec3 endPos;
	int endAreaNum;
	if( FlyPathValid( areaNum, origin, goalAreaNum, goalOrigin, travelFlags, endPos, endAreaNum ) )
	{
		if( AAS2PathLogSample( 2 ) )
			common->Printf(
				"[AAS2DBG] fly direct area=%d goal=%d from=(%.1f %.1f %.1f) to=(%.1f %.1f %.1f)\n",
				areaNum, goalAreaNum, origin.x, origin.y, origin.z,
				goalOrigin.x, goalOrigin.y, goalOrigin.z );

		path.moveGoal = goalOrigin;
		path.moveAreaNum = goalAreaNum;
		return true;
	}

	const idVec3 routeDelta = goalOrigin - origin;
	const float routeXYLengthSqr = routeDelta.x * routeDelta.x + routeDelta.y * routeDelta.y;
	int currentArea = areaNum;
	idVec3 routeOrigin = origin;
	int recentAreas[4] = { areaNum, areaNum, areaNum, areaNum };
	int recentIndex = 0;

	for( int iteration = 0; iteration < 32 && currentArea != goalAreaNum; ++iteration )
	{
		int routeTime;
		aas2Reachability_t* reach = NULL;
		if( !RouteToGoalArea( currentArea, routeOrigin, goalAreaNum,
							  travelFlags, routeTime, &reach ) || reach == NULL )
		{
			break;
		}

		idVec3 waypoint = reach->end;
		if( routeXYLengthSqr > 1.0f )
		{
			const float fraction = idMath::ClampFloat( 0.0f, 1.0f,
								   ( ( waypoint.x - origin.x ) * routeDelta.x + ( waypoint.y - origin.y ) * routeDelta.y ) / routeXYLengthSqr );
			waypoint.z = origin.z + routeDelta.z * fraction;
		}
		else
		{
			waypoint.z = origin.z;
		}

		if( !FlyPathValid( areaNum, origin, 0, waypoint, travelFlags, endPos, endAreaNum ) )
		{
			// Return the furthest safe point found on the true 3D fly corridor.
			if( endPos != origin )
			{
				path.moveGoal = endPos;
				path.moveAreaNum = endAreaNum;
				return true;
			}
			return false;
		}
		path.moveGoal = waypoint;
		path.moveAreaNum = reach->toAreaNum;
		path.secondaryGoal = reach->end;
		currentArea = reach->toAreaNum;
		routeOrigin = waypoint;

		if( currentArea == goalAreaNum )
		{
			if( FlyPathValid( areaNum, origin, goalAreaNum, goalOrigin, travelFlags, endPos, endAreaNum ) )
			{
				path.moveGoal = goalOrigin;
				path.moveAreaNum = goalAreaNum;
			}
			return true;
		}

		for( int i = 0; i < 4; ++i ) if( recentAreas[i] == currentArea )
			{
				return true;
			}

		recentAreas[recentIndex] = currentArea;
		recentIndex = ( recentIndex + 1 ) & 3;
	}

	const bool found = path.moveGoal != origin;
	if( AAS2PathLogSample( found ? 2 : 1 ) )
		common->Printf(
			"[AAS2DBG] fly %s area=%d goal=%d moveArea=%d move=(%.1f %.1f %.1f)\n",
			found ? "route" : "NO-ROUTE", areaNum, goalAreaNum, path.moveAreaNum,
			path.moveGoal.x, path.moveGoal.y, path.moveGoal.z );

	return found;
}

/*
============
idAAS2RuntimeLocal::ShowWalkPath
============
*/
void idAAS2RuntimeLocal::ShowWalkPath( const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin ) const
{
	if( file == NULL )
	{
		return;
	}

	const int startArea = file->PointAreaNum( origin );
	int time;
	aas2Reachability_t* reach = NULL;
	if( RouteToGoalArea( startArea, origin, goalAreaNum, ~0, time, &reach ) && reach != NULL )
	{
		common->Printf( "AAS2 path %d -> %d via %d -> %d (%d)\n", startArea, goalAreaNum,
						reach->fromAreaNum, reach->toAreaNum, time );
	}
	else
	{
		common->Printf( "AAS2 no path %d -> %d\n", startArea, goalAreaNum );
	}

	( void )goalOrigin;
}

/*
============
idAAS2RuntimeLocal::ShowFlyPath
============
*/
void idAAS2RuntimeLocal::ShowFlyPath( const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin ) const
{
	if( file == NULL )
	{
		return;
	}

	idAAS2Path path;
	const int startArea = file->PointAreaNumMaxHeight( origin, 65536.0f );
	if( FlyPathToGoal( path, startArea, origin, goalAreaNum, goalOrigin, TFL_WALK | TFL_AIR | TFL_FLY ) )
	{
		common->Printf( "AAS2 fly path %d -> %d, next (%g %g %g)\n", startArea, goalAreaNum,
						path.moveGoal.x, path.moveGoal.y, path.moveGoal.z );
	}
	else
	{
		common->Printf( "AAS2 no fly path %d -> %d\n", startArea, goalAreaNum );
	}
}
