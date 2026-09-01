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

#include "AAS2File.h"

const float AAS2_CELL_SIZE = 128.0f;

/*
============
idAAS2Settings::SpatialKey
============
*/
int idAAS2File::SpatialKey( int x, int y )
{
	return static_cast<int>( static_cast<unsigned int>( x ) * 73856093u ^
							 static_cast<unsigned int>( y ) * 19349663u );
}

/*
============
idAAS2Settings::BuildSpatialIndex
============
*/
void idAAS2File::BuildSpatialIndex()
{
	spatialRefs.Clear();
	spatialHash.Clear( 4096, areas.Num() * 4 > 4096 ? areas.Num() * 4 : 4096 );
	globalAreas.Clear();
	for( int areaNum = 1; areaNum < areas.Num(); ++areaNum )
	{
		const idBounds& bounds = areas[areaNum].bounds;
		const int minX = static_cast<int>( idMath::Floor( bounds[0].x / AAS2_CELL_SIZE ) );
		const int maxX = static_cast<int>( idMath::Floor( bounds[1].x / AAS2_CELL_SIZE ) );
		const int minY = static_cast<int>( idMath::Floor( bounds[0].y / AAS2_CELL_SIZE ) );
		const int maxY = static_cast<int>( idMath::Floor( bounds[1].y / AAS2_CELL_SIZE ) );
		const long long count = static_cast<long long>( maxX - minX + 1 ) * ( maxY - minY + 1 );
		if( count <= 0 || count > 4096 )
		{
			globalAreas.Append( areaNum );
			continue;
		}
		for( int y = minY; y <= maxY; ++y ) for( int x = minX; x <= maxX; ++x )
			{
				spatialRef_t ref = { x, y, areaNum };
				const int index = spatialRefs.Append( ref );
				spatialHash.Add( SpatialKey( x, y ), index );
			}
	}
}

/*
============
idAAS2Settings::PointInsideArea
============
*/
bool idAAS2File::PointInsideArea( int areaNum, const idVec3& point, float& floorHeight ) const
{
	if( areaNum <= 0 || areaNum >= areas.Num() )
	{
		return false;
	}
	const aas2Area_t& area = areas[areaNum];
	bool positive = false, negative = false;
	for( int i = 0; i < area.numEdges; ++i )
	{
		const int signedEdge = edgeIndex[area.firstEdge + i];
		const aas2Edge_t& edge = edges[idMath::Abs( signedEdge )];
		const idVec3& a = vertices[edge.vertexNum[signedEdge < 0 ? 1 : 0]];
		const idVec3& b = vertices[edge.vertexNum[signedEdge < 0 ? 0 : 1]];
		const float dx = b.x - a.x, dy = b.y - a.y;
		const float cross = dx * ( point.y - a.y ) - dy * ( point.x - a.x );
		const float tolerance = 0.5f * idMath::Sqrt( dx * dx + dy * dy );
		if( cross > tolerance )
		{
			positive = true;
		}
		if( cross < -tolerance )
		{
			negative = true;
		}
		if( positive && negative )
		{
			return false;
		}
	}
	const idVec3& normal = area.floorPlane.Normal();
	if( idMath::Fabs( normal.z ) < 0.001f )
	{
		return false;
	}
	floorHeight = -( normal.x * point.x + normal.y * point.y + area.floorPlane[3] ) / normal.z;
	return true;
}

/*
============
idAAS2Settings::PointAreaDistanceSqr
============
*/
float idAAS2File::PointAreaDistanceSqr( int areaNum, const idVec3& point ) const
{
	float floorHeight;
	if( PointInsideArea( areaNum, point, floorHeight ) )
	{
		const float dz = point.z - floorHeight;
		return dz * dz;
	}
	if( areaNum <= 0 || areaNum >= areas.Num() )
	{
		return idMath::INFINITUM;
	}
	const aas2Area_t& area = areas[areaNum];
	float bestDistance = idMath::INFINITUM;
	for( int i = 0; i < area.numEdges; ++i )
	{
		const aas2Edge_t& edge = edges[idMath::Abs( edgeIndex[area.firstEdge + i] )];
		const idVec3& a = vertices[edge.vertexNum[0]];
		const idVec3& b = vertices[edge.vertexNum[1]];
		const float dx = b.x - a.x;
		const float dy = b.y - a.y;
		const float lengthSqr = dx * dx + dy * dy;
		const float fraction = lengthSqr > 1.0e-8f ?
							   idMath::ClampFloat( 0.0f, 1.0f,
									   ( ( point.x - a.x ) * dx + ( point.y - a.y ) * dy ) / lengthSqr ) : 0.0f;
		const idVec3 nearest = a + ( b - a ) * fraction;
		const float x = point.x - nearest.x;
		const float y = point.y - nearest.y;
		const float z = point.z - nearest.z;
		const float distance = x * x + y * y + z * z;
		if( distance < bestDistance )
		{
			bestDistance = distance;
		}
	}
	return bestDistance;
}

/*
============
idAAS2Settings::AreaCenter
============
*/
idVec3 idAAS2File::AreaCenter( int areaNum ) const
{
	return areaNum > 0 && areaNum < areas.Num() ? areas[areaNum].center : vec3_origin;
}

/*
============
idAAS2Settings::AreaBounds
============
*/
idBounds idAAS2File::AreaBounds( int areaNum ) const
{
	if( areaNum > 0 && areaNum < areas.Num() )
	{
		return areas[areaNum].bounds;
	}
	idBounds empty;
	empty.Clear();
	return empty;
}

/*
============
idAAS2Settings::PointAreaNumMaxHeight
============
*/
int idAAS2File::PointAreaNumMaxHeight( const idVec3& origin, float maxHeight ) const
{
	const int cellX = static_cast<int>( idMath::Floor( origin.x / AAS2_CELL_SIZE ) );
	const int cellY = static_cast<int>( idMath::Floor( origin.y / AAS2_CELL_SIZE ) );
	int bestArea = 0;
	float bestScore = idMath::INFINITUM;
	for( int pass = 0; pass < 2; ++pass )
	{
		int index = pass == 0 ? spatialHash.GetFirst( SpatialKey( cellX, cellY ) ) : 0;
		const int limit = pass == 0 ? -1 : globalAreas.Num();
		while( pass == 0 ? index >= 0 : index < limit )
		{
			int areaNum;
			if( pass == 0 )
			{
				const spatialRef_t& ref = spatialRefs[index];
				if( ref.x != cellX || ref.y != cellY )
				{
					index = spatialHash.GetNext( index );
					continue;
				}
				areaNum = ref.areaNum;
			}
			else
			{
				areaNum = globalAreas[index];
			}
			float floorHeight;
			if( PointInsideArea( areaNum, origin, floorHeight ) )
			{
				const float height = origin.z - floorHeight;
				if( height >= -settings.maxStepHeight - 1.0f && height <= maxHeight )
				{
					const float score = idMath::Fabs( height ) + ( height < 0.0f ? maxHeight : 0.0f );
					if( score < bestScore )
					{
						bestScore = score;
						bestArea = areaNum;
					}
				}
			}
			if( pass == 0 )
			{
				index = spatialHash.GetNext( index );
			}
			else
			{
				++index;
			}
		}
	}
	return bestArea;
}

/*
============
idAAS2Settings::PointAreaNum
============
*/
int idAAS2File::PointAreaNum( const idVec3& origin ) const
{
	const float height = settings.boundingBoxes[0][1].z - settings.boundingBoxes[0][0].z + settings.maxStepHeight;
	return PointAreaNumMaxHeight( origin, height );
}

/*
============
idAAS2Settings::PointAreaCandidates
============
*/
int idAAS2File::PointAreaCandidates( const idVec3& origin, float maxHeight, int* areaNums, int maxAreaNums ) const
{
	if( areaNums == NULL || maxAreaNums <= 0 )
	{
		return 0;
	}
	const int cellX = static_cast<int>( idMath::Floor( origin.x / AAS2_CELL_SIZE ) );
	const int cellY = static_cast<int>( idMath::Floor( origin.y / AAS2_CELL_SIZE ) );
	float scores[32];
	const int capacity = idMath::ClampInt( 1, 32, maxAreaNums );
	int count = 0;
	for( int pass = 0; pass < 2; ++pass )
	{
		int index = pass == 0 ? spatialHash.GetFirst( SpatialKey( cellX, cellY ) ) : 0;
		const int limit = pass == 0 ? -1 : globalAreas.Num();
		while( pass == 0 ? index >= 0 : index < limit )
		{
			int areaNum;
			if( pass == 0 )
			{
				const spatialRef_t& ref = spatialRefs[index];
				if( ref.x != cellX || ref.y != cellY )
				{
					index = spatialHash.GetNext( index );
					continue;
				}
				areaNum = ref.areaNum;
			}
			else
			{
				areaNum = globalAreas[index];
			}
			float floorHeight;
			if( PointInsideArea( areaNum, origin, floorHeight ) )
			{
				const float height = origin.z - floorHeight;
				if( height >= -settings.maxStepHeight - 1.0f && height <= maxHeight )
				{
					const float score = idMath::Fabs( height ) + ( height < 0.0f ? maxHeight : 0.0f );
					int duplicate = -1;
					for( int i = 0; i < count; ++i ) if( areaNums[i] == areaNum )
						{
							duplicate = i;
							break;
						}
					if( duplicate < 0 )
					{
						int insert = count;
						while( insert > 0 && scores[insert - 1] > score )
						{
							--insert;
						}
						if( insert < capacity )
						{
							const int newCount = count < capacity ? count + 1 : count;
							for( int i = newCount - 1; i > insert; --i )
							{
								areaNums[i] = areaNums[i - 1];
								scores[i] = scores[i - 1];
							}
							areaNums[insert] = areaNum;
							scores[insert] = score;
							count = newCount;
						}
					}
				}
			}
			if( pass == 0 )
			{
				index = spatialHash.GetNext( index );
			}
			else
			{
				++index;
			}
		}
	}
	// Multiple coplanar compiler polygons may contain a seam point.  Return all
	// of those, but never mix in a different vertical floor merely because it
	// shares the same XY cell.
	int coplanarCount = 0;
	while( coplanarCount < count && scores[coplanarCount] <= scores[0] + 1.0f )
	{
		++coplanarCount;
	}
	return coplanarCount;
}

/*
============
idAAS2Settings::BestReachableAreaInBounds
============
*/
int idAAS2File::BestReachableAreaInBounds( const idBounds& bounds, const idVec3& queryPoint, int areaFlags, int excludeTravelFlags ) const
{
	const int minX = static_cast<int>( idMath::Floor( bounds[0].x / AAS2_CELL_SIZE ) );
	const int maxX = static_cast<int>( idMath::Floor( bounds[1].x / AAS2_CELL_SIZE ) );
	const int minY = static_cast<int>( idMath::Floor( bounds[0].y / AAS2_CELL_SIZE ) );
	const int maxY = static_cast<int>( idMath::Floor( bounds[1].y / AAS2_CELL_SIZE ) );
	int bestArea = 0;
	float bestDistance = idMath::INFINITUM;
	for( int y = minY; y <= maxY; ++y ) for( int x = minX; x <= maxX; ++x )
		{
			for( int index = spatialHash.GetFirst( SpatialKey( x, y ) ); index >= 0; index = spatialHash.GetNext( index ) )
			{
				const spatialRef_t& ref = spatialRefs[index];
				if( ref.x != x || ref.y != y )
				{
					continue;
				}
				const aas2Area_t& area = areas[ref.areaNum];
				if( ( areaFlags != 0 && ( area.flags & areaFlags ) == 0 ) || area.IsDisabled() || ( area.travelFlags & excludeTravelFlags ) || !area.bounds.IntersectsBounds( bounds ) )
				{
					continue;
				}
				const float distance = PointAreaDistanceSqr( ref.areaNum, queryPoint );
				if( distance < bestDistance )
				{
					bestDistance = distance;
					bestArea = ref.areaNum;
				}
			}
		}
	for( int i = 0; i < globalAreas.Num(); ++i )
	{
		const int areaNum = globalAreas[i];
		const aas2Area_t& area = areas[areaNum];
		if( ( areaFlags != 0 && ( area.flags & areaFlags ) == 0 ) || area.IsDisabled() || ( area.travelFlags & excludeTravelFlags ) || !area.bounds.IntersectsBounds( bounds ) )
		{
			continue;
		}
		const float distance = PointAreaDistanceSqr( areaNum, queryPoint );
		if( distance < bestDistance )
		{
			bestDistance = distance;
			bestArea = areaNum;
		}
	}
	return bestArea;
}

/*
============
idAAS2Settings::BoundsReachableAreaNum
============
*/
int idAAS2File::BoundsReachableAreaNum( const idBounds& bounds, int areaFlags, int excludeTravelFlags ) const
{
	return BestReachableAreaInBounds( bounds, bounds.GetCenter(), areaFlags, excludeTravelFlags );
}

/*
============
idAAS2Settings::PointReachableAreaNum
============
*/
int idAAS2File::PointReachableAreaNum( const idVec3& origin, const idBounds& searchBounds, int areaFlags, int excludeTravelFlags ) const
{
	const int areaNum = PointAreaNum( origin );
	if( areaNum && ( areaFlags == 0 || ( areas[areaNum].flags & areaFlags ) != 0 ) && !areas[areaNum].IsDisabled() &&
			!( areas[areaNum].travelFlags & excludeTravelFlags ) )
	{
		return areaNum;
	}

	// Match legacy AAS semantics: grow the fallback search gradually and keep
	// the first local floor found. Searching the full actor bounds immediately
	// can select a large polygon behind the actor and make routing reverse at
	// every navigation seam.
	for( int step = 1; step <= 12; ++step )
	{
		const float fraction = step * ( 1.0f / 12.0f );
		idBounds bounds;
		bounds[0] = origin + searchBounds[0] * fraction;
		bounds[1] = origin + searchBounds[1] * fraction;
		const int fallback = BestReachableAreaInBounds( bounds, origin,
							 areaFlags, excludeTravelFlags );
		if( fallback != 0 )
		{
			return fallback;
		}
	}
	return 0;
}

/*
============
idAAS2Settings::PushPointIntoAreaNum
============
*/
bool idAAS2File::PushPointIntoAreaNum( int areaNum, idVec3& point ) const
{
	if( areaNum <= 0 || areaNum >= areas.Num() )
	{
		return false;
	}
	float floorHeight;
	if( PointInsideArea( areaNum, point, floorHeight ) )
	{
		if( point.z < floorHeight )
		{
			point.z = floorHeight;
		}
		return true;
	}
	const aas2Area_t& area = areas[areaNum];
	idVec3 nearest = point;
	float bestDistance = idMath::INFINITUM;
	for( int i = 0; i < area.numEdges; ++i )
	{
		const aas2Edge_t& edge = edges[idMath::Abs( edgeIndex[area.firstEdge + i] )];
		const idVec3& a = vertices[edge.vertexNum[0]], &b = vertices[edge.vertexNum[1]];
		const float dx = b.x - a.x, dy = b.y - a.y, lengthSqr = dx * dx + dy * dy;
		const float t = lengthSqr > 0.0f ? idMath::ClampFloat( 0.0f, 1.0f,
						( ( point.x - a.x ) * dx + ( point.y - a.y ) * dy ) / lengthSqr ) : 0.0f;
		const idVec3 candidate = a + ( b - a ) * t;
		const float distance = ( candidate.x - point.x ) * ( candidate.x - point.x ) + ( candidate.y - point.y ) * ( candidate.y - point.y );
		if( distance < bestDistance )
		{
			bestDistance = distance;
			nearest = candidate;
		}
	}
	point.x = nearest.x;
	point.y = nearest.y;
	if( PointInsideArea( areaNum, point, floorHeight ) && point.z < floorHeight )
	{
		point.z = floorHeight;
	}
	return true;
}

/*
============
idAAS2Settings::Trace
============
*/
bool idAAS2File::Trace( aas2Trace_t& trace, const idVec3& start, const idVec3& end ) const
{
	trace.numAreas = 0;
	trace.lastAreaNum = 0;
	trace.blockingAreaNum = 0;
	trace.planeNum = 0;
	const idVec3 delta = end - start;
	const int steps = idMath::ClampInt( 1, 4096, static_cast<int>( delta.Length() / 4.0f ) + 1 );
	const float agentHeight = settings.boundingBoxes[0][1].z - settings.boundingBoxes[0][0].z + settings.maxStepHeight;
	bool entered = false;
	for( int i = 0; i <= steps; ++i )
	{
		const float fraction = static_cast<float>( i ) / steps;
		const idVec3 point = start + delta * fraction;
		const int areaNum = PointAreaNumMaxHeight( point, agentHeight );
		if( areaNum == 0 )
		{
			if( !entered && trace.getOutOfSolid )
			{
				continue;
			}
			trace.fraction = fraction;
			trace.endpos = point;
			trace.blockingAreaNum = 0;
			return true;
		}
		entered = true;
		if( areas[areaNum].IsDisabled() || ( areas[areaNum].flags & trace.flags ) || ( areas[areaNum].travelFlags & trace.travelFlags ) )
		{
			trace.fraction = fraction;
			trace.endpos = point;
			trace.blockingAreaNum = areaNum;
			return true;
		}
		if( areaNum != trace.lastAreaNum )
		{
			trace.lastAreaNum = areaNum;
			if( trace.numAreas < trace.maxAreas )
			{
				if( trace.areas != NULL )
				{
					trace.areas[trace.numAreas] = areaNum;
				}
				if( trace.points != NULL )
				{
					trace.points[trace.numAreas] = point;
				}
				++trace.numAreas;
			}
		}
	}
	trace.fraction = entered ? 1.0f : 0.0f;
	trace.endpos = entered ? end : start;
	return false;
}
