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

#include "AAS2_Compiler.h"
#include "AAS2_Floor_Merge.h"
#include "AAS2_Tree.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/*
============
ReportProgress
============
*/
static void ReportProgress( ProgressCallback callback, void* userData, const char* stage, size_t current, size_t total )
{
	if( callback == nullptr )
	{
		return;
	}

	const size_t interval = Max<size_t>( 1, total / 100 );
	if( current == 0 || current == total || current % interval == 0 )
	{
		callback( stage, current, total, userData );
	}
}

/*
============
Subtract
============
*/
static Vec3 Subtract( const Vec3& left, const Vec3& right )
{
	return {left.x - right.x, left.y - right.y, left.z - right.z};
}

/*
============
Cross
============
*/
static Vec3 Cross( const Vec3& left, const Vec3& right )
{
	return
	{
		left.y* right.z - left.z * right.y,
		left.z* right.x - left.x * right.z,
		left.x* right.y - left.y* right.x
	};
}

/*
============
Length
============
*/
static float Length( const Vec3& value )
{
	return std::sqrt( value.x * value.x + value.y * value.y + value.z * value.z );
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
	const float length = Length( value );
	return length > 1.0e-12f ? Scale( value, 1.0f / length ) : Vec3{};
}

/*
============
TriangleCenter
============
*/
static Vec3 TriangleCenter( const Vec3& a, const Vec3& b, const Vec3& c )
{
	return {( a.x + b.x + c.x ) / 3.0f,
			( a.y + b.y + c.y ) / 3.0f,
			( a.z + b.z + c.z ) / 3.0f};
}

/*
============
TriangleBounds
============
*/
static Bounds TriangleBounds( const Vec3& a, const Vec3& b, const Vec3& c )
{
	Bounds bounds;
	bounds.mins = {Min( a.x, Min( b.x, c.x ) ),
				   Min( a.y, Min( b.y, c.y ) ),
				   Min( a.z, Min( b.z, c.z ) )
				  };
	bounds.maxs = {Max( a.x, Max( b.x, c.x ) ),
				   Max( a.y, Max( b.y, c.y ) ),
				   Max( a.z, Max( b.z, c.z ) )
				  };
	return bounds;
}

/*
============
Near
============
*/
static bool Near( const Vec3& left, const Vec3& right, float epsilon )
{
	return idMath::Fabs( left.x - right.x ) <= epsilon &&
		   idMath::Fabs( left.y - right.y ) <= epsilon &&
		   idMath::Fabs( left.z - right.z ) <= epsilon;
}

struct VertexCell
{
	long long x;
	long long y;
	long long z;
	bool operator==( const VertexCell& other ) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

struct VertexCellHash
{
	size_t operator()( const VertexCell& cell ) const
	{
		const size_t first = std::hash<long long>()( cell.x );
		const size_t second = std::hash<long long>()( cell.y );
		const size_t third = std::hash<long long>()( cell.z );
		return first ^ ( second + 0x9e3779b9u + ( first << 6 ) + ( first >> 2 ) ) ^
			   ( third + 0x9e3779b9u + ( second << 6 ) + ( second >> 2 ) );
	}
};

class VertexWelder
{
public:
	VertexWelder( std::vector<Vec3>& output, float tolerance,
				  size_t expectedVertices )
		: vertices( output ), epsilon( Max( 0.0f, tolerance ) ),
		  cellSize( Max( 1.0e-6f, tolerance ) )
	{
		buckets.reserve( expectedVertices );
	}

	uint32 Weld( const Vec3& vertex )
	{
		const VertexCell center = Cell( vertex );
		for( long long x = center.x - 1; x <= center.x + 1; ++x )
		{
			for( long long y = center.y - 1; y <= center.y + 1; ++y )
			{
				for( long long z = center.z - 1; z <= center.z + 1; ++z )
				{
					const auto range = buckets.equal_range( VertexCell{x, y, z} );
					for( auto found = range.first; found != range.second; ++found )
					{
						if( Near( vertex, vertices[found->second], epsilon ) )
						{
							return found->second;
						}
					}
				}
			}
		}
		const uint32 index = static_cast<uint32>( vertices.size() );
		vertices.push_back( vertex );
		buckets.emplace( center, index );
		return index;
	}

private:
	VertexCell Cell( const Vec3& vertex ) const
	{
		return {static_cast<long long>( std::floor( vertex.x / cellSize ) ),
				static_cast<long long>( std::floor( vertex.y / cellSize ) ),
				static_cast<long long>( std::floor( vertex.z / cellSize ) )};
	}

	std::vector<Vec3>& vertices;
	float epsilon;
	float cellSize;
	std::unordered_multimap<VertexCell, uint32, VertexCellHash> buckets;
};

/*
============
EdgeKey
============
*/
static uint64 EdgeKey( uint32 first, uint32 second )
{
	const uint32 low = Min( first, second );
	const uint32 high = Max( first, second );
	return ( static_cast<uint64>( low ) << 32 ) | high;
}

/*
============
GridCell
============
*/
static int GridCell( float coordinate, float cellSize )
{
	return static_cast<int>( std::floor( coordinate / cellSize ) );
}

/*
============
GridKey
============
*/
static uint64 GridKey( int x, int y )
{
	return ( static_cast<uint64>( static_cast<uint32>( x ) ) << 32 ) |
		   static_cast<uint32>( y );
}

/*
============
EdgeUse
============
*/
struct EdgeUse
{
	uint32 edge = 0;
	std::vector<uint32> areas;
};

/*
============
TriangleKey
============
*/
struct TriangleKey
{
	uint32 vertices[3] {};

	bool operator==( const TriangleKey& other ) const
	{
		return vertices[0] == other.vertices[0] &&
			   vertices[1] == other.vertices[1] &&
			   vertices[2] == other.vertices[2];
	}
};

/*
============
TriangleKeyHash
============
*/
struct TriangleKeyHash
{
	size_t operator()( const TriangleKey& key ) const
	{
		size_t value = std::hash<uint32>()( key.vertices[0] );
		for( int i = 1; i < 3; ++i )
		{
			const size_t next =
				std::hash<uint32>()( key.vertices[i] );
			value ^= next + 0x9e3779b9u + ( value << 6 ) + ( value >> 2 );
		}
		return value;
	}
};

/*
============
VerticalTransitionPoints
============
*/
static bool VerticalTransitionPoints( const Edge& firstEdge, const Edge& secondEdge, const std::vector<Vec3>& vertices, float epsilon,
									  float maxHorizontalGap, Vec3& firstPoint, Vec3& secondPoint )
{
	const Vec3& a = vertices[firstEdge.vertices[0]];
	const Vec3& b = vertices[firstEdge.vertices[1]];
	const Vec3& c = vertices[secondEdge.vertices[0]];
	const Vec3& d = vertices[secondEdge.vertices[1]];
	const float abX = b.x - a.x;
	const float abY = b.y - a.y;
	const float cdX = d.x - c.x;
	const float cdY = d.y - c.y;
	const float abLengthSquared = abX * abX + abY * abY;
	const float cdLengthSquared = cdX * cdX + cdY * cdY;

	if( abLengthSquared <= epsilon * epsilon || cdLengthSquared <= epsilon * epsilon )
	{
		return false;
	}

	const float directionCross = abX * cdY - abY * cdX;
	const float offsetCross = ( c.x - a.x ) * abY - ( c.y - a.y ) * abX;

	if( idMath::Fabs( directionCross ) > epsilon *
			std::sqrt( abLengthSquared * cdLengthSquared ) ||
			idMath::Fabs( offsetCross ) > maxHorizontalGap *
			std::sqrt( abLengthSquared ) )
	{
		return false;
	}

	const float cParameter = ( ( c.x - a.x ) * abX + ( c.y - a.y ) * abY ) /
							 abLengthSquared;
	const float dParameter = ( ( d.x - a.x ) * abX + ( d.y - a.y ) * abY ) /
							 abLengthSquared;
	const float overlapStart = Max( 0.0f, Min( cParameter, dParameter ) );
	const float overlapEnd = Min( 1.0f, Max( cParameter, dParameter ) );
	if( ( overlapEnd - overlapStart ) * std::sqrt( abLengthSquared ) <= epsilon )
	{
		return false;
	}
	const float firstParameter = ( overlapStart + overlapEnd ) * 0.5f;
	firstPoint = {a.x + ( b.x - a.x )* firstParameter,
				  a.y + ( b.y - a.y )* firstParameter,
				  a.z + ( b.z - a.z )* firstParameter
				 };
	const float secondParameter =
		( ( firstPoint.x - c.x ) * cdX + ( firstPoint.y - c.y ) * cdY ) /
		cdLengthSquared;
	secondPoint = {c.x + ( d.x - c.x )* secondParameter,
				   c.y + ( d.y - c.y )* secondParameter,
				   c.z + ( d.z - c.z )* secondParameter
				  };
	// The spans already overlap. Identical transition points are valid for a
	// coplanar T-junction whose two sides use different edge subdivisions.
	return true;
}

/*
============
ClosestTransitionPoints

Return the closest points of two finite floor-boundary segments in XY.
============
*/
static bool ClosestTransitionPoints( const Edge& firstEdge, const Edge& secondEdge, const std::vector<Vec3>& vertices,
									 float maxHorizontalGap, Vec3& firstPoint, Vec3& secondPoint )
{
	const Vec3& a = vertices[firstEdge.vertices[0]];
	const Vec3& b = vertices[firstEdge.vertices[1]];
	const Vec3& c = vertices[secondEdge.vertices[0]];
	const Vec3& d = vertices[secondEdge.vertices[1]];
	const float ux = b.x - a.x;
	const float uy = b.y - a.y;
	const float vx = d.x - c.x;
	const float vy = d.y - c.y;
	const float wx = a.x - c.x;
	const float wy = a.y - c.y;
	const float uu = ux * ux + uy * uy;
	const float vv = vx * vx + vy * vy;
	const float uv = ux * vx + uy * vy;
	const float uw = ux * wx + uy * wy;
	const float vw = vx * wx + vy * wy;

	if( uu <= 1.0e-8f || vv <= 1.0e-8f )
	{
		return false;
	}

	float firstParameter = 0.0f;
	float secondParameter = 0.0f;
	const float denominator = uu * vv - uv * uv;

	if( idMath::Fabs( denominator ) > 1.0e-8f * uu * vv )
	{
		firstParameter = ( uv * vw - vv * uw ) / denominator;
	}

	firstParameter = Max( 0.0f, Min( 1.0f, firstParameter ) );
	secondParameter = ( uv * firstParameter + vw ) / vv;

	if( secondParameter < 0.0f )
	{
		secondParameter = 0.0f;
		firstParameter = Max( 0.0f, Min( 1.0f, -uw / uu ) );
	}
	else if( secondParameter > 1.0f )
	{
		secondParameter = 1.0f;
		firstParameter = Max( 0.0f, Min( 1.0f, ( uv - uw ) / uu ) );
	}

	firstPoint = {a.x + ux * firstParameter,
				  a.y + uy * firstParameter,
				  a.z + ( b.z - a.z )* firstParameter
				 };
	secondPoint = {c.x + vx * secondParameter,
				   c.y + vy * secondParameter,
				   c.z + ( d.z - c.z )* secondParameter
				  };

	const float dx = firstPoint.x - secondPoint.x;
	const float dy = firstPoint.y - secondPoint.y;
	return dx * dx + dy * dy <= maxHorizontalGap * maxHorizontalGap;
}

/*
============
AddReachability
============
*/
static void AddReachability( File& output, uint32 fromArea, uint32 toArea, const Vec3& start, const Vec3& end, uint32 travelFlags )
{
	Reachability reachability;
	reachability.fromArea = fromArea;
	reachability.toArea = toArea;
	reachability.travelFlags = travelFlags;
	reachability.start = start;
	reachability.end = end;
	const uint32 index = static_cast<uint32>( output.reachabilities.size() );
	output.reachabilities.push_back( std::move( reachability ) );
	output.areas[fromArea].reachabilities.push_back( index );
}

/*
============
AddReachabilityPair
============
*/
static void AddReachabilityPair( File& output, uint32 firstArea, uint32 secondArea, const Vec3& firstPoint, const Vec3& secondPoint )
{
	const uint32 areas[2] = {firstArea, secondArea};
	const Vec3 points[2] = {firstPoint, secondPoint};
	for( int direction = 0; direction < 2; ++direction )
	{
		AddReachability( output, areas[direction], areas[1 - direction],
						 points[direction], points[1 - direction],
						 AAS2_TRAVEL_WALK );
	}
}

/*
============
CompileSurface
============
*/
Result CompileSurface( const SourceGeometry& source, const Settings& settings, File& output, ProgressCallback progress, void* progressUserData )
{
	output = File{};
	Result result;
	result.statistics.inputTriangles = source.triangles.size();

	if( source.vertices.empty() || source.triangles.empty() )
	{
		result.error = Error::emptyGeometry;
		return result;
	}

	const float clampedSlope = Max( 0.0f, Min( 89.9f, settings.maxFloorSlopeDegrees ) );
	const float minFloorZ = idMath::Cos( clampedSlope * idMath::PI / 180.0f );
	const float weldEpsilon = Max( 0.0f, settings.weldEpsilon );

	std::unordered_map<uint64, EdgeUse> edgeUses;
	std::unordered_set<TriangleKey, TriangleKeyHash> triangleUses;
	triangleUses.reserve( source.triangles.size() );
	VertexWelder vertexWelder( output.vertices, weldEpsilon, source.vertices.size() );
	ReportProgress( progress, progressUserData, "area graph", 0, source.triangles.size() );

	for( size_t triangleIndex = 0; triangleIndex < source.triangles.size(); ++triangleIndex )
	{
		ReportProgress( progress, progressUserData, "area graph", triangleIndex, source.triangles.size() );
		const SourceTriangle& triangle = source.triangles[triangleIndex];

		for( int corner = 0; corner < 3; ++corner )
		{
			if( triangle.vertices[corner] >= source.vertices.size() )
			{
				result.error = Error::invalidVertexIndex;
				result.sourceTriangle = triangleIndex;
				return result;
			}
		}

		const Vec3& sourceA = source.vertices[triangle.vertices[0]];
		const Vec3& sourceB = source.vertices[triangle.vertices[1]];
		const Vec3& sourceC = source.vertices[triangle.vertices[2]];
		const Vec3 normal = Normalize( Cross( Subtract( sourceB, sourceA ),
											  Subtract( sourceC, sourceA ) ) );
		if( Length( normal ) <= 1.0e-8f )
		{
			++result.statistics.rejectedDegenerateTriangles;
			continue;
		}
		if( normal.z < minFloorZ )
		{
			++result.statistics.rejectedSteepTriangles;
			continue;
		}

		const uint32 vertices[3] =
		{
			vertexWelder.Weld( sourceA ),
			vertexWelder.Weld( sourceB ),
			vertexWelder.Weld( sourceC )
		};
		if( vertices[0] == vertices[1] || vertices[0] == vertices[2] ||
				vertices[1] == vertices[2] )
		{
			++result.statistics.rejectedDegenerateTriangles;
			continue;
		}

		TriangleKey triangleKey;
		triangleKey.vertices[0] = vertices[0];
		triangleKey.vertices[1] = vertices[1];
		triangleKey.vertices[2] = vertices[2];
		std::sort( triangleKey.vertices, triangleKey.vertices + 3 );

		if( !triangleUses.insert( triangleKey ).second )
		{
			++result.statistics.rejectedDuplicateTriangles;
			continue;
		}

		Area area;
		area.center = TriangleCenter( output.vertices[vertices[0]],
									  output.vertices[vertices[1]],
									  output.vertices[vertices[2]] );
		area.bounds = TriangleBounds( output.vertices[vertices[0]],
									  output.vertices[vertices[1]],
									  output.vertices[vertices[2]] );
		area.floorNormal = normal;
		const uint32 areaIndex = static_cast<uint32>( output.areas.size() );

		for( int edgeCorner = 0; edgeCorner < 3; ++edgeCorner )
		{
			const uint32 first = vertices[edgeCorner];
			const uint32 second = vertices[( edgeCorner + 1 ) % 3];
			const uint64 key = EdgeKey( first, second );
			auto found = edgeUses.find( key );
			if( found == edgeUses.end() )
			{
				Edge edge;
				edge.vertices[0] = Min( first, second );
				edge.vertices[1] = Max( first, second );
				const uint32 edgeIndex =
					static_cast<uint32>( output.edges.size() );
				output.edges.push_back( edge );
				EdgeUse use;
				use.edge = edgeIndex;
				found = edgeUses.emplace( key, std::move( use ) ).first;
			}
			found->second.areas.push_back( areaIndex );
			area.edges.push_back( found->second.edge );
		}

		output.areas.push_back( std::move( area ) );
		++result.statistics.walkableTriangles;
	}

	ReportProgress( progress, progressUserData, "area graph", source.triangles.size(), source.triangles.size() );

	if( output.areas.empty() )
	{
		result.error = Error::emptyGeometry;
		return result;
	}

	for( const auto& entry : edgeUses )
	{
		const EdgeUse& use = entry.second;
		if( use.areas.size() > 2 )
		{
			++result.statistics.nonManifoldEdges;
		}
	}

	if( settings.mergeCoplanarFloors )
	{
		ReportProgress( progress, progressUserData, "coplanar floor merge", 0, 1 );
		result.statistics.mergedFloorPairs =
			MergeCoplanarFloors( output,
								 settings.floorMergeNormalDegrees,
								 settings.floorMergeDistanceEpsilon ).mergedPairs;
		ReportProgress( progress, progressUserData, "coplanar floor merge", 1, 1 );
	}

	std::vector<std::vector<uint32>> finalEdgeOwners( output.edges.size() );
	for( size_t area = 0; area < output.areas.size(); ++area )
	{
		for( uint32 edge : output.areas[area].edges )
		{
			finalEdgeOwners[edge].push_back( static_cast<uint32>( area ) );
		}
	}

	std::unordered_set<uint64> linkedAreaPairs;
	ReportProgress( progress, progressUserData, "shared-edge reachability", 0, finalEdgeOwners.size() );

	for( size_t edgeIndex = 0; edgeIndex < finalEdgeOwners.size(); ++edgeIndex )
	{
		ReportProgress( progress, progressUserData, "shared-edge reachability",
						edgeIndex, finalEdgeOwners.size() );
		const std::vector<uint32>& owners = finalEdgeOwners[edgeIndex];
		if( owners.size() != 2 )
		{
			continue;
		}

		const uint64 pairKey = EdgeKey( owners[0], owners[1] );
		if( linkedAreaPairs.count( pairKey ) != 0 )
		{
			continue;
		}

		const Edge& edge = output.edges[edgeIndex];
		const Vec3& first = output.vertices[edge.vertices[0]];
		const Vec3& second = output.vertices[edge.vertices[1]];
		const Vec3 midpoint{( first.x + second.x ) * 0.5f,
				  ( first.y + second.y ) * 0.5f,
				  ( first.z + second.z ) * 0.5f};

		AddReachabilityPair( output, owners[0], owners[1], midpoint, midpoint );
		linkedAreaPairs.insert( pairKey );
	}
	ReportProgress( progress, progressUserData, "shared-edge reachability",
					finalEdgeOwners.size(), finalEdgeOwners.size() );

	const float stepEpsilon = Max( 0.01f, weldEpsilon );

	// CSG clipping can leave neighboring coplanar floor boundaries a small
	// distance apart even though an agent can cross the seam. The old test
	// accepted only a vertical Z difference and therefore discarded every
	// coplanar seam that did not weld into one shared edge.
	const float seamTolerance = Max( 0.5f, weldEpsilon * 4.0f );
	const float maxStepHeight = Max( 0.0f, settings.maxStepHeight );

	// Configuration-space expansion bevels stair corners by the agent radius.
	// Adjacent treads can therefore have no shared/collinear edge even though
	// their vertical rise is a legal step. Search one agent radius sideways
	// for non-coplanar transitions; coplanar gaps remain restricted below to
	// seamTolerance so this cannot bridge through ordinary walls.
	const float stepHorizontalTolerance = Max(
			seamTolerance,
			Min( Max( 0.0f, settings.agentRadius ), maxStepHeight ) );
	const float maxFallHeight = Max( maxStepHeight, settings.maxFallHeight );
	const float maxLedgeGrabHeight = Max( maxStepHeight, settings.agentHeight );
	const float reachCellSize = Max( 64.0f, settings.agentRadius * 4.0f );
	const int maximumEdgeCells = 4096;

	std::unordered_map<uint64, std::vector<uint32>> reachCells;
	std::vector<uint32> globalBoundaryEdges;
	std::vector<uint32> boundaryEdges;
	boundaryEdges.reserve( finalEdgeOwners.size() );
	reachCells.reserve( finalEdgeOwners.size() );

	for( size_t edgeIndex = 0; edgeIndex < finalEdgeOwners.size(); ++edgeIndex )
	{
		if( finalEdgeOwners[edgeIndex].size() != 1 )
		{
			continue;
		}

		boundaryEdges.push_back( static_cast<uint32>( edgeIndex ) );
		const Edge& edge = output.edges[edgeIndex];
		const Vec3& first = output.vertices[edge.vertices[0]];
		const Vec3& second = output.vertices[edge.vertices[1]];
		const int minX = GridCell( Min( first.x, second.x ) - stepHorizontalTolerance,
								   reachCellSize );
		const int maxX = GridCell( Max( first.x, second.x ) + stepHorizontalTolerance,
								   reachCellSize );
		const int minY = GridCell( Min( first.y, second.y ) - stepHorizontalTolerance,
								   reachCellSize );
		const int maxY = GridCell( Max( first.y, second.y ) + stepHorizontalTolerance,
								   reachCellSize );
		const long long width = static_cast<long long>( maxX ) - minX + 1;
		const long long cellHeight = static_cast<long long>( maxY ) - minY + 1;

		if( width <= 0 || cellHeight <= 0 || width * cellHeight > maximumEdgeCells )
		{
			globalBoundaryEdges.push_back( static_cast<uint32>( edgeIndex ) );
			continue;
		}

		for( int x = minX; x <= maxX; ++x )
		{
			for( int y = minY; y <= maxY; ++y )
			{
				reachCells[GridKey( x, y )].push_back(
					static_cast<uint32>( edgeIndex ) );
			}
		}
	}

	std::vector<uint32> edgeStamps( finalEdgeOwners.size(), 0 );
	uint32 edgeStamp = 0;
	std::vector<uint32> secondEdgeCandidates;

	ReportProgress( progress, progressUserData, "vertical reachability", 0, boundaryEdges.size() );
	for( size_t boundaryIndex = 0; boundaryIndex < boundaryEdges.size(); ++boundaryIndex )
	{
		ReportProgress( progress, progressUserData, "vertical reachability",
						boundaryIndex, boundaryEdges.size() );
		const uint32 firstEdge = boundaryEdges[boundaryIndex];
		secondEdgeCandidates.clear();

		if( ++edgeStamp == 0 )
		{
			std::fill( edgeStamps.begin(), edgeStamps.end(), 0 );
			edgeStamp = 1;
		}

		for( uint32 edge : globalBoundaryEdges )
		{
			edgeStamps[edge] = edgeStamp;
			secondEdgeCandidates.push_back( edge );
		}

		const Edge& firstBoundary = output.edges[firstEdge];
		const Vec3& firstStart = output.vertices[firstBoundary.vertices[0]];
		const Vec3& firstEnd = output.vertices[firstBoundary.vertices[1]];

		const int minX = GridCell( Min( firstStart.x, firstEnd.x ) - stepHorizontalTolerance,
								   reachCellSize );
		const int maxX = GridCell( Max( firstStart.x, firstEnd.x ) + stepHorizontalTolerance,
								   reachCellSize );
		const int minY = GridCell( Min( firstStart.y, firstEnd.y ) - stepHorizontalTolerance,
								   reachCellSize );
		const int maxY = GridCell( Max( firstStart.y, firstEnd.y ) + stepHorizontalTolerance,
								   reachCellSize );
		const long long width = static_cast<long long>( maxX ) - minX + 1;
		const long long cellHeight = static_cast<long long>( maxY ) - minY + 1;

		if( width * cellHeight > maximumEdgeCells )
		{
			secondEdgeCandidates = boundaryEdges;
		}
		else
		{
			for( int x = minX; x <= maxX; ++x )
			{
				for( int y = minY; y <= maxY; ++y )
				{
					const auto found = reachCells.find( GridKey( x, y ) );
					if( found == reachCells.end() )
					{
						continue;
					}
					for( uint32 edge : found->second )
					{
						if( edgeStamps[edge] != edgeStamp )
						{
							edgeStamps[edge] = edgeStamp;
							secondEdgeCandidates.push_back( edge );
						}
					}
				}
			}
		}

		std::sort( secondEdgeCandidates.begin(), secondEdgeCandidates.end() );
		for( uint32 secondEdge : secondEdgeCandidates )
		{
			if( secondEdge <= firstEdge )
			{
				continue;
			}
			const uint32 firstArea = finalEdgeOwners[firstEdge][0];
			const uint32 secondArea = finalEdgeOwners[secondEdge][0];
			const uint64 pairKey = EdgeKey( firstArea, secondArea );
			if( firstArea == secondArea || linkedAreaPairs.count( pairKey ) != 0 )
			{
				continue;
			}
			Vec3 firstPoint;
			Vec3 secondPoint;
			bool hasTransition = VerticalTransitionPoints(
									 output.edges[firstEdge], output.edges[secondEdge],
									 output.vertices, stepEpsilon, stepHorizontalTolerance,
									 firstPoint, secondPoint );
			if( !hasTransition )
			{
				// Expanded solid corners can leave two valid floor polygons
				// meeting at a point, or two adjacent stair treads separated
				// horizontally by the expansion bevel. Collinear-span testing
				// misses both forms of transition.
				hasTransition = ClosestTransitionPoints(
									output.edges[firstEdge], output.edges[secondEdge],
									output.vertices, stepHorizontalTolerance,
									firstPoint, secondPoint );
			}

			if( !hasTransition )
			{
				continue;
			}

			const float height = idMath::Fabs( firstPoint.z - secondPoint.z );
			const float transitionDx = firstPoint.x - secondPoint.x;
			const float transitionDy = firstPoint.y - secondPoint.y;
			const float horizontalDistanceSquared =
				transitionDx * transitionDx + transitionDy * transitionDy;

			if( height <= stepEpsilon &&
					horizontalDistanceSquared > seamTolerance * seamTolerance )
			{
				continue;
			}
			if( height <= maxStepHeight + stepEpsilon )
			{
				AddReachabilityPair( output, firstArea, secondArea,
									 firstPoint, secondPoint );
				result.statistics.stepReachabilities += 2;
			}
			else
			{
				const bool firstIsHigh = firstPoint.z > secondPoint.z;
				const uint32 highArea = firstIsHigh ? firstArea : secondArea;
				const uint32 lowArea = firstIsHigh ? secondArea : firstArea;
				const Vec3& highPoint = firstIsHigh ? firstPoint : secondPoint;
				const Vec3& lowPoint = firstIsHigh ? secondPoint : firstPoint;
				if( height <= maxFallHeight + stepEpsilon )
				{
					AddReachability( output, highArea, lowArea, highPoint, lowPoint,
									 AAS2_TRAVEL_WALK_OFF_LEDGE );
					++result.statistics.fallReachabilities;
				}
				if( height <= maxLedgeGrabHeight + stepEpsilon )
				{
					AddReachability( output, lowArea, highArea, lowPoint, highPoint,
									 AAS2_TRAVEL_LEDGE_GRAB );
					++result.statistics.ledgeGrabReachabilities;
				}
				if( height > maxFallHeight + stepEpsilon &&
						height > maxLedgeGrabHeight + stepEpsilon )
				{
					continue;
				}
			}
			linkedAreaPairs.insert( pairKey );
		}
	}

	ReportProgress( progress, progressUserData, "vertical reachability", boundaryEdges.size(), boundaryEdges.size() );

	ReportProgress( progress, progressUserData, "area tree", 0, 1 );
	BuildAreaTree( output );
	ReportProgress( progress, progressUserData, "area tree", 1, 1 );

	result.statistics.vertices = output.vertices.size();
	result.statistics.edges = output.edges.size();
	result.statistics.areas = output.areas.size();
	result.statistics.reachabilities = output.reachabilities.size();

	if( !ValidateFile( output ) )
	{
		output = File{};
		result.error = Error::invalidOutput;
	}
	return result;
}

/*
============
AddDynamicVolumeReachabilities
============
*/
size_t AddDynamicVolumeReachabilities( File& file,
									   const Settings& settings )
{
	if( file.areas.empty() || file.edges.empty() )
	{
		return 0;
	}
	const uint16 dynamicFlags = AAS2_AREA_CLUSTER_PORTAL | AAS2_AREA_OBSTACLE;

	// A dynamic doorway is authored as an intentional connection.  C-space
	// expansion can separate its floor fragment from the adjoining room by
	// as much as one actor radius, while ordinary seams must retain the much
	// tighter weld tolerance.
	const float tolerance = Max(
								Max( 0.5f, settings.weldEpsilon * 4.0f ),
								Min( 16.0f, Max( 0.0f, settings.agentRadius ) ) );
	const float cellSize = Max( 64.0f, settings.agentRadius * 4.0f );
	const float maxStepHeight = Max( 0.0f, settings.maxStepHeight ) +
								Max( 0.01f, settings.weldEpsilon );

	std::vector<std::vector<uint32>> owners( file.edges.size() );
	for( size_t area = 0; area < file.areas.size(); ++area )
	{
		for( uint32 edge : file.areas[area].edges )
		{
			if( edge < owners.size() ) owners[edge].push_back(
					static_cast<uint32>( area ) );
		}
	}

	std::unordered_set<uint64> linkedPairs;
	for( const Reachability& reach : file.reachabilities )
	{
		linkedPairs.insert( EdgeKey( reach.fromArea, reach.toArea ) );
	}

	std::unordered_map<uint64, std::vector<uint32>> cells;
	std::vector<uint32> boundaries;

	for( size_t edgeIndex = 0; edgeIndex < owners.size(); ++edgeIndex )
	{
		if( owners[edgeIndex].size() != 1 )
		{
			continue;
		}
		boundaries.push_back( static_cast<uint32>( edgeIndex ) );
		const Edge& edge = file.edges[edgeIndex];
		const Vec3& a = file.vertices[edge.vertices[0]];
		const Vec3& b = file.vertices[edge.vertices[1]];
		const int minX = GridCell( Min( a.x, b.x ) - tolerance, cellSize );
		const int maxX = GridCell( Max( a.x, b.x ) + tolerance, cellSize );
		const int minY = GridCell( Min( a.y, b.y ) - tolerance, cellSize );
		const int maxY = GridCell( Max( a.y, b.y ) + tolerance, cellSize );

		for( int x = minX; x <= maxX; ++x )
		{
			for( int y = minY; y <= maxY; ++y )
			{
				cells[GridKey( x, y )].push_back(
					static_cast<uint32>( edgeIndex ) );
			}
		}
	}

	std::vector<uint32> stamps( file.edges.size(), 0 );
	uint32 stamp = 0;
	size_t added = 0;

	for( uint32 firstEdge : boundaries )
	{
		const uint32 firstArea = owners[firstEdge][0];
		if( ( file.areas[firstArea].flags & dynamicFlags ) == 0 )
		{
			continue;
		}
		if( ++stamp == 0 )
		{
			std::fill( stamps.begin(), stamps.end(), 0 );
			stamp = 1;
		}
		const Edge& edge = file.edges[firstEdge];
		const Vec3& a = file.vertices[edge.vertices[0]];
		const Vec3& b = file.vertices[edge.vertices[1]];
		const int minX = GridCell( Min( a.x, b.x ) - tolerance, cellSize );
		const int maxX = GridCell( Max( a.x, b.x ) + tolerance, cellSize );
		const int minY = GridCell( Min( a.y, b.y ) - tolerance, cellSize );
		const int maxY = GridCell( Max( a.y, b.y ) + tolerance, cellSize );

		for( int x = minX; x <= maxX; ++x ) for( int y = minY; y <= maxY; ++y )
			{
				const auto found = cells.find( GridKey( x, y ) );
				if( found == cells.end() )
				{
					continue;
				}
				for( uint32 secondEdge : found->second )
				{
					if( secondEdge == firstEdge || stamps[secondEdge] == stamp )
					{
						continue;
					}
					stamps[secondEdge] = stamp;
					const uint32 secondArea = owners[secondEdge][0];
					const uint64 key = EdgeKey( firstArea, secondArea );
					if( firstArea == secondArea || linkedPairs.count( key ) != 0 )
					{
						continue;
					}
					Vec3 firstPoint, secondPoint;
					if( !ClosestTransitionPoints( file.edges[firstEdge],
												  file.edges[secondEdge],
												  file.vertices, tolerance,
												  firstPoint, secondPoint ) ||
							idMath::Fabs( firstPoint.z - secondPoint.z ) > maxStepHeight )
					{
						continue;
					}
					AddReachabilityPair( file, firstArea, secondArea,
										 firstPoint, secondPoint );
					linkedPairs.insert( key );
					added += 2;
				}
			}
	}
	return added;
}

/*
============
ValidateFile
============
*/
bool ValidateFile( const File& file )
{
	if( file.areas.empty() || file.trees.empty() )
	{
		return false;
	}

	for( const Edge& edge : file.edges )
	{
		if( edge.vertices[0] >= file.vertices.size() ||
				edge.vertices[1] >= file.vertices.size() ||
				edge.vertices[0] == edge.vertices[1] )
		{
			return false;
		}
	}

	for( size_t areaIndex = 0; areaIndex < file.areas.size(); ++areaIndex )
	{
		const Area& area = file.areas[areaIndex];
		if( area.tree >= file.trees.size() || area.edges.size() < 3 )
		{
			return false;
		}
		for( uint32 edge : area.edges )
		{
			if( edge >= file.edges.size() )
			{
				return false;
			}
		}
		for( uint32 reachability : area.reachabilities )
		{
			if( reachability >= file.reachabilities.size() ||
					file.reachabilities[reachability].fromArea != areaIndex )
			{
				return false;
			}
		}
	}

	for( const Reachability& reachability : file.reachabilities )
	{
		if( reachability.fromArea >= file.areas.size() ||
				reachability.toArea >= file.areas.size() ||
				reachability.fromArea == reachability.toArea ||
				reachability.travelFlags == 0 )
		{
			return false;
		}
	}

	for( const Tree& tree : file.trees )
	{
		if( tree.rootNode >= 0 &&
				static_cast<size_t>( tree.rootNode ) >= file.nodes.size() )
		{
			return false;
		}
		if( tree.rootNode < 0 &&
				static_cast<size_t>( -tree.rootNode - 1 ) >= file.areas.size() )
		{
			return false;
		}
		for( uint32 area : tree.areas )
		{
			if( area >= file.areas.size() )
			{
				return false;
			}
		}
	}

	for( const Node& node : file.nodes )
	{
		for( int32 child : node.children )
		{
			if( ( child >= 0 && static_cast<size_t>( child ) >= file.nodes.size() ) ||
					( child < 0 && static_cast<size_t>( -child - 1 ) >= file.areas.size() ) )
			{
				return false;
			}
		}
	}
	return true;
}

/*
============
ErrorName
============
*/
const char* ErrorName( Error error )
{
	switch( error )
	{
		case Error::none:
			return "none";
		case Error::emptyGeometry:
			return "empty geometry";
		case Error::invalidVertexIndex:
			return "invalid vertex index";
		case Error::degenerateTriangle:
			return "degenerate triangle";
		case Error::nonManifoldEdge:
			return "non-manifold edge";
		case Error::invalidOutput:
			return "invalid output";
	}
	return "unknown";
}