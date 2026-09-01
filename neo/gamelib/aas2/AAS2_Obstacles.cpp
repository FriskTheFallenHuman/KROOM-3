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
avoidRect_t
============
*/
struct avoidRect_t
{
	idBounds bounds;
	idEntity* entity;
};

/*
============
avoidNode_t
============
*/
struct avoidNode_t
{
	idVec3 point;
	int obstacle;
};

/*
============
SegmentIntersectsRect
============
*/
static bool SegmentIntersectsRect( const idVec3& start, const idVec3& end, const idBounds& bounds, float* entryFraction = NULL )
{
	const idVec3 delta = end - start;
	float enter = 0.0f;
	float leave = 1.0f;

	for( int axis = 0; axis < 2; ++axis )
	{
		if( idMath::Fabs( delta[axis] ) < 1.0e-6f )
		{
			if( start[axis] > bounds[0][axis] && start[axis] < bounds[1][axis] )
			{
				continue;
			}
			return false;
		}

		float first = ( bounds[0][axis] - start[axis] ) / delta[axis];
		float second = ( bounds[1][axis] - start[axis] ) / delta[axis];

		if( first > second )
		{
			const float swap = first;
			first = second;
			second = swap;
		}

		if( first > enter )
		{
			enter = first;
		}

		if( second < leave )
		{
			leave = second;
		}

		if( enter >= leave )
		{
			return false;
		}
	}

	if( leave <= 0.001f || enter >= 0.999f )
	{
		return false;
	}

	if( entryFraction != NULL )
	{
		*entryFraction = enter;
	}
	return true;
}

/*
============
SegmentClear
============
*/
static bool SegmentClear( const idVec3& start, const idVec3& end, const std::vector<avoidRect_t>& rects, int skipObstacle = -1 )
{
	for( size_t i = 0; i < rects.size(); ++i )
	{
		if( static_cast<int>( i ) == skipObstacle )
		{
			continue;
		}

		if( SegmentIntersectsRect( start, end, rects[i].bounds ) )
		{
			return false;
		}
	}
	return true;
}

/*
============
SegmentClear
============
*/
static void SetSeekPlane( idAAS2AvoidancePath& path, const idVec3& first, const idVec3& second )
{
	idVec3 direction = second - first;
	direction.z = 0.0f;

	if( direction.Normalize() < 1.0e-4f )
	{
		path.seekPosPlane.Zero();
		return;
	}

	path.seekPosPlane.SetNormal( direction );
	path.seekPosPlane[3] = -( direction * first );
}

/*
============
idAAS2RuntimeLocal::FindPathAroundObstacles
============
*/
bool idAAS2RuntimeLocal::FindPathAroundObstacles( idAAS2AvoidancePath& avoidancePath, const idAAS2Path& routePath,
		int startAreaNum, const idVec3& startOrigin, const idBounds& actorBounds, const idVec3& lastDirection,
		const idAAS2AvoidanceObstacle* obstacles, int numObstacles ) const
{
	// moveGoal is the furthest point already proven reachable by TraceFloor.
	// obstacleGoal may extend farther along the multi-area route, but using it as
	// a straight destination would cut across bends until static wall edges are
	// also present in the compiled obstacle PVS.
	const idVec3& localGoal = routePath.moveGoal;
	avoidancePath.seekPos[0] = localGoal;
	avoidancePath.seekPos[1] = localGoal;
	avoidancePath.seekPosPlane.Zero();
	avoidancePath.firstObstacle = NULL;
	avoidancePath.hasValidPath = true;

	if( file == NULL || obstacles == NULL || numObstacles <= 0 )
	{
		return true;
	}

	const idVec3 actorSize = actorBounds[1] - actorBounds[0];
	idBounds queryBounds;
	queryBounds.Clear();
	queryBounds.AddPoint( startOrigin );
	queryBounds.AddPoint( localGoal );

	float queryRadius = file->GetSettings().obstaclePVSRadius;
	if( queryRadius > 256.0f )
	{
		queryRadius = 256.0f;
	}

	queryBounds.ExpandSelf( queryRadius );

	std::vector<avoidRect_t> rects;
	rects.reserve( 12 );

	for( int i = 0; i < numObstacles && rects.size() < 12; ++i )
	{
		if( obstacles[i].entity == NULL || !obstacles[i].bounds.IntersectsBounds( queryBounds ) )
		{
			continue;
		}
		avoidRect_t rect;

		// Minkowski-expand the obstacle by the actual moving actor bounds. This
		// retains asymmetric clip-model offsets and is not tied to an AAS profile.
		rect.bounds[0].x = obstacles[i].bounds[0].x - actorBounds[1].x - 1.0f;
		rect.bounds[0].y = obstacles[i].bounds[0].y - actorBounds[1].y - 1.0f;
		rect.bounds[1].x = obstacles[i].bounds[1].x - actorBounds[0].x + 1.0f;
		rect.bounds[1].y = obstacles[i].bounds[1].y - actorBounds[0].y + 1.0f;
		rect.bounds[0].z = -idMath::INFINITUM;
		rect.bounds[1].z = idMath::INFINITUM;
		rect.entity = obstacles[i].entity;
		rects.push_back( rect );
	}

	if( rects.empty() || SegmentClear( startOrigin, localGoal, rects ) )
	{
		return true;
	}

	int startObstacle = -1;
	for( size_t i = 0; i < rects.size(); ++i )
	{
		if( startOrigin.x > rects[i].bounds[0].x && startOrigin.x < rects[i].bounds[1].x &&
				startOrigin.y > rects[i].bounds[0].y && startOrigin.y < rects[i].bounds[1].y )
		{
			startObstacle = static_cast<int>( i );
			break;
		}
	}

	float firstFraction = idMath::INFINITUM;
	for( size_t i = 0; i < rects.size(); ++i )
	{
		float fraction;
		if( SegmentIntersectsRect( startOrigin, localGoal,
								   rects[i].bounds, &fraction ) && fraction < firstFraction )
		{
			firstFraction = fraction;
			avoidancePath.firstObstacle = rects[i].entity;
		}
	}

	std::vector<avoidNode_t> nodes;
	nodes.reserve( 2 + rects.size() * 4 );
	nodes.push_back( { startOrigin, -1 } );
	nodes.push_back( { localGoal, -1 } );
	const float cornerClearance = 2.0f;

	for( size_t i = 0; i < rects.size(); ++i )
	{
		for( int corner = 0; corner < 4; ++corner )
		{
			idVec3 point = startOrigin;
			point.x = rects[i].bounds[( corner & 1 ) != 0 ? 1 : 0].x +
					  ( ( corner & 1 ) != 0 ? cornerClearance : -cornerClearance );
			point.y = rects[i].bounds[( corner & 2 ) != 0 ? 1 : 0].y +
					  ( ( corner & 2 ) != 0 ? cornerClearance : -cornerClearance );
			nodes.push_back( { point, static_cast<int>( i ) } );
		}
	}

	const int nodeCount = static_cast<int>( nodes.size() );
	std::vector<float> distance( nodeCount, idMath::INFINITUM );
	std::vector<int> previous( nodeCount, -1 );
	std::vector<unsigned char> visited( nodeCount, 0 );
	distance[0] = 0.0f;
	for( int pass = 0; pass < nodeCount; ++pass )
	{
		int current = -1;
		for( int i = 0; i < nodeCount; ++i )
		{
			if( !visited[i] && ( current < 0 || distance[i] < distance[current] ) )
			{
				current = i;
			}
		}

		if( current < 0 || distance[current] == idMath::INFINITUM )
		{
			break;
		}

		if( current == 1 )
		{
			break;
		}

		visited[current] = 1;
		for( int next = 1; next < nodeCount; ++next )
		{
			if( next == current || visited[next] )
			{
				continue;
			}

			if( current == 0 && startObstacle >= 0 && nodes[next].obstacle != startObstacle )
			{
				continue;
			}

			if( !SegmentClear( nodes[current].point, nodes[next].point, rects,
							   current == 0 ? startObstacle : -1 ) )
			{
				continue;
			}

			if( routePath.type != AAS2_PATHTYPE_FLY )
			{
				const int fromArea = current == 0 ? startAreaNum :
									 file->PointAreaNumMaxHeight( nodes[current].point, actorSize.z + file->GetSettings().maxStepHeight );
				const int toArea = next == 1 ? routePath.moveAreaNum :
								   file->PointAreaNumMaxHeight( nodes[next].point, actorSize.z + file->GetSettings().maxStepHeight );
				if( fromArea <= 0 || toArea <= 0 )
				{
					continue;
				}
				idVec3 validEnd;
				int validArea = fromArea;
				if( !WalkPathValidFromArea( fromArea, nodes[current].point, toArea,
											nodes[next].point, TFL_WALK, validEnd, validArea ) )
				{
					continue;
				}
			}

			float cost = ( nodes[next].point - nodes[current].point ).LengthFast();
			if( current == 0 && lastDirection.ToVec2().LengthSqr() > 1.0f &&
					lastDirection.ToVec2() * ( nodes[next].point - startOrigin ).ToVec2() < 0.0f )
			{
				cost += 100.0f;
			}
			if( distance[current] + cost < distance[next] )
			{
				distance[next] = distance[current] + cost;
				previous[next] = current;
			}
		}
	}

	if( previous[1] < 0 )
	{
		avoidancePath.seekPos[0] = startOrigin;
		avoidancePath.seekPos[1] = startOrigin;
		avoidancePath.hasValidPath = false;
		return false;
	}

	std::vector<int> reversePath;
	for( int node = 1; node >= 0; node = previous[node] )
	{
		reversePath.push_back( node );
		if( node == 0 )
		{
			break;
		}
	}
	std::reverse( reversePath.begin(), reversePath.end() );
	if( reversePath.size() <= 2 )
	{
		return true;
	}

	const idVec3 corner = nodes[reversePath[1]].point;
	const idVec3 next = nodes[reversePath.size() > 2 ? reversePath[2] : reversePath[1]].point;
	idVec3 incoming = corner - startOrigin;
	idVec3 outgoing = next - corner;
	incoming.z = outgoing.z = 0.0f;
	const float incomingLength = incoming.Normalize();
	const float outgoingLength = outgoing.Normalize();
	float turnRadius = actorSize.x > actorSize.y ? actorSize.x : actorSize.y;
	turnRadius += 4.0f;
	if( turnRadius > incomingLength * 0.45f )
	{
		turnRadius = incomingLength * 0.45f;
	}
	if( turnRadius > outgoingLength * 0.45f )
	{
		turnRadius = outgoingLength * 0.45f;
	}
	if( turnRadius >= 4.0f && incoming * outgoing < 0.985f )
	{
		avoidancePath.seekPos[0] = corner - incoming * turnRadius;
		avoidancePath.seekPos[1] = corner + outgoing * turnRadius;
		SetSeekPlane( avoidancePath, avoidancePath.seekPos[0], avoidancePath.seekPos[1] );
	}
	else
	{
		avoidancePath.seekPos[0] = corner;
		avoidancePath.seekPos[1] = next;
		SetSeekPlane( avoidancePath, corner, next );
	}
	return true;
}
