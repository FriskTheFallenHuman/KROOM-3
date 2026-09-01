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

#include "AAS2_Floor_Merge.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

/*
============
Dot
============
*/
static float Dot( const Vec3& left, const Vec3& right )
{
	return left.x * right.x + left.y * right.y + left.z * right.z;
}

/*
============
Add
============
*/
static Vec3 Add( const Vec3& left, const Vec3& right )
{
	return {left.x + right.x, left.y + right.y, left.z + right.z};
}

/*
============
Scale
============
*/
static Vec3 Scale( const Vec3& value, float scale )
{
	return {value.x * scale, value.y * scale, value.z * scale};
}

/*
============
Normalize
============
*/
static Vec3 Normalize( const Vec3& value )
{
	const float length = std::sqrt( Dot( value, value ) );
	return length > 1.0e-12f ? Scale( value, 1.0f / length ) : Vec3{};
}

/*
============
DisjointSet
============
*/
struct DisjointSet
{
	explicit DisjointSet( size_t count ) : parents( count ), groups( count )
	{
		for( size_t i = 0; i < count; ++i )
		{
			parents[i] = static_cast<uint32>( i );
			groups[i].push_back( static_cast<uint32>( i ) );
		}
	}

	uint32 Find( uint32 value )
	{
		if( parents[value] != value )
		{
			parents[value] = Find( parents[value] );
		}
		return parents[value];
	}

	void Join( uint32 left, uint32 right )
	{
		left = Find( left );
		right = Find( right );

		if( left != right )
		{
			if( groups[left].size() < groups[right].size() )
			{
				std::swap( left, right );
			}

			parents[right] = left;
			groups[left].insert( groups[left].end(), groups[right].begin(), groups[right].end() );
			groups[right].clear();
		}
	}

	std::vector<uint32> Combined( uint32 left, uint32 right )
	{
		left = Find( left );
		right = Find( right );
		std::vector<uint32> members;
		members.reserve( groups[left].size() + groups[right].size() );
		members.insert( members.end(), groups[left].begin(), groups[left].end() );
		members.insert( members.end(), groups[right].begin(), groups[right].end() );
		return members;
	}

	std::vector<uint32> parents;
	std::vector<std::vector<uint32>> groups;
};

/*
============
Coplanar
============
*/
static bool Coplanar( const File& file, const std::vector<uint32>& members, float minimumNormalDot, float distanceEpsilon )
{
	const Area& reference = file.areas[members.front()];
	const Vec3 normal = Normalize( reference.floorNormal );
	const float distance = Dot( normal, reference.center );

	for( uint32 member : members )
	{
		const Area& area = file.areas[member];
		if( Dot( normal, Normalize( area.floorNormal ) ) < minimumNormalDot )
		{
			return false;
		}

		for( uint32 edgeIndex : area.edges )
		{
			const Edge& edge = file.edges[edgeIndex];
			for( int endpoint = 0; endpoint < 2; ++endpoint )
			{
				if( idMath::Fabs( Dot( normal, file.vertices[edge.vertices[endpoint]] ) -
								  distance ) > distanceEpsilon )
				{
					return false;
				}
			}
		}
	}
	return true;
}

/*
============
Boundary
============
*/
struct Boundary
{
	std::vector<uint32> edges;
	std::vector<uint32> vertices;
};

/*
============
BuildBoundary
============
*/
static bool BuildBoundary( const File& file, const std::vector<uint32>& members, Boundary& boundary )
{
	std::unordered_map<uint32, size_t> edgeCounts;
	for( uint32 member : members )
	{
		for( uint32 edge : file.areas[member].edges )
		{
			++edgeCounts[edge];
		}
	}

	std::unordered_map<uint32, std::vector<uint32>> adjacency;
	for( const auto& entry : edgeCounts )
	{
		if( entry.second > 2 )
		{
			return false;
		}
		if( entry.second == 1 )
		{
			const Edge& edge = file.edges[entry.first];
			boundary.edges.push_back( entry.first );
			adjacency[edge.vertices[0]].push_back( entry.first );
			adjacency[edge.vertices[1]].push_back( entry.first );
		}
	}

	if( boundary.edges.size() < 3 )
	{
		return false;
	}

	for( const auto& entry : adjacency )
	{
		if( entry.second.size() != 2 )
		{
			return false;
		}
	}

	const uint32 firstEdge = boundary.edges.front();
	uint32 currentVertex = file.edges[firstEdge].vertices[0];
	uint32 currentEdge = firstEdge;
	const uint32 startVertex = currentVertex;
	boundary.vertices.clear();
	boundary.edges.clear();

	do
	{
		boundary.vertices.push_back( currentVertex );
		boundary.edges.push_back( currentEdge );
		const Edge& edge = file.edges[currentEdge];
		currentVertex = edge.vertices[0] == currentVertex ?
						edge.vertices[1] : edge.vertices[0];
		const std::vector<uint32>& incident = adjacency[currentVertex];
		currentEdge = incident[0] == currentEdge ? incident[1] : incident[0];
		if( boundary.edges.size() > adjacency.size() )
		{
			return false;
		}
	}
	while( currentVertex != startVertex );

	return boundary.edges.size() == adjacency.size() && currentEdge == firstEdge;
}

/*
============
Project
============
*/
static void Project( const Vec3& point, int droppedAxis, float& first, float& second )
{
	if( droppedAxis == 0 )
	{
		first = point.y;
		second = point.z;
	}
	else if( droppedAxis == 1 )
	{
		first = point.x;
		second = point.z;
	}
	else
	{
		first = point.x;
		second = point.y;
	}
}

/*
============
ConvexBoundary
============
*/
static bool ConvexBoundary( const File& file, const Boundary& boundary, const Vec3& normal )
{
	const float components[3] =
	{
		idMath::Fabs( normal.x ), idMath::Fabs( normal.y ), idMath::Fabs( normal.z )
	};
	const int droppedAxis = components[0] > components[1] ?
							( components[0] > components[2] ? 0 : 2 ) :
							( components[1] > components[2] ? 1 : 2 );
	float sign = 0.0f;
	for( size_t i = 0; i < boundary.vertices.size(); ++i )
	{
		const Vec3& a = file.vertices[boundary.vertices[i]];
		const Vec3& b = file.vertices[boundary.vertices[( i + 1 ) % boundary.vertices.size()]];
		const Vec3& c = file.vertices[boundary.vertices[( i + 2 ) % boundary.vertices.size()]];
		float ax, ay, bx, by, cx, cy;
		Project( a, droppedAxis, ax, ay );
		Project( b, droppedAxis, bx, by );
		Project( c, droppedAxis, cx, cy );
		const float cross = ( bx - ax ) * ( cy - by ) - ( by - ay ) * ( cx - bx );

		if( idMath::Fabs( cross ) <= 1.0e-5f )
		{
			continue;
		}

		if( sign == 0.0f )
		{
			sign = cross;
		}
		else if( ( sign > 0.0f ) != ( cross > 0.0f ) )
		{
			return false;
		}
	}
	return sign != 0.0f;
}

/*
============
RebuildAreas
============
*/
static void RebuildAreas( File& file, DisjointSet& sets )
{
	std::unordered_map<uint32, std::vector<uint32>> groups;
	for( size_t area = 0; area < file.areas.size(); ++area )
	{
		groups[sets.Find( static_cast<uint32>( area ) )].push_back(
			static_cast<uint32>( area ) );
	}

	std::vector<Area> rebuilt;
	rebuilt.reserve( groups.size() );

	for( const auto& entry : groups )
	{
		Boundary boundary;
		if( !BuildBoundary( file, entry.second, boundary ) )
		{
			continue;
		}
		Area area = file.areas[entry.second.front()];
		area.edges = boundary.edges;
		area.reachabilities.clear();
		area.center = Vec3{};
		area.floorNormal = Vec3{};
		area.bounds.mins = {std::numeric_limits<float>::max(),
							std::numeric_limits<float>::max(),
							std::numeric_limits<float>::max()
						   };
		area.bounds.maxs = { -std::numeric_limits<float>::max(),
							 -std::numeric_limits<float>::max(),
							 -std::numeric_limits<float>::max()
						   };
		for( uint32 vertexIndex : boundary.vertices )
		{
			const Vec3& vertex = file.vertices[vertexIndex];
			area.center = Add( area.center, vertex );
			area.bounds.mins.x = Min( area.bounds.mins.x, vertex.x );
			area.bounds.mins.y = Min( area.bounds.mins.y, vertex.y );
			area.bounds.mins.z = Min( area.bounds.mins.z, vertex.z );
			area.bounds.maxs.x = Max( area.bounds.maxs.x, vertex.x );
			area.bounds.maxs.y = Max( area.bounds.maxs.y, vertex.y );
			area.bounds.maxs.z = Max( area.bounds.maxs.z, vertex.z );
		}

		area.center = Scale( area.center, 1.0f / boundary.vertices.size() );
		for( uint32 member : entry.second )
		{
			area.floorNormal = Add( area.floorNormal, file.areas[member].floorNormal );
		}
		area.floorNormal = Normalize( area.floorNormal );
		rebuilt.push_back( std::move( area ) );
	}
	file.areas = std::move( rebuilt );

	std::vector<uint32> remap( file.edges.size(), std::numeric_limits<uint32>::max() );
	std::vector<Edge> compactEdges;

	for( Area& area : file.areas )
	{
		for( uint32& edge : area.edges )
		{
			if( remap[edge] == std::numeric_limits<uint32>::max() )
			{
				remap[edge] = static_cast<uint32>( compactEdges.size() );
				compactEdges.push_back( file.edges[edge] );
			}
			edge = remap[edge];
		}
	}

	file.edges = std::move( compactEdges );
}

/*
============
MergeCoplanarFloors
============
*/
FloorMergeResult MergeCoplanarFloors( File& file, float normalDegrees, float planeDistanceEpsilon )
{
	FloorMergeResult result;
	if( file.areas.size() < 2 )
	{
		return result;
	}

	std::vector<std::vector<uint32>> owners( file.edges.size() );
	for( size_t area = 0; area < file.areas.size(); ++area )
	{
		for( uint32 edge : file.areas[area].edges )
		{
			owners[edge].push_back( static_cast<uint32>( area ) );
		}
	}

	std::vector<std::pair<uint32, uint32>> candidates;
	for( const auto& edgeOwners : owners )
	{
		if( edgeOwners.size() == 2 )
		{
			candidates.emplace_back( edgeOwners[0], edgeOwners[1] );
		}
	}

	const float clampedDegrees = Max( 0.0f, Min( 90.0f, normalDegrees ) );
	const float minimumNormalDot = idMath::Cos( clampedDegrees * idMath::PI / 180.0f );
	const float distanceEpsilon = Max( 0.0f, planeDistanceEpsilon );
	DisjointSet sets( file.areas.size() );
	bool changed = true;

	while( changed )
	{
		changed = false;
		for( const auto& candidate : candidates )
		{
			const uint32 firstRoot = sets.Find( candidate.first );
			const uint32 secondRoot = sets.Find( candidate.second );
			if( firstRoot == secondRoot )
			{
				continue;
			}
			const std::vector<uint32> members =
				sets.Combined( firstRoot, secondRoot );
			Boundary boundary;
			if( !Coplanar( file, members, minimumNormalDot, distanceEpsilon ) ||
					!BuildBoundary( file, members, boundary ) ||
					!ConvexBoundary( file, boundary,
									 file.areas[members.front()].floorNormal ) )
			{
				continue;
			}
			sets.Join( firstRoot, secondRoot );
			++result.mergedPairs;
			changed = true;
		}
	}

	if( result.mergedPairs != 0 )
	{
		RebuildAreas( file, sets );
	}
	return result;
}
