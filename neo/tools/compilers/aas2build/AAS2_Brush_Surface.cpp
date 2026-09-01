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

#include "AAS2_Brush_Surface.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
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
Add
============
*/
static Vec3 Add( const Vec3& left, const Vec3& right )
{
	return {left.x + right.x, left.y + right.y, left.z + right.z};
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
Scale
============
*/
static Vec3 Scale( const Vec3& value, float scale )
{
	return {value.x * scale, value.y * scale, value.z * scale};
}

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
	return std::sqrt( Dot( value, value ) );
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
Finite
============
*/
static bool Finite( float value )
{
	return std::isfinite( value ) != 0;
}

/*
============
Finite
============
*/
static bool Finite( const Vec3& value )
{
	return Finite( value.x ) && Finite( value.y ) && Finite( value.z );
}

/*
============
Near
============
*/
static bool Near( const Vec3& left, const Vec3& right, float epsilon )
{
	return Length( Subtract( left, right ) ) <= epsilon;
}

/*
============
EmptyBounds
============
*/
static Bounds EmptyBounds()
{
	Bounds bounds;
	bounds.mins = {std::numeric_limits<float>::max(),
				   std::numeric_limits<float>::max(),
				   std::numeric_limits<float>::max()
				  };
	bounds.maxs = { -std::numeric_limits<float>::max(),
					-std::numeric_limits<float>::max(),
					-std::numeric_limits<float>::max()
				  };
	return bounds;
}

/*
============
AddPoint
============
*/
static void AddPoint( Bounds& bounds, const Vec3& point )
{
	bounds.mins.x = Min( bounds.mins.x, point.x );
	bounds.mins.y = Min( bounds.mins.y, point.y );
	bounds.mins.z = Min( bounds.mins.z, point.z );
	bounds.maxs.x = Max( bounds.maxs.x, point.x );
	bounds.maxs.y = Max( bounds.maxs.y, point.y );
	bounds.maxs.z = Max( bounds.maxs.z, point.z );
}

/*
============
PolygonBounds
============
*/
static Bounds PolygonBounds( const std::vector<Vec3>& polygon )
{
	Bounds bounds = EmptyBounds();
	for( const Vec3& point : polygon )
	{
		AddPoint( bounds, point );
	}
	return bounds;
}

/*
============
BoundsIntersect
============
*/
static bool BoundsIntersect( const Bounds& first, const Bounds& second, float epsilon )
{
	return first.maxs.x + epsilon >= second.mins.x &&
		   first.mins.x - epsilon <= second.maxs.x &&
		   first.maxs.y + epsilon >= second.mins.y &&
		   first.mins.y - epsilon <= second.maxs.y &&
		   first.maxs.z + epsilon >= second.mins.z &&
		   first.mins.z - epsilon <= second.maxs.z;
}

/*
============
NormalizePlanes
============
*/
static bool NormalizePlanes( const ConvexBrush& brush, std::vector<BrushPlane>& planes, size_t& invalidPlane )
{
	planes.reserve( brush.planes.size() );
	for( size_t i = 0; i < brush.planes.size(); ++i )
	{
		const BrushPlane& source = brush.planes[i];
		const float length = Length( source.normal );

		if( !Finite( source.normal ) || !Finite( source.distance ) || !Finite( length ) || length <= 1.0e-8f )
		{
			invalidPlane = i;
			return false;
		}

		BrushPlane plane;
		plane.normal = Scale( source.normal, 1.0f / length );
		plane.distance = source.distance / length;
		planes.push_back( plane );
	}
	return true;
}

/*
============
TripleIntersection
============
*/
static bool TripleIntersection( const BrushPlane& first, const BrushPlane& second, const BrushPlane& third, float epsilon, Vec3& point )
{
	const Vec3 secondCrossThird = Cross( second.normal, third.normal );
	const float denominator = Dot( first.normal, secondCrossThird );

	if( idMath::Fabs( denominator ) <= epsilon )
	{
		return false;
	}

	point = Scale( Add( Add( Scale( secondCrossThird, first.distance ),
							 Scale( Cross( third.normal, first.normal ), second.distance ) ),
						Scale( Cross( first.normal, second.normal ), third.distance ) ),
				   1.0f / denominator );
	return Finite( point );
}

/*
============
Inside
============
*/
static bool Inside( const std::vector<BrushPlane>& planes, const Vec3& point, float epsilon )
{
	for( const BrushPlane& plane : planes )
	{
		if( Dot( plane.normal, point ) > plane.distance + epsilon )
		{
			return false;
		}
	}
	return true;
}

/*
============
AddUnique
============
*/
static void AddUnique( std::vector<Vec3>& points, const Vec3& point, float epsilon )
{
	for( const Vec3& existing : points )
	{
		if( Near( existing, point, epsilon ) )
		{
			return;
		}
	}
	points.push_back( point );
}

/*
============
FaceVertices
============
*/
static std::vector<Vec3> FaceVertices( const std::vector<BrushPlane>& planes, size_t facePlane, float epsilon )
{
	std::vector<Vec3> vertices;

	for( size_t second = 0; second < planes.size(); ++second )
	{
		if( second == facePlane )
		{
			continue;
		}

		for( size_t third = second + 1; third < planes.size(); ++third )
		{
			if( third == facePlane )
			{
				continue;
			}
			Vec3 point;
			if( TripleIntersection( planes[facePlane], planes[second],
									planes[third], epsilon * 0.01f, point ) &&
					Inside( planes, point, epsilon ) )
			{
				AddUnique( vertices, point, epsilon );
			}
		}
	}

	if( vertices.size() < 3 )
	{
		return {};
	}

	Vec3 center;
	for( const Vec3& vertex : vertices )
	{
		center = Add( center, vertex );
	}

	center = Scale( center, 1.0f / vertices.size() );
	const Vec3 normal = planes[facePlane].normal;
	const Vec3 reference = idMath::Fabs( normal.z ) < 0.9f ?
						   Vec3{0.0f, 0.0f, 1.0f} :
						   Vec3{1.0f, 0.0f, 0.0f};
	const Vec3 firstAxis = Normalize( Cross( reference, normal ) );
	const Vec3 secondAxis = Cross( normal, firstAxis );
	std::sort( vertices.begin(), vertices.end(),
			   [&]( const Vec3 & left, const Vec3 & right )
	{
		const Vec3 leftDelta = Subtract( left, center );
		const Vec3 rightDelta = Subtract( right, center );
		const float leftAngle = std::atan2( Dot( leftDelta, secondAxis ),
											Dot( leftDelta, firstAxis ) );
		const float rightAngle = std::atan2( Dot( rightDelta, secondAxis ),
											 Dot( rightDelta, firstAxis ) );
		return leftAngle < rightAngle;
	} );

	return vertices;
}

/*
============
VertexCell
============
*/
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

/*
============
VertexCellHash
============
*/
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

/*
============
VertexWelder
============
*/
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

	uint32 Weld( const Vec3& point )
	{
		const VertexCell center = Cell( point );
		for( long long x = center.x - 1; x <= center.x + 1; ++x )
		{
			for( long long y = center.y - 1; y <= center.y + 1; ++y )
			{
				for( long long z = center.z - 1; z <= center.z + 1; ++z )
				{
					const auto range = buckets.equal_range( VertexCell{x, y, z} );
					for( auto found = range.first; found != range.second; ++found )
					{
						if( Near( point, vertices[found->second], epsilon ) )
						{
							return found->second;
						}
					}
				}
			}
		}
		const uint32 index = static_cast<uint32>( vertices.size() );
		vertices.push_back( point );
		buckets.emplace( center, index );
		return index;
	}

private:
	VertexCell Cell( const Vec3& point ) const
	{
		return {static_cast<long long>( std::floor( point.x / cellSize ) ),
				static_cast<long long>( std::floor( point.y / cellSize ) ),
				static_cast<long long>( std::floor( point.z / cellSize ) )};
	}

	std::vector<Vec3>& vertices;
	float epsilon;
	float cellSize;
	std::unordered_multimap<VertexCell, uint32, VertexCellHash> buckets;
};

/*
============
RedundantPolygonPoint
============
*/
static bool RedundantPolygonPoint( const Vec3& previous, const Vec3& point, const Vec3& next, float epsilon )
{
	const Vec3 first = Subtract( point, previous );
	const Vec3 second = Subtract( next, point );
	return Length( first ) <= epsilon || Length( second ) <= epsilon ||
		   Length( Cross( first, second ) ) <= epsilon *
		   ( Length( first ) + Length( second ) + 1.0f );
}

/*
============
CleanPolygon
============
*/
static void CleanPolygon( std::vector<Vec3>& polygon, float epsilon )
{
	size_t write = 0;
	const size_t originalSize = polygon.size();

	for( size_t read = 0; read < originalSize; ++read )
	{
		const Vec3 point = polygon[read];
		if( write != 0 && Near( polygon[write - 1], point, epsilon ) )
		{
			continue;
		}
		while( write >= 2 &&
				RedundantPolygonPoint( polygon[write - 2], polygon[write - 1],
									   point, epsilon ) )
		{
			--write;
		}
		polygon[write++] = point;
	}

	polygon.resize( write );

	if( polygon.size() > 1 && Near( polygon.front(), polygon.back(), epsilon ) )
	{
		polygon.pop_back();
	}

	// The forward pass removes every redundant interior point. Only the cyclic
	// seam can still be redundant, so trim it by advancing an index or popping
	// the back. No middle erase or restart is needed.
	size_t first = 0;

	while( polygon.size() - first >= 3 )
	{
		if( RedundantPolygonPoint( polygon.back(), polygon[first],
								   polygon[first + 1],
								   epsilon ) )
		{
			++first;
		}
		else if( RedundantPolygonPoint( polygon[polygon.size() - 2],
										polygon.back(), polygon[first],
										epsilon ) )
		{
			polygon.pop_back();
		}
		else
		{
			break;
		}
	}

	if( polygon.size() - first < 3 )
	{
		polygon.clear();
		return;
	}

	if( first != 0 )
	{
		const size_t remaining = polygon.size() - first;
		for( size_t i = 0; i < remaining; ++i )
		{
			polygon[i] = polygon[first + i];
		}
		polygon.resize( remaining );
	}
}

/*
============
PolygonSpan
============
*/
struct PolygonSpan
{
	size_t first = 0;
	size_t count = 0;
	Bounds bounds{};
};

const size_t kMaximumFragmentsPerFace = 32768;
const size_t kMaximumFragmentPointsPerFace = 1024 * 1024;
const size_t kMaximumArenaPointsPerFace = 4 * 1024 * 1024;

// Generated vertices live in an immutable per-face arena. Polygon lists only
// move spans into that arena, so a fragment which is wholly inside or outside a
// clipping plane is retained without copying any Vec3 data.
struct PolygonArena
{
	std::vector<Vec3> points;

	void Clear()
	{
		points.clear();
	}

	bool Add( const std::vector<Vec3>& polygon, PolygonSpan& span )
	{
		if( polygon.size() < 3 )
		{
			span = PolygonSpan{};
			return true;
		}
		if( polygon.size() > kMaximumArenaPointsPerFace -
				Min( points.size(),
					 kMaximumArenaPointsPerFace ) )
		{
			return false;
		}
		span.first = points.size();
		span.count = polygon.size();
		span.bounds = PolygonBounds( polygon );
		points.resize( points.size() + polygon.size() );
		for( size_t i = 0; i < polygon.size(); ++i )
		{
			points[span.first + i] = polygon[i];
		}
		return true;
	}
};

/*
============
PolygonList
============
*/
struct PolygonList
{
	std::vector<PolygonSpan> spans;
	size_t pointCount = 0;

	void Clear()
	{
		spans.clear();
		pointCount = 0;
	}

	bool Add( const PolygonSpan& span )
	{
		if( span.count < 3 )
		{
			return true;
		}
		if( spans.size() >= kMaximumFragmentsPerFace ||
				span.count > kMaximumFragmentPointsPerFace -
				Min( pointCount,
					 kMaximumFragmentPointsPerFace ) )
		{
			return false;
		}
		spans.push_back( span );
		pointCount += span.count;
		return true;
	}

	void Swap( PolygonList& other )
	{
		spans.swap( other.spans );
		std::swap( pointCount, other.pointCount );
	}
};

/*
============
SplitPolygon
============
*/
static bool SplitPolygon( PolygonArena& arena, const PolygonSpan& span, const BrushPlane& plane, float epsilon, PolygonSpan& insideSpan, bool& hasInside, PolygonSpan& outsideSpan,
						  bool& hasOutside, std::vector<Vec3>& inside, std::vector<Vec3>& outside )
{
	hasInside = false;
	hasOutside = false;
	bool anyInside = false;
	bool anyOutside = false;

	for( size_t i = 0; i < span.count; ++i )
	{
		const Vec3& point = arena.points[span.first + i];
		if( Dot( plane.normal, point ) - plane.distance <= epsilon )
		{
			anyInside = true;
		}
		else
		{
			anyOutside = true;
		}
	}

	if( !anyOutside )
	{
		insideSpan = span;
		hasInside = true;
		return true;
	}

	if( !anyInside )
	{
		outsideSpan = span;
		hasOutside = true;
		return true;
	}

	inside.clear();
	outside.clear();
	inside.reserve( span.count + 2 );
	outside.reserve( span.count + 2 );

	for( size_t i = 0; i < span.count; ++i )
	{
		const Vec3& first = arena.points[span.first + i];
		const Vec3& second = arena.points[
								 span.first + ( ( i + 1 ) % span.count )];
		const float firstDistance = Dot( plane.normal, first ) - plane.distance;
		const float secondDistance = Dot( plane.normal, second ) - plane.distance;
		const bool firstInside = firstDistance <= epsilon;
		const bool secondInside = secondDistance <= epsilon;
		( firstInside ? inside : outside ).push_back( first );
		if( firstInside != secondInside )
		{
			const float fraction = firstDistance / ( firstDistance - secondDistance );
			const Vec3 intersection = Add( first, Scale( Subtract( second, first ), fraction ) );
			inside.push_back( intersection );
			outside.push_back( intersection );
		}
	}

	CleanPolygon( inside, epsilon );
	CleanPolygon( outside, epsilon );

	if( inside.size() >= 3 )
	{
		if( !arena.Add( inside, insideSpan ) )
		{
			return false;
		}
		hasInside = true;
	}

	if( outside.size() >= 3 )
	{
		if( !arena.Add( outside, outsideSpan ) )
		{
			return false;
		}
		hasOutside = true;
	}

	return true;
}

/*
============
SubtractBrush
============
*/
static bool SubtractBrush( PolygonArena& arena,	PolygonList& fragments, const std::vector<BrushPlane>& brush, const Bounds& brushBounds, float epsilon,
						   PolygonList& candidates, PolygonList& nextCandidates, PolygonList& result, std::vector<Vec3>& inside, std::vector<Vec3>& outside )
{
	result.Clear();
	candidates.Clear();

	for( const PolygonSpan& fragment : fragments.spans )
	{
		PolygonList& destination =
			BoundsIntersect( fragment.bounds, brushBounds, epsilon ) ?
			candidates : result;
		if( !destination.Add( fragment ) )
		{
			return false;
		}
	}

	fragments.Clear();
	nextCandidates.Clear();

	if( candidates.spans.empty() )
	{
		fragments.Swap( result );
		return true;
	}

	for( const BrushPlane& plane : brush )
	{
		nextCandidates.Clear();
		for( const PolygonSpan& candidate : candidates.spans )
		{
			PolygonSpan insideSpan;
			PolygonSpan outsideSpan;
			bool hasInside = false;
			bool hasOutside = false;
			if( !SplitPolygon( arena, candidate, plane, epsilon,
							   insideSpan, hasInside, outsideSpan, hasOutside,
							   inside, outside ) )
			{
				return false;
			}
			if( hasOutside && !result.Add( outsideSpan ) )
			{
				return false;
			}
			if( hasInside && !nextCandidates.Add( insideSpan ) )
			{
				return false;
			}
		}

		candidates.Clear();
		candidates.Swap( nextCandidates );

		if( candidates.spans.empty() )
		{
			break;
		}
	}

	fragments.Clear();
	fragments.Swap( result );

	return true;
}

/*
============
FloorFace
============
*/
struct FloorFace
{
	size_t brush = 0;
	BrushPlane plane{};
	uint32 contents = 0;
	std::vector<Vec3> vertices;
	Bounds bounds{};
};

/*
============
HasEquivalentBoundary
============
*/
static bool HasEquivalentBoundary( const std::vector<BrushPlane>& brush, const BrushPlane& face, float epsilon )
{
	for( const BrushPlane& plane : brush )
	{
		if( Dot( plane.normal, face.normal ) >= 1.0f - epsilon &&
				idMath::Fabs( plane.distance - face.distance ) <= epsilon )
		{
			return true;
		}
	}
	return false;
}

/*
============
PolygonBrushRelation
============
*/
enum class PolygonBrushRelation
{
	disjoint,
	contained,
	intersecting
};

/*
============
ClassifyPolygonAgainstBrush
============
*/
static PolygonBrushRelation ClassifyPolygonAgainstBrush( const std::vector<Vec3>& polygon,	const std::vector<BrushPlane>& brush, float epsilon )
{
	bool contained = true;
	for( const BrushPlane& plane : brush )
	{
		bool anyInside = false;
		for( const Vec3& point : polygon )
		{
			if( Dot( plane.normal, point ) <= plane.distance + epsilon )
			{
				anyInside = true;
			}
			else
			{
				contained = false;
			}
		}

		// A convex polygon whose vertices all lie outside one brush half-space
		// cannot intersect that brush. This rejects the overwhelming majority
		// of AABB broad-phase matches without creating a single fragment.
		if( !anyInside )
		{
			return PolygonBrushRelation::disjoint;
		}
	}
	return contained ? PolygonBrushRelation::contained : PolygonBrushRelation::intersecting;
}

/*
============
CSGCandidate
============
*/
struct CSGCandidate
{
	uint32 brush = 0;
};

const float kSpatialCellSize = 256.0f;
const int kMaximumCellsPerBrush = 4096;

/*
============
SpatialCell
============
*/
int SpatialCell( float coordinate )
{
	return static_cast<int>( std::floor( coordinate / kSpatialCellSize ) );
}

/*
============
SpatialKey
============
*/
uint64 SpatialKey( int x, int y )
{
	return ( static_cast<uint64>( static_cast<uint32>( x ) ) << 32 ) | static_cast<uint32>( y );
}

/*
============
BrushSpatialIndex
============
*/
struct BrushSpatialIndex
{
	std::unordered_map<uint64, std::vector<uint32>> cells;
	std::vector<uint32> globalBrushes;
	std::vector<uint32> stamps;
	uint32 stamp = 0;

	void Build( const std::vector<Bounds>& bounds )
	{
		stamps.assign( bounds.size(), 0 );
		cells.reserve( bounds.size() * 2 );

		for( size_t brush = 0; brush < bounds.size(); ++brush )
		{
			const int minX = SpatialCell( bounds[brush].mins.x );
			const int maxX = SpatialCell( bounds[brush].maxs.x );
			const int minY = SpatialCell( bounds[brush].mins.y );
			const int maxY = SpatialCell( bounds[brush].maxs.y );
			const long long width = static_cast<long long>( maxX ) - minX + 1;
			const long long height = static_cast<long long>( maxY ) - minY + 1;

			if( width <= 0 || height <= 0 || width * height > kMaximumCellsPerBrush )
			{
				globalBrushes.push_back( static_cast<uint32>( brush ) );
				continue;
			}

			for( int x = minX; x <= maxX; ++x )
			{
				for( int y = minY; y <= maxY; ++y )
				{
					cells[SpatialKey( x, y )].push_back( static_cast<uint32>( brush ) );
				}
			}
		}
	}

	void Query( const Bounds& bounds, std::vector<uint32>& brushes )
	{
		brushes.clear();

		if( ++stamp == 0 )
		{
			std::fill( stamps.begin(), stamps.end(), 0 );
			stamp = 1;
		}

		for( uint32 brush : globalBrushes )
		{
			stamps[brush] = stamp;
			brushes.push_back( brush );
		}

		const int minX = SpatialCell( bounds.mins.x );
		const int maxX = SpatialCell( bounds.maxs.x );
		const int minY = SpatialCell( bounds.mins.y );
		const int maxY = SpatialCell( bounds.maxs.y );
		const long long width = static_cast<long long>( maxX ) - minX + 1;
		const long long height = static_cast<long long>( maxY ) - minY + 1;

		if( width <= 0 || height <= 0 || width * height > kMaximumCellsPerBrush )
		{
			brushes.resize( stamps.size() );
			for( size_t i = 0; i < stamps.size(); ++i )
			{
				brushes[i] = static_cast<uint32>( i );
			}
			return;
		}

		for( int x = minX; x <= maxX; ++x )
		{
			for( int y = minY; y <= maxY; ++y )
			{
				const auto found = cells.find( SpatialKey( x, y ) );
				if( found == cells.end() )
				{
					continue;
				}
				for( uint32 brush : found->second )
				{
					if( stamps[brush] != stamp )
					{
						stamps[brush] = stamp;
						brushes.push_back( brush );
					}
				}
			}
		}

		std::sort( brushes.begin(), brushes.end() );
	}
};

/*
============
BuildBrushFloorGeometry
============
*/
BrushSurfaceResult BuildBrushFloorGeometry(	const std::vector<ConvexBrush>& brushes, float maxFloorSlopeDegrees, float epsilon, SourceGeometry& output, ProgressCallback progress, void* progressUserData )
{
	output = SourceGeometry{};
	BrushSurfaceResult result;
	const float safeEpsilon = Max( 1.0e-5f, epsilon );
	const float slope = Max( 0.0f, Min( 89.9f, maxFloorSlopeDegrees ) );
	const float minimumFloorZ = idMath::Cos( slope * idMath::PI / 180.0f );

	std::vector<std::vector<BrushPlane>> normalizedBrushes( brushes.size() );
	std::vector<Bounds> brushBounds( brushes.size(), EmptyBounds() );
	std::vector<FloorFace> floorFaces;

	ReportProgress( progress, progressUserData, "brush face reconstruction", 0, brushes.size() );

	for( size_t brushIndex = 0; brushIndex < brushes.size(); ++brushIndex )
	{
		ReportProgress( progress, progressUserData, "brush face reconstruction", brushIndex, brushes.size() );
		const ConvexBrush& brush = brushes[brushIndex];
		std::vector<BrushPlane>& planes = normalizedBrushes[brushIndex];
		size_t invalidPlane = 0;

		if( brush.planes.size() < 4 || !NormalizePlanes( brush, planes, invalidPlane ) )
		{
			result.error = brush.planes.size() < 4 ?
						   BrushSurfaceError::unboundedOrDegenerateBrush :
						   BrushSurfaceError::invalidPlane;
			result.sourceBrush = brushIndex;
			result.sourcePlane = invalidPlane;
			output = SourceGeometry{};
			return result;
		}

		size_t brushFaces = 0;
		for( size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex )
		{
			std::vector<Vec3> vertices = FaceVertices( planes, planeIndex, safeEpsilon );
			if( vertices.size() >= 3 )
			{
				++brushFaces;

				for( const Vec3& vertex : vertices )
				{
					AddPoint( brushBounds[brushIndex], vertex );
				}

				if( planes[planeIndex].normal.z >= minimumFloorZ )
				{
					FloorFace face;
					face.brush = brushIndex;
					face.plane = planes[planeIndex];
					face.contents = brush.contents;
					face.bounds = PolygonBounds( vertices );
					face.vertices = std::move( vertices );
					floorFaces.push_back( std::move( face ) );
					++result.floorFaces;
				}
			}
		}

		if( brushFaces < 4 )
		{
			result.error = BrushSurfaceError::unboundedOrDegenerateBrush;
			result.sourceBrush = brushIndex;
			output = SourceGeometry{};
			return result;
		}
		result.boundaryFaces += brushFaces;
	}

	ReportProgress( progress, progressUserData, "brush face reconstruction", brushes.size(), brushes.size() );

	BrushSpatialIndex spatialIndex;
	spatialIndex.Build( brushBounds );
	std::vector<uint32> candidateBrushes;
	std::vector<CSGCandidate> csgCandidates;
	PolygonArena polygonArena;
	PolygonList fragments;
	PolygonList workCandidates;
	PolygonList nextCandidates;
	PolygonList subtractionResult;
	std::vector<Vec3> inside;
	std::vector<Vec3> outside;
	std::vector<uint32> indices;

	VertexWelder vertexWelder( output.vertices, safeEpsilon, floorFaces.size() * 4 );
	ReportProgress( progress, progressUserData, "floor CSG", 0, floorFaces.size() );

	for( size_t faceIndex = 0; faceIndex < floorFaces.size(); ++faceIndex )
	{
		ReportProgress( progress, progressUserData, "floor CSG", faceIndex, floorFaces.size() );
		const FloorFace& face = floorFaces[faceIndex];
		polygonArena.Clear();
		fragments.Clear();
		PolygonSpan faceSpan;

		if( !polygonArena.Add( face.vertices, faceSpan ) || !fragments.Add( faceSpan ) )
		{
			result.error = BrushSurfaceError::fragmentLimit;
			result.sourceBrush = face.brush;
			output = SourceGeometry{};
			return result;
		}

		spatialIndex.Query( face.bounds, candidateBrushes );
		csgCandidates.clear();
		bool covered = false;

		for( uint32 other : candidateBrushes )
		{
			if( other == face.brush )
			{
				continue;
			}

			++result.candidatePairs;

			if( !BoundsIntersect( face.bounds, brushBounds[other], safeEpsilon ) )
			{
				++result.rejectedCandidatePairs;
				continue;
			}

			// Give coincident union boundaries deterministic ownership. The
			// earliest brush retains the overlap; later brushes contribute only
			// the portion not already covered by that boundary.
			if( other > face.brush && HasEquivalentBoundary( normalizedBrushes[other], face.plane, safeEpsilon ) )
			{
				++result.rejectedCandidatePairs;
				continue;
			}

			const PolygonBrushRelation relation = ClassifyPolygonAgainstBrush( face.vertices, normalizedBrushes[other], safeEpsilon );
			if( relation == PolygonBrushRelation::disjoint )
			{
				++result.rejectedCandidatePairs;
				continue;
			}

			if( relation == PolygonBrushRelation::contained )
			{
				// SubtractBrush would clip this polygon against every plane only
				// to discard it. A direct clear is exact and avoids pathological
				// work for nested or overlapping map brushes.
				fragments.Clear();
				covered = true;
				break;
			}

			CSGCandidate candidate;
			candidate.brush = other;
			csgCandidates.push_back( candidate );
		}

		if( !covered )
		{
			// BrushSpatialIndex::Query already returns ascending brush IDs.
			// Preserve that deterministic order and avoid a floating-point
			// comparator under the engine's Win32 /fp:fast configuration.
			const bool denseFace = csgCandidates.size() >= 256;
			if( denseFace )
			{
				ReportProgress( progress, progressUserData, "dense floor CSG", 0, csgCandidates.size() );
			}
			for( size_t candidateIndex = 0; candidateIndex < csgCandidates.size(); ++candidateIndex )
			{
				if( denseFace )
				{
					ReportProgress( progress, progressUserData, "dense floor CSG", candidateIndex, csgCandidates.size() );
				}
				const uint32 other = csgCandidates[candidateIndex].brush;
				if( !SubtractBrush( polygonArena, fragments, normalizedBrushes[other], brushBounds[other], safeEpsilon, workCandidates, nextCandidates, subtractionResult, inside, outside ) )
				{
					result.error = BrushSurfaceError::fragmentLimit;
					result.sourceBrush = face.brush;
					result.sourcePlane = other;
					result.peakFragments = Max(
											   result.peakFragments,
											   Max( fragments.spans.size(),
													subtractionResult.spans.size() ) );
					result.peakFragmentPoints = Max(
													result.peakFragmentPoints,
													Max( fragments.pointCount,
														 subtractionResult.pointCount ) );
					output = SourceGeometry{};
					return result;
				}

				result.peakFragments = Max( result.peakFragments, fragments.spans.size() );
				result.peakFragmentPoints = Max( result.peakFragmentPoints, fragments.pointCount );

				if( fragments.spans.empty() )
				{
					break;
				}
			}

			if( denseFace )
			{
				ReportProgress( progress, progressUserData, "dense floor CSG", csgCandidates.size(), csgCandidates.size() );
			}
		}

		for( const PolygonSpan& fragment : fragments.spans )
		{
			indices.clear();
			indices.reserve( fragment.count );

			for( size_t point = 0; point < fragment.count; ++point )
			{
				indices.push_back( vertexWelder.Weld( polygonArena.points[fragment.first + point] ) );
			}

			for( size_t corner = 1; corner + 1 < fragment.count; ++corner )
			{
				if( indices[0] == indices[corner] || indices[0] == indices[corner + 1] || indices[corner] == indices[corner + 1] )
				{
					++result.discardedDegenerateTriangles;
					continue;
				}

				uint32 second = indices[corner];
				uint32 third = indices[corner + 1];
				const Vec3 triangleNormal = Cross(
												Subtract( output.vertices[second], output.vertices[indices[0]] ),
												Subtract( output.vertices[third], output.vertices[indices[0]] ) );
				if( !Finite( triangleNormal ) || Length( triangleNormal ) <= safeEpsilon * safeEpsilon )
				{
					++result.discardedDegenerateTriangles;
					continue;
				}

				if( Dot( triangleNormal, face.plane.normal ) < 0.0f )
				{
					std::swap( second, third );
				}

				SourceTriangle triangle;
				triangle.vertices[0] = indices[0];
				triangle.vertices[1] = second;
				triangle.vertices[2] = third;
				triangle.flags = face.contents;
				output.triangles.push_back( triangle );
				++result.floorTriangles;
			}
		}
	}

	ReportProgress( progress, progressUserData, "floor CSG", floorFaces.size(), floorFaces.size() );

	return result;
}

/*
============
BrushSurfaceErrorName
============
*/
const char* BrushSurfaceErrorName( BrushSurfaceError error )
{
	switch( error )
	{
		case BrushSurfaceError::none:
			return "none";
		case BrushSurfaceError::invalidPlane:
			return "invalid brush plane";
		case BrushSurfaceError::unboundedOrDegenerateBrush:
			return "unbounded or degenerate brush";
		case BrushSurfaceError::fragmentLimit:
			return "per-face CSG fragment safety limit exceeded";
	}
	return "unknown";
}