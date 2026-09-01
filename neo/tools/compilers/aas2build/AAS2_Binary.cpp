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

#include "AAS2_Binary.h"
#include "AAS2_Compiler.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>

const uint8 kMagic[4] = {'2', 'S', 'A', 'A'};
const uint8 kVersionMajor = 3;
const uint8 kVersionMinor = 18;
const uint8 kVersionPatch = 'a';
const uint32 kMaximumCount = 16000000u;

/*
============
Writer
============
*/
struct Writer
{
	std::vector<uint8>& bytes;
	void U8( uint8 v )
	{
		bytes.push_back( v );
	}
	void U16( uint16 v )
	{
		U8( static_cast<uint8>( v >> 8 ) );
		U8( static_cast<uint8>( v ) );
	}
	void I16( int16 v )
	{
		U16( static_cast<uint16>( v ) );
	}
	void U32( uint32 v )
	{
		U8( static_cast<uint8>( v >> 24 ) );
		U8( static_cast<uint8>( v >> 16 ) );
		U8( static_cast<uint8>( v >> 8 ) );
		U8( static_cast<uint8>( v ) );
	}
	void U32LE( uint32 v )
	{
		U8( static_cast<uint8>( v ) );
		U8( static_cast<uint8>( v >> 8 ) );
		U8( static_cast<uint8>( v >> 16 ) );
		U8( static_cast<uint8>( v >> 24 ) );
	}
	void I32( int32 v )
	{
		U32( static_cast<uint32>( v ) );
	}
	void Float( float v )
	{
		uint32 b = 0;
		std::memcpy( &b, &v, sizeof( b ) );
		U32( b );
	}
	void Vec( const Vec3& v )
	{
		Float( v.x );
		Float( v.y );
		Float( v.z );
	}
	void String( const std::string& v )
	{
		U32LE( static_cast<uint32>( v.size() ) );
		bytes.insert( bytes.end(), v.begin(), v.end() );
	}
};

/*
============
Reader
============
*/
struct Reader
{
	const std::vector<uint8>& bytes;
	size_t cursor = 0;
	bool Take( size_t n )
	{
		if( cursor > bytes.size() || n > bytes.size() - cursor )
		{
			return false;
		}
		cursor += n;
		return true;
	}
	bool U8( uint8& v )
	{
		if( cursor >= bytes.size() )
		{
			return false;
		}
		v = bytes[cursor++];
		return true;
	}
	bool U16( uint16& v )
	{
		if( !Take( 2 ) )
		{
			return false;
		}
		v = static_cast<uint16>( bytes[cursor - 2] << 8 ) | bytes[cursor - 1];
		return true;
	}
	bool I16( int16& v )
	{
		uint16 x = 0;
		if( !U16( x ) )
		{
			return false;
		}
		v = static_cast<int16>( x );
		return true;
	}
	bool U32( uint32& v )
	{
		if( !Take( 4 ) )
		{
			return false;
		}
		v = ( static_cast<uint32>( bytes[cursor - 4] ) << 24 ) |
			( static_cast<uint32>( bytes[cursor - 3] ) << 16 ) |
			( static_cast<uint32>( bytes[cursor - 2] ) << 8 ) | bytes[cursor - 1];
		return true;
	}
	bool U32LE( uint32& v )
	{
		if( !Take( 4 ) )
		{
			return false;
		}
		v = bytes[cursor - 4] | ( static_cast<uint32>( bytes[cursor - 3] ) << 8 ) |
			( static_cast<uint32>( bytes[cursor - 2] ) << 16 ) |
			( static_cast<uint32>( bytes[cursor - 1] ) << 24 );
		return true;
	}
	bool I32( int32& v )
	{
		uint32 x = 0;
		if( !U32( x ) )
		{
			return false;
		}
		v = static_cast<int32>( x );
		return true;
	}
	bool Float( float& v )
	{
		uint32 b = 0;
		if( !U32( b ) )
		{
			return false;
		}
		std::memcpy( &v, &b, sizeof( v ) );
		return std::isfinite( v ) != 0;
	}
	bool Vec( Vec3& v )
	{
		return Float( v.x ) && Float( v.y ) && Float( v.z );
	}
	bool String( std::string& v )
	{
		uint32 n = 0;
		if( !U32LE( n ) || n > 1048576u || !Take( n ) )
		{
			return false;
		}
		v.assign( reinterpret_cast<const char*>( bytes.data() + cursor - n ), n );
		return true;
	}
	bool Count( uint32& v )
	{
		return U32( v ) && v <= kMaximumCount;
	}
};

/*
============
Quantize
============
*/
static int16 Quantize( float v )
{
	const float r = std::floor( v + ( v >= 0.0f ? 0.5f : -0.5f ) );
	return static_cast<int16>( Max( -32768.0f, Min( 32767.0f, r ) ) );
}

/*
============
Distance
============
*/
static float Distance( const Vec3& a, const Vec3& b )
{
	const float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
	return std::sqrt( x * x + y * y + z * z );
}

/*
============
ReachTravelTime
============
*/
static uint16 ReachTravelTime( const Reachability& r, const BinaryMetadata& m )
{
	float t = m.groundSpeed > 0.0f ? Distance( r.start, r.end ) * 100.0f / m.groundSpeed : 0.0f;
	if( r.travelFlags & AAS2_TRAVEL_WALK_OFF_LEDGE )
	{
		t += static_cast<float>( m.ttStartWalkOffLedge );
	}
	else if( r.travelFlags & AAS2_TRAVEL_LEDGE_GRAB )
	{
		t += static_cast<float>( m.ttBarrierJump );
	}
	else if( r.travelFlags & AAS2_TRAVEL_WATER_JUMP )
	{
		t += static_cast<float>( m.ttWaterJump );
	}
	else if( r.travelFlags & AAS2_TRAVEL_LADDER )
	{
		t += static_cast<float>( m.ttStartLadderClimb );
	}
	return static_cast<uint16>( Max( 1.0f, Min( 65535.0f, t ) ) );
}

/*
============
BuildOrientedEdges
============
*/
static bool BuildOrientedEdges( const File& file, const Area& area, std::vector<int32>& out )
{
	out.clear();
	if( area.edges.empty() )
	{
		return false;
	}
	for( int direction = 0; direction < 2; ++direction )
	{
		std::vector<bool> used( area.edges.size(), false );
		out.clear();
		const uint32 firstIndex = area.edges[0];
		const Edge& first = file.edges[firstIndex];
		const uint32 firstVertex = first.vertices[direction];
		uint32 current = first.vertices[direction ^ 1];
		out.push_back( direction == 0 ? static_cast<int32>( firstIndex + 1 ) : -static_cast<int32>( firstIndex + 1 ) );
		used[0] = true;
		while( out.size() < area.edges.size() )
		{
			bool found = false;
			for( size_t i = 1; i < area.edges.size(); ++i )
			{
				if( used[i] )
				{
					continue;
				}
				const Edge& edge = file.edges[area.edges[i]];
				if( edge.vertices[0] == current )
				{
					out.push_back( static_cast<int32>( area.edges[i] + 1 ) );
					current = edge.vertices[1];
					used[i] = true;
					found = true;
					break;
				}
				if( edge.vertices[1] == current )
				{
					out.push_back( -static_cast<int32>( area.edges[i] + 1 ) );
					current = edge.vertices[0];
					used[i] = true;
					found = true;
					break;
				}
			}
			if( !found )
			{
				break;
			}
		}
		if( out.size() == area.edges.size() && current == firstVertex )
		{
			return true;
		}
	}
	return false;
}

/*
============
WriteSettings
============
*/
static void WriteSettings( Writer& w, const BinaryMetadata& m )
{
	w.I32( m.type );
	w.String( m.fileExtensionAAS );
	w.String( m.groupName );
	w.String( m.explicitGroupName );
	w.Vec( m.agent.mins );
	w.Vec( m.agent.maxs );
	w.I32( m.primitiveModeBrush );
	w.I32( m.primitiveModePatch );
	w.I32( m.primitiveModeModel );
	w.Vec( m.gravityDir );
	const float values[] = {m.gravityValue, m.maxStepHeight, m.maxBarrierHeight, m.maxWaterJumpHeight,
							static_cast<float>( m.maxFallHeight ), m.minFloorCos, m.minHighCeiling, m.groundSpeed, m.waterSpeed,
							m.ladderSpeed, m.wallCornerEdgeRadius, m.ledgeCornerEdgeRadius, m.obstaclePVSRadius,
							m.minCrouchingCoverHeight, m.minStandingCoverHeight, m.crouchingFireHeight, m.standingFireHeight,
							m.minWallWidth, m.maxWallWidth, m.minDoorWidth, m.maxDoorWidth, m.coverCornerDistance,
							m.coverWallDistance, m.chokePointWidth
						   };
	for( float value : values )
	{
		w.Float( value );
	}
	w.I32( m.ttBarrierJump );
	w.I32( m.ttWaterJump );
	w.I32( m.ttStartWalkOffLedge );
	w.I32( m.ttStartLadderClimb );
}

/*
============
ReadSettings
============
*/
static bool ReadSettings( Reader& r, BinaryMetadata& m )
{
	if( !r.I32( m.type ) || !r.String( m.fileExtensionAAS ) || !r.String( m.groupName ) ||
			!r.String( m.explicitGroupName ) || !r.Vec( m.agent.mins ) || !r.Vec( m.agent.maxs ) ||
			!r.I32( m.primitiveModeBrush ) || !r.I32( m.primitiveModePatch ) ||
			!r.I32( m.primitiveModeModel ) || !r.Vec( m.gravityDir ) )
	{
		return false;
	}
	float maxFall = 0.0f;
	float* values[] = {&m.gravityValue, &m.maxStepHeight, &m.maxBarrierHeight, &m.maxWaterJumpHeight,
					   &maxFall, &m.minFloorCos, &m.minHighCeiling, &m.groundSpeed, &m.waterSpeed, &m.ladderSpeed,
					   &m.wallCornerEdgeRadius, &m.ledgeCornerEdgeRadius, &m.obstaclePVSRadius,
					   &m.minCrouchingCoverHeight, &m.minStandingCoverHeight, &m.crouchingFireHeight,
					   &m.standingFireHeight, &m.minWallWidth, &m.maxWallWidth, &m.minDoorWidth, &m.maxDoorWidth,
					   &m.coverCornerDistance, &m.coverWallDistance, &m.chokePointWidth
					  };
	for( float* value : values ) if( !r.Float( *value ) )
		{
			return false;
		}
	m.maxFallHeight = static_cast<int32>( maxFall );
	return r.I32( m.ttBarrierJump ) && r.I32( m.ttWaterJump ) && r.I32( m.ttStartWalkOffLedge ) && r.I32( m.ttStartLadderClimb );
}

/*
============
SkipArray
============
*/
static bool SkipArray( Reader& r, size_t elementSize )
{
	uint32 count = 0;
	return r.Count( count ) && ( count == 0 || ( elementSize <= std::numeric_limits<size_t>::max() / count && r.Take( elementSize * count ) ) );
}

/*
============
SerializeAAS2
============
*/
BinaryError SerializeAAS2( const File& file, const BinaryMetadata& metadata, std::vector<uint8>& output )
{
	output.clear();
	if( !ValidateFile( file ) || metadata.fileExtensionAAS.size() > 1048576u ||
			metadata.groupName.size() > 1048576u ||
			metadata.explicitGroupName.size() > 1048576u )
	{
		return BinaryError::invalidFile;
	}

	if( file.areas.size() >= 32767u )
	{
		return BinaryError::tooManyAreas;
	}

	if( file.reachabilities.size() >= 32767u )
	{
		return BinaryError::tooManyReachabilities;
	}

	std::vector<std::vector<int32>> oriented( file.areas.size() );
	std::vector<int32> edgeIndexes;
	std::vector<uint32> firstEdges( file.areas.size() );

	for( size_t i = 0; i < file.areas.size(); ++i )
	{
		if( !BuildOrientedEdges( file, file.areas[i], oriented[i] ) )
		{
			return BinaryError::unorientedArea;
		}
		firstEdges[i] = static_cast<uint32>( edgeIndexes.size() );
		edgeIndexes.insert( edgeIndexes.end(), oriented[i].begin(), oriented[i].end() );
	}

	std::vector<int16> next( file.reachabilities.size(), -1 ), reverseNext( file.reachabilities.size(), -1 );
	std::vector<int16> firstReverse( file.areas.size(), -1 ), lastReverse( file.areas.size(), -1 );

	for( const Area& area : file.areas )
	{
		for( size_t i = 1; i < area.reachabilities.size(); ++i )
		{
			next[area.reachabilities[i - 1]] = static_cast<int16>( area.reachabilities[i] );
		}
	}

	for( size_t i = 0; i < file.reachabilities.size(); ++i )
	{
		const size_t to = file.reachabilities[i].toArea;
		if( firstReverse[to] < 0 )
		{
			firstReverse[to] = static_cast<int16>( i );
		}
		if( lastReverse[to] >= 0 )
		{
			reverseNext[lastReverse[to]] = static_cast<int16>( i );
		}
		lastReverse[to] = static_cast<int16>( i );
	}

	Writer w{output};
	output.insert( output.end(), kMagic, kMagic + 4 );
	w.U8( kVersionMajor );
	w.U8( kVersionMinor );
	w.U8( kVersionPatch );
	w.U32( metadata.sourceTimestamp );
	w.U32( metadata.mapCRC );
	for( int i = 0; i < 4; ++i )
	{
		w.I32( 0 );
	}
	WriteSettings( w, metadata );

	w.U32( static_cast<uint32>( file.nodes.size() ) );
	for( const Node& node : file.nodes )
	{
		w.Vec( node.normal );
		w.Float( -node.distance );
	}

	w.U32( static_cast<uint32>( file.vertices.size() ) );
	for( const Vec3& vertex : file.vertices )
	{
		w.Vec( vertex );
	}

	w.U32( static_cast<uint32>( file.edges.size() + 1 ) );
	w.I32( 0 );
	w.I32( 0 );
	w.I32( 0 );

	for( const Edge& edge : file.edges )
	{
		w.I32( static_cast<int32>( edge.vertices[0] ) );
		w.I32( static_cast<int32>( edge.vertices[1] ) );
		w.I32( static_cast<int32>( edge.flags ) );
	}

	w.U32( static_cast<uint32>( edgeIndexes.size() ) );
	for( int32 index : edgeIndexes )
	{
		w.I32( index );
	}

	w.U32( static_cast<uint32>( file.reachabilities.size() ) );
	for( size_t i = 0; i < file.reachabilities.size(); ++i )
	{
		const Reachability& reach = file.reachabilities[i];
		w.U32( reach.travelFlags );
		w.U16( ReachTravelTime( reach, metadata ) );
		w.U16( static_cast<uint16>( reach.fromArea + 1 ) );
		w.U16( static_cast<uint16>( reach.toArea + 1 ) );
		w.U16( 0 );
		w.I16( Quantize( reach.start.x ) );
		w.I16( Quantize( reach.start.y ) );
		w.I16( Quantize( reach.start.z ) );
		w.I16( Quantize( reach.end.x ) );
		w.I16( Quantize( reach.end.y ) );
		w.I16( Quantize( reach.end.z ) );
		w.I32( 0 );
		w.I16( next[i] );
		w.I16( reverseNext[i] );
	}

	w.U32( static_cast<uint32>( file.areas.size() + 1 ) );
	for( int i = 0; i < 10; ++i )
	{
		w.I32( 0 );
	}

	for( size_t i = 0; i < file.areas.size(); ++i )
	{
		const Area& area = file.areas[i];
		uint16 flags = area.flags;
		for( uint32 reach : area.reachabilities )
			if( file.reachabilities[reach].travelFlags & AAS2_TRAVEL_WALK_OFF_LEDGE )
			{
				flags = static_cast<uint16>( flags | AAS2_AREA_LEDGE );
			}
		w.U32( area.travelFlags );
		w.U16( flags );
		w.I16( static_cast<int16>( area.edges.size() ) );

		// clusterAreaNum indexes the routing cache and is zero-based even
		// though the file's global area array has a dummy element at zero.
		w.I32( static_cast<int32>( firstEdges[i] ) );
		w.I16( 1 );
		w.I16( static_cast<int16>( i ) );
		w.I32( 0 );
		w.I16( area.reachabilities.empty() ? -1 : static_cast<int16>( area.reachabilities.front() ) );
		w.I16( firstReverse[i] );
		for( int field = 0; field < 8; ++field )
		{
			w.U16( 0 );
		}
	}

	w.U32( static_cast<uint32>( file.nodes.size() + 1 ) );
	w.U32( 0 );
	w.U32( 0 );
	w.I32( 0 );
	w.I32( 0 );

	for( size_t i = 0; i < file.nodes.size(); ++i )
	{
		w.U32( static_cast<uint32>( i ) );
		w.U32( 0 );
		for( int side = 0; side < 2; ++side )
		{
			w.I32( file.nodes[i].children[side] >= 0 ? file.nodes[i].children[side] + 1 : file.nodes[i].children[side] );
		}
	}

	w.U32( 1 );
	for( int i = 0; i < 6; ++i )
	{
		w.U16( 0 );    // dummy portal
	}
	w.U32( 0 ); // portal index
	w.U32( 2 );

	for( int i = 0; i < 4; ++i )
	{
		w.I32( 0 );    // dummy cluster
	}

	w.I32( static_cast<int32>( file.areas.size() ) );
	w.I32( static_cast<int32>( file.areas.size() ) );
	w.I32( 0 );
	w.I32( 0 );
	w.U32( 0 ); // obstacle PVS
	w.U32( 0 ); // reachability names
	w.U32( 0 ); // animation names
	w.U32( 0 ); // dependency names
	w.U32( 0 ); // interaction names
	w.U32( 0 ); // traversal names
	w.U32( 0 ); // cover
	w.U32( 0 ); // area cover indexes
	w.U32( 0 ); // touching cover indexes
	w.U32( 0 ); // traversals
	w.U32( 0 ); // hint nodes
	w.U32( 1 ); // tree
	w.Vec( file.trees[0].floorNormal );
	w.I32( file.trees[0].rootNode >= 0 ? file.trees[0].rootNode + 1 : file.trees[0].rootNode );
	w.I32( 1 );
	w.I32( static_cast<int32>( file.areas.size() + 1 ) );
	w.U32( static_cast<uint32>( file.areas.size() + 1 ) );

	for( int i = 0; i < 6; ++i )
	{
		w.I16( 0 );
	}

	for( const Area& area : file.areas )
	{
		w.I16( Quantize( area.bounds.mins.x ) );
		w.I16( Quantize( area.bounds.mins.y ) );
		w.I16( Quantize( area.bounds.mins.z ) );
		w.I16( Quantize( area.bounds.maxs.x ) );
		w.I16( Quantize( area.bounds.maxs.y ) );
		w.I16( Quantize( area.bounds.maxs.z ) );
	}
	return BinaryError::none;
}

/*
============
DeserializeAAS2
============
*/
BinaryError DeserializeAAS2( const std::vector<uint8>& input, File& file, BinaryMetadata& metadata )
{
	file = File{};
	metadata = BinaryMetadata{};

	if( input.size() < 31 || std::memcmp( input.data(), kMagic, 4 ) != 0 )
	{
		return BinaryError::invalidFile;
	}

	Reader r{input, 4};
	uint8 major = 0, minor = 0, patch = 0;

	if( !r.U8( major ) || !r.U8( minor ) || !r.U8( patch ) || major != kVersionMajor || minor != kVersionMinor || patch != kVersionPatch ||
			!r.U32( metadata.sourceTimestamp ) || !r.U32( metadata.mapCRC ) )
	{
		return BinaryError::invalidFile;
	}

	int32 ignored = 0;
	for( int i = 0; i < 4; ++i ) if( !r.I32( ignored ) )
		{
			return BinaryError::invalidFile;
		}

	if( !ReadSettings( r, metadata ) )
	{
		return BinaryError::invalidFile;
	}

	uint32 count = 0;
	std::vector<Vec3> planeNormals;
	std::vector<float> planeDistances;

	if( !r.Count( count ) )
	{
		return BinaryError::tooLarge;
	}

	planeNormals.resize( count );
	planeDistances.resize( count );

	for( uint32 i = 0; i < count; ++i )
	{
		float d = 0;
		if( !r.Vec( planeNormals[i] ) || !r.Float( d ) )
		{
			return BinaryError::invalidFile;
		}
		planeDistances[i] = -d;
	}

	if( !r.Count( count ) )
	{
		return BinaryError::tooLarge;
	}

	file.vertices.resize( count );
	for( Vec3& vertex : file.vertices ) if( !r.Vec( vertex ) )
		{
			return BinaryError::invalidFile;
		}

	struct DiskEdge
	{
		int32 v[2], flags;
	};

	std::vector<DiskEdge> diskEdges;
	if( !r.Count( count ) )
	{
		return BinaryError::tooLarge;
	}

	diskEdges.resize( count );
	for( DiskEdge& edge : diskEdges ) if( !r.I32( edge.v[0] ) || !r.I32( edge.v[1] ) || !r.I32( edge.flags ) )
		{
			return BinaryError::invalidFile;
		}

	std::vector<int32> diskEdgeIndex;
	if( !r.Count( count ) )
	{
		return BinaryError::tooLarge;
	}

	diskEdgeIndex.resize( count );
	for( int32& index : diskEdgeIndex ) if( !r.I32( index ) )
		{
			return BinaryError::invalidFile;
		}

	struct DiskReach
	{
		uint32 flags;
		uint16 from, to;
		Vec3 start, end;
	};

	std::vector<DiskReach> diskReaches;
	if( !r.Count( count ) )
	{
		return BinaryError::tooLarge;
	}

	diskReaches.resize( count );
	for( DiskReach& reach : diskReaches )
	{
		uint16 travel = 0, padding = 0;
		int16 q[6], link = 0;

		if( !r.U32( reach.flags ) || !r.U16( travel ) || !r.U16( reach.from ) || !r.U16( reach.to ) || !r.U16( padding ) )
		{
			return BinaryError::invalidFile;
		}

		for( int i = 0; i < 6; ++i ) if( !r.I16( q[i] ) )
			{
				return BinaryError::invalidFile;
			}

		reach.start = {static_cast<float>( q[0] ), static_cast<float>( q[1] ), static_cast<float>( q[2] )};
		reach.end = {static_cast<float>( q[3] ), static_cast<float>( q[4] ), static_cast<float>( q[5] )};

		if( !r.I32( ignored ) || !r.I16( link ) || !r.I16( link ) )
		{
			return BinaryError::invalidFile;
		}
	}

	struct DiskArea
	{
		uint32 travel;
		uint16 flags;
		int16 numEdges;
		int32 firstEdge;
	};

	std::vector<DiskArea> diskAreas;
	if( !r.Count( count ) || count == 0 )
	{
		return BinaryError::invalidFile;
	}

	diskAreas.resize( count );

	for( DiskArea& area : diskAreas )
	{
		int16 s = 0;
		if( !r.U32( area.travel ) || !r.U16( area.flags ) || !r.I16( area.numEdges ) || !r.I32( area.firstEdge ) )
		{
			return BinaryError::invalidFile;
		}

		for( int i = 0; i < 2; ++i ) if( !r.I16( s ) )
			{
				return BinaryError::invalidFile;
			}

		if( !r.I32( ignored ) )
		{
			return BinaryError::invalidFile;
		}

		for( int i = 0; i < 10; ++i ) if( !r.I16( s ) )
			{
				return BinaryError::invalidFile;
			}
	}

	struct DiskNode
	{
		uint32 plane;
		int32 children[2];
	};

	std::vector<DiskNode> diskNodes;
	if( !r.Count( count ) || count == 0 )
	{
		return BinaryError::invalidFile;
	}

	diskNodes.resize( count );
	for( DiskNode& node : diskNodes )
	{
		uint32 flags = 0;
		if( !r.U32( node.plane ) || !r.U32( flags ) || !r.I32( node.children[0] ) || !r.I32( node.children[1] ) )
		{
			return BinaryError::invalidFile;
		}
	}

	if( !SkipArray( r, 12 ) || !SkipArray( r, 4 ) || !SkipArray( r, 16 ) || !SkipArray( r, 1 ) ||
			!SkipArray( r, 132 ) || !SkipArray( r, 128 ) || !SkipArray( r, 128 ) || !SkipArray( r, 128 ) ||
			!SkipArray( r, 128 ) || !SkipArray( r, 56 ) || !SkipArray( r, 4 ) || !SkipArray( r, 4 ) ||
			!SkipArray( r, 56 ) || !SkipArray( r, 24 ) )
	{
		return BinaryError::invalidFile;
	}

	struct DiskTree
	{
		Vec3 normal;
		int32 head, first, last;
	};

	std::vector<DiskTree> trees;
	if( !r.Count( count ) || count == 0 )
	{
		return BinaryError::invalidFile;
	}

	trees.resize( count );
	for( DiskTree& tree : trees ) if( !r.Vec( tree.normal ) || !r.I32( tree.head ) || !r.I32( tree.first ) || !r.I32( tree.last ) )
		{
			return BinaryError::invalidFile;
		}

	if( !SkipArray( r, 12 ) || r.cursor != input.size() || diskEdges.empty() || diskAreas.empty() || diskNodes.empty() )
	{
		return BinaryError::invalidFile;
	}

	file.edges.resize( diskEdges.size() - 1 );
	for( size_t i = 1; i < diskEdges.size(); ++i )
	{
		if( diskEdges[i].v[0] < 0 || diskEdges[i].v[1] < 0 )
		{
			return BinaryError::invalidFile;
		}
		file.edges[i - 1].vertices[0] = static_cast<uint32>( diskEdges[i].v[0] );
		file.edges[i - 1].vertices[1] = static_cast<uint32>( diskEdges[i].v[1] );
		file.edges[i - 1].flags = static_cast<uint32>( diskEdges[i].flags );
	}

	file.areas.resize( diskAreas.size() - 1 );
	for( size_t i = 1; i < diskAreas.size(); ++i )
	{
		const DiskArea& source = diskAreas[i];
		if( source.numEdges < 0 || source.firstEdge < 0 || static_cast<size_t>( source.firstEdge + source.numEdges ) > diskEdgeIndex.size() )
		{
			return BinaryError::invalidFile;
		}

		Area& area = file.areas[i - 1];
		area.flags = source.flags;
		area.travelFlags = source.travel;
		area.tree = 0;

		for( int j = 0; j < source.numEdges; ++j )
		{
			const int32 signedEdge = diskEdgeIndex[source.firstEdge + j];
			const uint32 absolute = static_cast<uint32>( signedEdge < 0 ? -signedEdge : signedEdge );
			if( absolute == 0 || absolute >= diskEdges.size() )
			{
				return BinaryError::invalidFile;
			}

			area.edges.push_back( absolute - 1 );
			const Edge& edge = file.edges[absolute - 1];
			if( edge.vertices[0] >= file.vertices.size() || edge.vertices[1] >= file.vertices.size() )
			{
				return BinaryError::invalidFile;
			}

			const Vec3& v = file.vertices[edge.vertices[signedEdge < 0 ? 1 : 0]];
			if( j == 0 )
			{
				area.bounds.mins = area.bounds.maxs = v;
			}
			else
			{
				area.bounds.mins.x = Min( area.bounds.mins.x, v.x );
				area.bounds.mins.y = Min( area.bounds.mins.y, v.y );
				area.bounds.mins.z = Min( area.bounds.mins.z, v.z );
				area.bounds.maxs.x = Max( area.bounds.maxs.x, v.x );
				area.bounds.maxs.y = Max( area.bounds.maxs.y, v.y );
				area.bounds.maxs.z = Max( area.bounds.maxs.z, v.z );
			}
			area.center.x += v.x;
			area.center.y += v.y;
			area.center.z += v.z;
		}
		const float inverse = source.numEdges ? 1.0f / source.numEdges : 0.0f;
		area.center.x *= inverse;
		area.center.y *= inverse;
		area.center.z *= inverse;
	}

	file.reachabilities.resize( diskReaches.size() );
	for( size_t i = 0; i < diskReaches.size(); ++i )
	{
		if( diskReaches[i].from == 0 || diskReaches[i].to == 0 || diskReaches[i].from >= diskAreas.size() || diskReaches[i].to >= diskAreas.size() )
		{
			return BinaryError::invalidFile;
		}
		Reachability& reach = file.reachabilities[i];
		reach.fromArea = diskReaches[i].from - 1;
		reach.toArea = diskReaches[i].to - 1;
		reach.travelFlags = diskReaches[i].flags;
		reach.start = diskReaches[i].start;
		reach.end = diskReaches[i].end;
		file.areas[reach.fromArea].reachabilities.push_back( static_cast<uint32>( i ) );
	}

	file.nodes.resize( diskNodes.size() - 1 );
	for( size_t i = 1; i < diskNodes.size(); ++i )
	{
		if( diskNodes[i].plane >= planeNormals.size() )
		{
			return BinaryError::invalidFile;
		}

		Node& node = file.nodes[i - 1];
		node.normal = planeNormals[diskNodes[i].plane];
		node.distance = planeDistances[diskNodes[i].plane];

		for( int side = 0; side < 2; ++side )
		{
			node.children[side] = diskNodes[i].children[side] > 0 ? diskNodes[i].children[side] - 1 : diskNodes[i].children[side];
		}
	}

	for( const DiskTree& source : trees )
	{
		Tree tree;
		tree.floorNormal = source.normal;
		tree.rootNode = source.head > 0 ? source.head - 1 : source.head;

		for( size_t i = 0; i < file.areas.size(); ++i )
		{
			tree.areas.push_back( static_cast<uint32>( i ) );
		}
		file.trees.push_back( std::move( tree ) );
	}

	return ValidateFile( file ) ? BinaryError::none : BinaryError::invalidFile;
}

/*
============
WriteAAS2
============
*/
BinaryError WriteAAS2( const char* path, const File& file, const BinaryMetadata& metadata )
{
	if( !path || !*path )
	{
		return BinaryError::ioError;
	}

	std::vector<uint8> bytes;
	const BinaryError result = SerializeAAS2( file, metadata, bytes );

	if( result != BinaryError::none )
	{
		return result;
	}

	std::ofstream stream( path, std::ios::binary | std::ios::trunc );
	return stream && stream.write( reinterpret_cast<const char*>( bytes.data() ), static_cast<std::streamsize>( bytes.size() ) ) ? BinaryError::none : BinaryError::ioError;
}

/*
============
BinaryErrorName
============
*/
const char* BinaryErrorName( BinaryError error )
{
	switch( error )
	{
		case BinaryError::none:
			return "none";
		case BinaryError::invalidFile:
			return "invalid file";
		case BinaryError::tooManyAreas:
			return "area count exceeds signed AAS2 limit";
		case BinaryError::tooManyReachabilities:
			return "reachability count exceeds signed AAS2 limit";
		case BinaryError::unorientedArea:
			return "area boundary cannot be oriented";
		case BinaryError::tooLarge:
			return "count exceeds format limit";
		case BinaryError::ioError:
			return "I/O error";
	}
	return "unknown";
}