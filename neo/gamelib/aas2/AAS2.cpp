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

idCVar aas2_debugPathing( "aas2_debugPathing", "0", CVAR_GAME | CVAR_INTEGER, "AAS2 diagnostics:\n1 = failures and door state\n2 = sampled path decisions\n3 = verbose validation and obstacle decisions\n4 = point-area test output" );

/*
============
AAS2AreaLogSample
============
*/
static bool AAS2AreaLogSample()
{
	if( aas2_debugPathing.GetInteger() < 3 )
	{
		return false;
	}
	static int interval = -1;
	static int count = 0;
	const int currentInterval = Sys_Milliseconds() / 1000;
	if( interval != currentInterval )
	{
		interval = currentInterval;
		count = 0;
	}
	return count++ < 16;
}

/*
============
NativeAreaFlags
============
*/
static int NativeAreaFlags( int flags )
{
	int nativeFlags = 0;
	if( flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY | AREA_FLOOR ) )
	{
		nativeFlags |= AAS2_AREA_REACHABLE;
	}
	if( flags & AREA_LEDGE )
	{
		nativeFlags |= AAS2_AREA_LEDGE;
	}
	return nativeFlags;
}

/*
============
LegacyAreaFlags
============
*/
static int LegacyAreaFlags( const aas2Area_t& area, const idAAS2Settings& settings )
{
	int flags = AREA_FLOOR;
	if( area.flags & AAS2_AREA_LEDGE )
	{
		flags |= AREA_LEDGE;
	}
	if( area.flags & AAS2_AREA_REACHABLE )
	{
		flags |= AREA_REACHABLE_WALK;
		if( settings.allowFlyReachabilities )
		{
			flags |= AREA_REACHABLE_FLY;
		}
	}
	return flags;
}

/*
============
LegacyAreaFlags
============
*/
static int LegacyTravelFlags( const aas2Area_t& area )
{
	int flags = TFL_AIR;
	if( area.IsDisabled() || ( area.travelFlags & AAS2_TRAVEL_INVALID ) )
	{
		flags |= TFL_INVALID;
	}
	if( area.travelFlags & AAS2_TRAVEL_WALK )
	{
		flags |= TFL_WALK;
	}
	if( area.travelFlags & AAS2_TRAVEL_WALK_OFF_LEDGE )
	{
		flags |= TFL_WALKOFFLEDGE;
	}
	if( area.travelFlags & AAS2_TRAVEL_LEDGE_GRAB )
	{
		flags |= TFL_BARRIERJUMP;
	}
	if( area.travelFlags & AAS2_TRAVEL_WATER_JUMP )
	{
		flags |= TFL_WATERJUMP;
	}
	if( area.travelFlags & AAS2_TRAVEL_LADDER )
	{
		flags |= TFL_LADDER;
	}
	if( area.travelFlags & AAS2_TRAVEL_FLY )
	{
		flags |= TFL_FLY;
	}
	return flags;
}

/*
============
idAAS2Runtime::Alloc
============
*/
idAAS2Runtime* idAAS2Runtime::Alloc()
{
	return new( TAG_AAS ) idAAS2RuntimeLocal;
}

/*
============
idAAS2RuntimeLocal::idAAS2RuntimeLocal
============
*/
idAAS2RuntimeLocal::idAAS2RuntimeLocal() : file( NULL )
{
}

/*
============
idAAS2RuntimeLocal::~idAAS2RuntimeLocal
============
*/
idAAS2RuntimeLocal::~idAAS2RuntimeLocal()
{
	Shutdown();
}

/*
============
idAAS2RuntimeLocal::Shutdown
============
*/
void idAAS2RuntimeLocal::Shutdown()
{
	RemoveAllObstacles();
	delete file;
	file = NULL;
}

/*
============
idAAS2RuntimeLocal::Init
============
*/
bool idAAS2RuntimeLocal::Init( const idStr& mapName, unsigned int mapFileCRC )
{
	if( file != NULL && mapName.Icmp( file->GetName() ) == 0 && mapFileCRC == file->GetCRC() )
	{
		RemoveAllObstacles();
		return true;
	}
	Shutdown();
	file = new( TAG_AAS ) idAAS2File;
	if( !file->Load( mapName, mapFileCRC ) )
	{
		delete file;
		file = NULL;
		return false;
	}
	common->Printf( "[AAS2DBG] instrumented runtime active; aas2_debugPathing=%d\n",
					aas2_debugPathing.GetInteger() );
	return true;
}

/*
============
idAAS2RuntimeLocal::Stats
============
*/
void idAAS2RuntimeLocal::Stats() const
{
	if( file != NULL )
	{
		file->PrintInfo();
	}
}

/*
============
idAAS2RuntimeLocal::Test
============
*/
void idAAS2RuntimeLocal::Test( const idVec3& origin )
{
	if( file == NULL || aas2_debugPathing.GetInteger() < 4 || !AAS2AreaLogSample() )
	{
		return;
	}
	common->Printf( "native AAS2 point area: %d at (%.1f %.1f %.1f)\n",
					file->PointAreaNum( origin ), origin.x, origin.y, origin.z );
}

/*
============
idAAS2RuntimeLocal::GetSettings
============
*/
const idAAS2Settings* idAAS2RuntimeLocal::GetSettings() const
{
	return file != NULL ? &file->GetSettings() : NULL;
}

/*
============
idAAS2RuntimeLocal::PointAreaNum
============
*/
int idAAS2RuntimeLocal::PointAreaNum( const idVec3& origin ) const
{
	return file != NULL ? file->PointAreaNum( origin ) : 0;
}

/*
============
idAAS2RuntimeLocal::PointReachableAreaNum
============
*/
int idAAS2RuntimeLocal::PointReachableAreaNum( const idVec3& origin, const idBounds& bounds, int areaFlags ) const
{
	if( file == NULL )
	{
		return 0;
	}
	const int nativeFlags = NativeAreaFlags( areaFlags );
	const int exactArea = file->PointAreaNum( origin );
	if( ( areaFlags & AREA_REACHABLE_FLY ) && file->GetSettings().allowFlyReachabilities )
	{
		const int flyArea = file->PointAreaNumMaxHeight( origin, 65536.0f );
		if( flyArea > 0 )
		{
			const aas2Area_t& area = file->GetArea( flyArea );
			if( !area.IsDisabled() && ( nativeFlags == 0 || ( area.flags & nativeFlags ) != 0 ) &&
					!( area.travelFlags & AAS2_TRAVEL_INVALID ) )
			{
				return flyArea;
			}
		}
	}
	const int selectedArea = file->PointReachableAreaNum(
								 origin, bounds, nativeFlags, AAS2_TRAVEL_INVALID );
	if( exactArea != selectedArea && AAS2AreaLogSample() )
	{
		common->Printf( "[AAS2DBG] area fallback exact=%d selected=%d "
						"at=(%.1f %.1f %.1f)\n", exactArea, selectedArea,
						origin.x, origin.y, origin.z );
	}
	return selectedArea;
}

/*
============
idAAS2RuntimeLocal::BoundsReachableAreaNum
============
*/
int idAAS2RuntimeLocal::BoundsReachableAreaNum( const idBounds& bounds, int areaFlags ) const
{
	if( file == NULL )
	{
		return 0;
	}
	if( ( areaFlags & AREA_REACHABLE_FLY ) && file->GetSettings().allowFlyReachabilities )
	{
		const int flyArea = file->PointAreaNumMaxHeight( bounds.GetCenter(), 65536.0f );
		if( flyArea > 0 && !file->GetArea( flyArea ).IsDisabled() )
		{
			return flyArea;
		}
	}
	return file->BoundsReachableAreaNum( bounds, NativeAreaFlags( areaFlags ), AAS2_TRAVEL_INVALID );
}

/*
============
idAAS2RuntimeLocal::PushPointIntoAreaNum
============
*/
void idAAS2RuntimeLocal::PushPointIntoAreaNum( int areaNum, idVec3& origin ) const
{
	if( file != NULL )
	{
		file->PushPointIntoAreaNum( areaNum, origin );
	}
}

/*
============
idAAS2RuntimeLocal::AreaCenter
============
*/
idVec3 idAAS2RuntimeLocal::AreaCenter( int areaNum ) const
{
	return file != NULL ? file->AreaCenter( areaNum ) : vec3_origin;
}

/*
============
idAAS2RuntimeLocal::AreaFlags
============
*/
int idAAS2RuntimeLocal::AreaFlags( int areaNum ) const
{
	return file != NULL && areaNum > 0 && areaNum < file->GetNumAreas() ? LegacyAreaFlags( file->GetArea( areaNum ), file->GetSettings() ) : 0;
}

/*
============
idAAS2RuntimeLocal::AreaTravelFlags
============
*/
int idAAS2RuntimeLocal::AreaTravelFlags( int areaNum ) const
{
	return file != NULL && areaNum > 0 && areaNum < file->GetNumAreas() ? LegacyTravelFlags( file->GetArea( areaNum ) ) : 0;
}

/*
============
idAAS2RuntimeLocal::Trace
============
*/
bool idAAS2RuntimeLocal::Trace( aas2Trace_t& trace, const idVec3& start, const idVec3& end ) const
{
	if( file != NULL )
	{
		const int legacyFlags = trace.flags;
		trace.flags = NativeAreaFlags( legacyFlags );
		const bool blocked = file->Trace( trace, start, end );
		trace.flags = legacyFlags;
		return blocked;
	}
	trace.fraction = 0.0f;
	trace.endpos = start;
	trace.lastAreaNum = 0;
	return true;
}

/*
============
idAAS2RuntimeLocal::GetPlane
============
*/
const idPlane& idAAS2RuntimeLocal::GetPlane( int planeNum ) const
{
	static idPlane dummy;
	return file != NULL ? file->GetPlane( planeNum ) : dummy;
}

/*
============
idAAS2RuntimeLocal::GetEdgeVertexNumbers
============
*/
void idAAS2RuntimeLocal::GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const
{
	if( file == NULL )
	{
		verts[0] = verts[1] = 0;
		return;
	}
	const aas2Edge_t& edge = file->GetEdge( idMath::Abs( edgeNum ) );
	verts[0] = edge.vertexNum[edgeNum < 0 ? 1 : 0];
	verts[1] = edge.vertexNum[edgeNum < 0 ? 0 : 1];
}

/*
============
idAAS2RuntimeLocal::GetEdge
============
*/
void idAAS2RuntimeLocal::GetEdge( int edgeNum, idVec3& start, idVec3& end ) const
{
	int verts[2];
	GetEdgeVertexNumbers( edgeNum, verts );
	if( file == NULL )
	{
		start.Zero();
		end.Zero();
		return;
	}
	start = file->GetVertex( verts[0] );
	end = file->GetVertex( verts[1] );
}

/*
============
idAAS2RuntimeLocal::GetWallEdges
============
*/
int idAAS2RuntimeLocal::GetWallEdges( int areaNum, const idBounds& bounds, int travelFlags, int* output, int maxEdges ) const
{
	if( file == NULL || areaNum <= 0 || areaNum >= file->GetNumAreas() )
	{
		return 0;
	}
	const int areaCount = file->GetNumAreas();
	std::vector<unsigned char> visited( areaCount, 0 );
	std::vector<int> pending;
	std::vector<unsigned char> emitted( file->GetNumEdges(), 0 );
	pending.push_back( areaNum );
	visited[areaNum] = 1;
	int count = 0;
	while( !pending.empty() && count < maxEdges )
	{
		const int currentAreaNum = pending.back();
		pending.pop_back();
		const aas2Area_t& area = file->GetArea( currentAreaNum );
		if( area.IsDisabled() || !area.bounds.IntersectsBounds( bounds ) )
		{
			continue;
		}

		// The legacy query walks the local reachability neighborhood.  Steering
		// needs walls from the next area as well as the one containing the actor.
		for( aas2Reachability_t* reach = area.reach; reach != NULL; reach = reach->next )
		{
			const int destination = reach->toAreaNum;
			if( destination > 0 && destination < areaCount && !visited[destination] &&
					( reach->travelType & travelFlags ) != 0 &&
					!file->GetArea( destination ).IsDisabled() &&
					file->GetArea( destination ).bounds.IntersectsBounds( bounds ) )
			{
				visited[destination] = 1;
				pending.push_back( destination );
			}
		}

		for( int i = 0; i < area.numEdges && count < maxEdges; ++i )
		{
			int edgeNum = file->GetEdgeIndex( area.firstEdge + i );
			const int absoluteEdge = idMath::Abs( edgeNum );
			if( absoluteEdge <= 0 || absoluteEdge >= file->GetNumEdges() || emitted[absoluteEdge] )
			{
				continue;
			}
			idVec3 edgeStart, edgeEnd;
			GetEdge( edgeNum, edgeStart, edgeEnd );
			const idVec3 edgeDelta = edgeEnd - edgeStart;
			const float edgeLengthSqr = edgeDelta.LengthSqr();
			bool crossed = false;
			for( aas2Reachability_t* reach = area.reach; reach != NULL; reach = reach->next )
			{
				if( ( reach->travelType & travelFlags ) == 0 )
				{
					continue;
				}
				if( reach->edgeNum != 0 && reach->edgeNum == absoluteEdge )
				{
					crossed = true;
					break;
				}
				// CSG seams and dynamic portal T-junctions do not necessarily
				// share an edge number.  Their transition point still opens this
				// boundary for local obstacle avoidance.
				const float fraction = edgeLengthSqr > 1.0e-6f ?
									   idMath::ClampFloat( 0.0f, 1.0f, ( reach->start - edgeStart ) * edgeDelta / edgeLengthSqr ) : 0.0f;
				if( ( reach->start - ( edgeStart + edgeDelta * fraction ) ).LengthSqr() <= 1.0f )
				{
					crossed = true;
					break;
				}
			}
			if( crossed )
			{
				continue;
			}

			// The native file format only promises a closed area boundary; it does
			// not promise the clockwise orientation expected by Doom 3's obstacle
			// strip builder. Make the current area's walkable side the right side of
			// every directed wall edge, so the generated strip expands outwards.
			const float centerSide = edgeDelta.x * ( area.center.y - edgeStart.y ) -
									 edgeDelta.y * ( area.center.x - edgeStart.x );
			if( centerSide > 0.0f )
			{
				edgeNum = -edgeNum;
			}
			emitted[absoluteEdge] = 1;
			if( output != NULL )
			{
				output[count] = edgeNum;
			}
			++count;
		}
	}
	return count;
}

/*
============
idAAS2RuntimeLocal::SortWallEdges
============
*/
void idAAS2RuntimeLocal::SortWallEdges( int* edges, int numEdges ) const
{
	if( file == NULL || edges == NULL || numEdges <= 1 )
	{
		return;
	}

	// Dynamic obstacle avoidance builds an outward-facing strip from every wall
	// edge. Preserve the area winding here: reversing an edge also reverses that
	// strip and makes the walkable side look blocked.
	struct wallEdge_t
	{
		int edgeNum;
		int verts[2];
		int next;
	};
	std::vector<wallEdge_t> wallEdges( numEdges );
	std::vector<int> sequenceFirst( numEdges );
	std::vector<int> sequenceLast( numEdges );
	for( int i = 0; i < numEdges; ++i )
	{
		wallEdges[i].edgeNum = edges[i];
		GetEdgeVertexNumbers( edges[i], wallEdges[i].verts );
		wallEdges[i].next = -1;
		sequenceFirst[i] = sequenceLast[i] = i;
	}

	int numSequences = numEdges;
	bool merged;
	do
	{
		merged = false;
		for( int i = 0; i < numSequences && !merged; ++i )
		{
			for( int j = i + 1; j < numSequences; ++j )
			{
				if( wallEdges[sequenceFirst[i]].verts[0] ==
						wallEdges[sequenceLast[j]].verts[1] )
				{
					wallEdges[sequenceLast[j]].next = sequenceFirst[i];
					sequenceFirst[i] = sequenceFirst[j];
				}
				else if( wallEdges[sequenceLast[i]].verts[1] ==
						 wallEdges[sequenceFirst[j]].verts[0] )
				{
					wallEdges[sequenceLast[i]].next = sequenceFirst[j];
					sequenceLast[i] = sequenceLast[j];
				}
				else
				{
					continue;
				}
				for( int k = j; k + 1 < numSequences; ++k )
				{
					sequenceFirst[k] = sequenceFirst[k + 1];
					sequenceLast[k] = sequenceLast[k + 1];
				}
				--numSequences;
				merged = true;
				break;
			}
		}
	}
	while( merged );

	int outputIndex = 0;
	for( int i = 0; i < numSequences; ++i )
	{
		for( int edgeIndex = sequenceFirst[i]; edgeIndex >= 0;
				edgeIndex = wallEdges[edgeIndex].next )
		{
			edges[outputIndex++] = wallEdges[edgeIndex].edgeNum;
		}
	}
}

/*
============
idAAS2RuntimeLocal::SetAreaState
============
*/
bool idAAS2RuntimeLocal::SetAreaState( const idBounds& bounds, int areaContents, bool disabled )
{
	if( file == NULL )
	{
		return false;
	}
	bool changed = false;
	int matched = 0;
	int changedCount = 0;
	idBounds expanded;
	expanded[0] = bounds[0] - file->GetSettings().boundingBoxes[0][1];
	expanded[1] = bounds[1] - file->GetSettings().boundingBoxes[0][0];
	for( int areaNum = 1; areaNum < file->GetNumAreas(); ++areaNum )
	{
		aas2Area_t& area = file->GetArea( areaNum );
		if( !area.bounds.IntersectsBounds( expanded ) || ( areaContents != 0 && !( area.contents & areaContents ) ) )
		{
			continue;
		}
		++matched;
		if( area.stateDisabled != disabled )
		{
			area.stateDisabled = disabled;
			changed = true;
			++changedCount;
		}
	}
	if( aas2_debugPathing.GetInteger() >= 1 )
	{
		common->Printf( "[AAS2DBG] area-state contents=%x disabled=%d matched=%d changed=%d "
						"bounds=(%.1f %.1f %.1f)-(%.1f %.1f %.1f)\n",
						areaContents, disabled ? 1 : 0, matched, changedCount,
						bounds[0].x, bounds[0].y, bounds[0].z,
						bounds[1].x, bounds[1].y, bounds[1].z );
	}
	return changed;
}

/*
============
idAAS2RuntimeLocal::AddObstacle
============
*/
aas2Handle_t idAAS2RuntimeLocal::AddObstacle( const idBounds& bounds )
{
	if( file == NULL )
	{
		return 0;
	}
	obstacle_t* obstacle = new( TAG_AAS ) obstacle_t;
	obstacle->bounds[0] = bounds[0] - file->GetSettings().boundingBoxes[0][1];
	obstacle->bounds[1] = bounds[1] - file->GetSettings().boundingBoxes[0][0];
	for( int areaNum = 1; areaNum < file->GetNumAreas(); ++areaNum ) if( file->GetArea( areaNum ).bounds.IntersectsBounds( obstacle->bounds ) )
		{
			obstacle->areas.Append( areaNum );
			aas2Area_t& area = file->GetArea( areaNum );
			if( area.disableCount != 0xffff )
			{
				++area.disableCount;
			}
		}
	for( int i = 0; i < obstacleList.Num(); ++i ) if( obstacleList[i] == NULL )
		{
			obstacleList[i] = obstacle;
			return i + 1;
		}
	return obstacleList.Append( obstacle ) + 1;
}

/*
============
idAAS2RuntimeLocal::RemoveObstacle
============
*/
void idAAS2RuntimeLocal::RemoveObstacle( aas2Handle_t handle )
{
	const int index = handle - 1;
	if( index < 0 || index >= obstacleList.Num() || obstacleList[index] == NULL || file == NULL )
	{
		return;
	}
	obstacle_t* obstacle = obstacleList[index];
	for( int i = 0; i < obstacle->areas.Num(); ++i )
	{
		aas2Area_t& area = file->GetArea( obstacle->areas[i] );
		if( area.disableCount )
		{
			--area.disableCount;
		}
	}
	delete obstacle;
	obstacleList[index] = NULL;
}


/*
============
idAAS2RuntimeLocal::RemoveAllObstacles
============
*/
void idAAS2RuntimeLocal::RemoveAllObstacles()
{
	for( int i = 0; i < obstacleList.Num(); ++i ) if( obstacleList[i] != NULL )
		{
			RemoveObstacle( i + 1 );
		}
	obstacleList.Clear();
}

/*
============
idAAS2RuntimeLocal::TravelAllowed
============
*/
bool idAAS2RuntimeLocal::TravelAllowed( const aas2Reachability_t& reach, int travelFlags ) const
{
	if( reach.disableCount )
	{
		return false;
	}
	// Doom travel flags and AAS2 travel flags use different bit positions. The
	// normalized travelType is the stable game-facing category.
	return ( reach.travelType & travelFlags ) != 0;
}
