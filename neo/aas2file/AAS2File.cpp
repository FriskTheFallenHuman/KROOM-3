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

const int AAS2_MAX_COUNT = 16000000;

/*
============
AAS2Reader
============
*/
struct AAS2Reader
{
	idFile* file;
	bool Read( void* data, int size )
	{
		return size >= 0 && file->Read( data, size ) == size;
	}
	bool U8( unsigned char& value )
	{
		return Read( &value, 1 );
	}
	bool U16( unsigned short& value )
	{
		unsigned char bytes[2];
		if( !Read( bytes, 2 ) )
		{
			return false;
		}
		value = static_cast<unsigned short>( ( bytes[0] << 8 ) | bytes[1] );
		return true;
	}
	bool I16( short& value )
	{
		unsigned short raw;
		if( !U16( raw ) )
		{
			return false;
		}
		value = static_cast<short>( raw );
		return true;
	}
	bool U32( unsigned int& value )
	{
		unsigned char bytes[4];
		if( !Read( bytes, 4 ) )
		{
			return false;
		}
		value = ( static_cast<unsigned int>( bytes[0] ) << 24 ) |
				( static_cast<unsigned int>( bytes[1] ) << 16 ) |
				( static_cast<unsigned int>( bytes[2] ) << 8 ) | bytes[3];
		return true;
	}
	bool I32( int& value )
	{
		unsigned int raw;
		if( !U32( raw ) )
		{
			return false;
		}
		value = static_cast<int>( raw );
		return true;
	}
	bool Float( float& value )
	{
		unsigned int raw;
		if( !U32( raw ) )
		{
			return false;
		}
		memcpy( &value, &raw, sizeof( value ) );
		return std::isfinite( value ) != 0;
	}
	bool Vec3( idVec3& value )
	{
		return Float( value.x ) && Float( value.y ) && Float( value.z );
	}
	bool String( idStr& value )
	{
		unsigned char bytes[4];
		if( !Read( bytes, 4 ) )
		{
			return false;
		}
		const unsigned int length = bytes[0] | ( bytes[1] << 8 ) | ( bytes[2] << 16 ) | ( bytes[3] << 24 );
		if( length > 1048576u || length > static_cast<unsigned int>( file->Length() - file->Tell() ) )
		{
			return false;
		}
		value.Fill( ' ', length );
		return length == 0 || Read( &value[0], length );
	}
	bool Count( int& value )
	{
		unsigned int raw;
		if( !U32( raw ) || raw > AAS2_MAX_COUNT )
		{
			return false;
		}
		value = static_cast<int>( raw );
		return true;
	}
	bool SkipArray( int elementSize )
	{
		int count;
		if( !Count( count ) || elementSize < 0 )
		{
			return false;
		}
		const long long bytes = static_cast<long long>( count ) * elementSize;
		return bytes >= 0 && bytes <= file->Length() - file->Tell() && bytes <= 0x7fffffff &&
			   file->Seek( static_cast<long>( bytes ), FS_SEEK_CUR ) >= 0;
	}
};

/*
============
diskSettings_t
============
*/
struct diskSettings_t
{
	idStr extension;
	idBounds bounds;
	idVec3 gravityDir;
	float gravityValue;
	float maxStepHeight;
	float maxBarrierHeight;
	float maxWaterJumpHeight;
	float maxFallHeight;
	float minFloorCos;
	float obstaclePVSRadius;
};

/*
============
ReadSettings
============
*/
static bool ReadSettings( AAS2Reader& reader, diskSettings_t& settings )
{
	int ignored;
	idStr ignoredString;
	if( !reader.I32( ignored ) || !reader.String( settings.extension ) ||
			!reader.String( ignoredString ) || !reader.String( ignoredString ) ||
			!reader.Vec3( settings.bounds[0] ) || !reader.Vec3( settings.bounds[1] ) ||
			!reader.I32( ignored ) || !reader.I32( ignored ) || !reader.I32( ignored ) ||
			!reader.Vec3( settings.gravityDir ) )
	{
		return false;
	}
	float values[24];
	for( int i = 0; i < 24; ++i ) if( !reader.Float( values[i] ) )
		{
			return false;
		}

	settings.gravityValue = values[0];
	settings.maxStepHeight = values[1];
	settings.maxBarrierHeight = values[2];
	settings.maxWaterJumpHeight = values[3];
	settings.maxFallHeight = values[4];
	settings.minFloorCos = values[5];
	settings.obstaclePVSRadius = values[12];

	for( int i = 0; i < 4; ++i ) if( !reader.I32( ignored ) )
		{
			return false;
		}

	return true;
}

/*
============
SharedEdge
============
*/
static int SharedEdge( const aas2Area_t& from, const aas2Area_t& to, const idList<int, TAG_AAS>& indexes )
{
	for( int i = 0; i < from.numEdges; ++i )
	{
		const int first = idMath::Abs( indexes[from.firstEdge + i] );
		for( int j = 0; j < to.numEdges; ++j )
		{
			if( first == idMath::Abs( indexes[to.firstEdge + j] ) )
			{
				return first;
			}
		}
	}
	return 0;
}

/*
============
idAAS2Settings::idAAS2Settings
============
*/
idAAS2Settings::idAAS2Settings()
{
	numBoundingBoxes = 1;
	boundingBoxes[0] = idBounds( idVec3( -16, -16, 0 ), idVec3( 16, 16, 72 ) );
	usePatches = true;
	writeBrushMap = false;
	playerFlood = false;
	noOptimize = false;
	allowSwimReachabilities = false;
	allowFlyReachabilities = false;
	fileExtension = "aas48";
	gravity.Set( 0, 0, -1066 );
	gravityDir = gravity;
	gravityValue = gravityDir.Normalize();
	invGravityDir = -gravityDir;
	maxStepHeight = 14.0f;
	maxBarrierHeight = 32.0f;
	maxWaterJumpHeight = 20.0f;
	maxFallHeight = 64.0f;
	minFloorCos = 0.7f;
	obstaclePVSRadius = 1024.0f;
	tt_barrierJump = 100;
	tt_startCrouching = 100;
	tt_waterJump = 100;
	tt_startWalkOffLedge = 100;
}

/*
============
idAAS2Settings::FromDict
============
*/
bool idAAS2Settings::FromDict( const char* profileName, const idDict* dict )
{
	if( dict == NULL )
	{
		return false;
	}
	dict->GetVector( "mins", "-16 -16 0", boundingBoxes[0][0] );
	dict->GetVector( "maxs", "16 16 72", boundingBoxes[0][1] );
	numBoundingBoxes = 1;
	dict->GetBool( "usePatches", "1", usePatches );
	dict->GetBool( "writeBrushMap", "0", writeBrushMap );
	dict->GetBool( "playerFlood", "0", playerFlood );
	dict->GetBool( "allowSwimReachabilities", "0", allowSwimReachabilities );
	dict->GetBool( "allowFlyReachabilities", "0", allowFlyReachabilities );
	dict->GetString( "fileExtension", profileName != NULL ? profileName : "aas48", fileExtension );
	dict->GetVector( "gravity", "0 0 -1066", gravity );
	gravityDir = gravity;
	gravityValue = gravityDir.Normalize();
	invGravityDir = -gravityDir;
	dict->GetFloat( "maxStepHeight", "14", maxStepHeight );
	dict->GetFloat( "maxBarrierHeight", "32", maxBarrierHeight );
	dict->GetFloat( "maxWaterJumpHeight", "20", maxWaterJumpHeight );
	dict->GetFloat( "maxFallHeight", "64", maxFallHeight );
	dict->GetFloat( "minFloorCos", "0.7", minFloorCos );
	dict->GetFloat( "obstaclePVSRadius", "1024", obstaclePVSRadius );
	dict->GetInt( "tt_barrierJump", "100", tt_barrierJump );
	dict->GetInt( "tt_startCrouching", "100", tt_startCrouching );
	dict->GetInt( "tt_waterJump", "100", tt_waterJump );
	dict->GetInt( "tt_startWalkOffLedge", "100", tt_startWalkOffLedge );
	return true;
}

/*
============
idAAS2Settings::ValidForBounds
============
*/
bool idAAS2Settings::ValidForBounds( const idBounds& bounds ) const
{
	for( int axis = 0; axis < 3; ++axis )
	{
		if( bounds[0][axis] < boundingBoxes[0][0][axis] || bounds[1][axis] > boundingBoxes[0][1][axis] )
		{
			return false;
		}
	}
	return true;
}

/*
============
idAAS2File::idAAS2File
============
*/
idAAS2File::idAAS2File() : crc( 0 )
{
	planes.SetGranularity( 4096 );
	vertices.SetGranularity( 4096 );
	edges.SetGranularity( 4096 );
	edgeIndex.SetGranularity( 4096 );
	reachabilities.SetGranularity( 4096 );
	areas.SetGranularity( 1024 );
	nodes.SetGranularity( 1024 );
	portals.SetGranularity( 1024 );
	portalIndex.SetGranularity( 4096 );
	clusters.SetGranularity( 1024 );
	trees.SetGranularity( 8 );
	spatialRefs.SetGranularity( 4096 );
}

/*
============
idAAS2File::~idAAS2File
============
*/
idAAS2File::~idAAS2File()
{
	Clear();
}

/*
============
idAAS2File::Clear
============
*/
void idAAS2File::Clear()
{
	name.Clear();
	crc = 0;
	planes.Clear();
	vertices.Clear();
	edges.Clear();
	edgeIndex.Clear();
	reachabilities.Clear();
	areas.Clear();
	nodes.Clear();
	portals.Clear();
	portalIndex.Clear();
	clusters.Clear();
	trees.Clear();
	spatialRefs.Clear();
	spatialHash.Clear();
	globalAreas.Clear();
}

/*
============
idAAS2File::BinaryName
============
*/
idStr idAAS2File::BinaryName( const idStr& logicalName ) const
{
	idStr extension;
	logicalName.ExtractFileExtension( extension );
	idStr suffix = extension;
	if( suffix.Icmpn( "aas_", 4 ) == 0 )
	{
		suffix = suffix.Mid( 4, suffix.Length() - 4 );
	}
	else if( suffix.Icmpn( "aas", 3 ) == 0 )
	{
		suffix = suffix.Mid( 3, suffix.Length() - 3 );
	}
	idStr result = logicalName;
	result.StripFileExtension();
	result += "_";
	result += suffix;
	result += ".aas2";
	return result;
}

/*
============
idAAS2File::Load
============
*/
bool idAAS2File::Load( const idStr& logicalName, unsigned int mapFileCRC )
{
	const idStr binaryName = BinaryName( logicalName );
	idFile* source = fileSystem->OpenFileRead( binaryName );
	if( source == NULL )
	{
		return false;
	}
	AAS2Reader reader = { source };
	bool ok = false;
	do
	{
		unsigned char magic[4], major, minor, patch;
		unsigned int timestamp, diskCRC;
		int ignored;
		if( !reader.Read( magic, 4 ) || memcmp( magic, "2SAA", 4 ) != 0 ||
				!reader.U8( major ) || !reader.U8( minor ) || !reader.U8( patch ) ||
				major != 3 || minor != 18 || patch != 'a' || !reader.U32( timestamp ) || !reader.U32( diskCRC ) )
		{
			break;
		}
		for( int i = 0; i < 4; ++i ) if( !reader.I32( ignored ) )
			{
				goto loadFailed;
			}
		if( mapFileCRC != 0 && diskCRC != mapFileCRC )
		{
			common->Warning( "AAS2 file '%s' is out of date", binaryName.c_str() );
			break;
		}
		diskSettings_t diskSettings;
		if( !ReadSettings( reader, diskSettings ) )
		{
			break;
		}

		int count;
		if( !reader.Count( count ) )
		{
			break;
		}
		planes.SetNum( count );
		for( int i = 0; i < count; ++i )
		{
			idVec3 normal;
			float distance;
			if( !reader.Vec3( normal ) || !reader.Float( distance ) )
			{
				goto loadFailed;
			}
			planes[i].SetNormal( normal );
			planes[i][3] = distance;
		}
		if( !reader.Count( count ) )
		{
			break;
		}
		vertices.SetNum( count );
		for( int i = 0; i < count; ++i ) if( !reader.Vec3( vertices[i] ) )
			{
				goto loadFailed;
			}
		if( !reader.Count( count ) || count < 1 )
		{
			break;
		}
		edges.SetNum( count );
		for( int i = 0; i < count; ++i ) if( !reader.I32( edges[i].vertexNum[0] ) || !reader.I32( edges[i].vertexNum[1] ) || !reader.I32( edges[i].flags ) )
			{
				goto loadFailed;
			}
		if( !reader.Count( count ) )
		{
			break;
		}
		edgeIndex.SetNum( count );
		for( int i = 0; i < count; ++i ) if( !reader.I32( edgeIndex[i] ) )
			{
				goto loadFailed;
			}

		if( !reader.Count( count ) )
		{
			break;
		}
		reachabilities.SetNum( count );
		memset( reachabilities.Ptr(), 0, reachabilities.MemoryUsed() );
		idList<short> diskNext, diskReverseNext;
		diskNext.SetNum( count );
		diskReverseNext.SetNum( count );
		for( int i = 0; i < count; ++i )
		{
			unsigned int flags;
			unsigned short padding;
			short coordinates[6];
			if( !reader.U32( flags ) || !reader.U16( reachabilities[i].travelTime ) ||
					!reader.U16( reachabilities[i].fromAreaNum ) || !reader.U16( reachabilities[i].toAreaNum ) ||
					!reader.U16( padding ) )
			{
				goto loadFailed;
			}
			for( int j = 0; j < 6; ++j ) if( !reader.I16( coordinates[j] ) )
				{
					goto loadFailed;
				}
			reachabilities[i].travelFlags = flags;
			reachabilities[i].start.Set( coordinates[0], coordinates[1], coordinates[2] );
			reachabilities[i].end.Set( coordinates[3], coordinates[4], coordinates[5] );
			if( !reader.I32( ignored ) || !reader.I16( diskNext[i] ) || !reader.I16( diskReverseNext[i] ) )
			{
				goto loadFailed;
			}
		}

		if( !reader.Count( count ) || count < 1 )
		{
			break;
		}
		areas.SetNum( count );
		memset( areas.Ptr(), 0, areas.MemoryUsed() );
		idList<short> firstReach, firstReverseReach;
		firstReach.SetNum( count );
		firstReverseReach.SetNum( count );
		for( int i = 0; i < count; ++i )
		{
			unsigned int travel;
			short numEdges, cluster, clusterArea, reach, reverseReach, extra;
			unsigned short flags;
			if( !reader.U32( travel ) || !reader.U16( flags ) || !reader.I16( numEdges ) ||
					!reader.I32( areas[i].firstEdge ) || !reader.I16( cluster ) || !reader.I16( clusterArea ) ||
					!reader.I32( ignored ) || !reader.I16( reach ) || !reader.I16( reverseReach ) )
			{
				goto loadFailed;
			}
			for( int j = 0; j < 8; ++j ) if( !reader.I16( extra ) )
				{
					goto loadFailed;
				}
			areas[i].travelFlags = static_cast<int>( travel );
			areas[i].flags = flags;
			areas[i].numEdges = numEdges;
			areas[i].cluster = cluster;
			areas[i].clusterAreaNum = clusterArea;
			firstReach[i] = reach;
			firstReverseReach[i] = reverseReach;
		}

		if( !reader.Count( count ) || count < 1 )
		{
			break;
		}
		nodes.SetNum( count );
		for( int i = 0; i < count; ++i ) if( !reader.U32( nodes[i].planeNum ) || !reader.U32( nodes[i].flags ) ||
												 !reader.I32( nodes[i].children[0] ) || !reader.I32( nodes[i].children[1] ) )
			{
				goto loadFailed;
			}
		if( !reader.Count( count ) )
		{
			break;
		}
		portals.SetNum( count );
		for( int i = 0; i < count; ++i ) if( !reader.U16( portals[i].areaNum ) || !reader.I16( portals[i].clusters[0] ) ||
												 !reader.I16( portals[i].clusters[1] ) || !reader.I16( portals[i].clusterAreaNum[0] ) ||
												 !reader.I16( portals[i].clusterAreaNum[1] ) || !reader.U16( portals[i].maxAreaTravelTime ) )
			{
				goto loadFailed;
			}
		if( !reader.Count( count ) )
		{
			break;
		}
		portalIndex.SetNum( count );
		for( int i = 0; i < count; ++i ) if( !reader.I32( portalIndex[i] ) )
			{
				goto loadFailed;
			}
		if( !reader.Count( count ) )
		{
			break;
		}
		clusters.SetNum( count );
		for( int i = 0; i < count; ++i ) if( !reader.I32( clusters[i].numAreas ) || !reader.I32( clusters[i].numReachableAreas ) ||
												 !reader.I32( clusters[i].numPortals ) || !reader.I32( clusters[i].firstPortal ) )
			{
				goto loadFailed;
			}

		if( !reader.SkipArray( 1 ) || !reader.SkipArray( 132 ) || !reader.SkipArray( 128 ) ||
				!reader.SkipArray( 128 ) || !reader.SkipArray( 128 ) || !reader.SkipArray( 128 ) ||
				!reader.SkipArray( 56 ) || !reader.SkipArray( 4 ) || !reader.SkipArray( 4 ) ||
				!reader.SkipArray( 56 ) || !reader.SkipArray( 24 ) )
		{
			break;
		}
		if( !reader.Count( count ) || count < 1 )
		{
			break;
		}
		trees.SetNum( count );
		for( int i = 0; i < count; ++i ) if( !reader.Vec3( trees[i].floorNormal ) || !reader.I32( trees[i].headNode ) ||
												 !reader.I32( trees[i].firstArea ) || !reader.I32( trees[i].lastArea ) )
			{
				goto loadFailed;
			}
		if( !reader.Count( count ) || count != areas.Num() )
		{
			break;
		}
		for( int i = 0; i < count; ++i )
		{
			short quantized[6];
			for( int j = 0; j < 6; ++j ) if( !reader.I16( quantized[j] ) )
				{
					goto loadFailed;
				}
			areas[i].bounds[0].Set( quantized[0], quantized[1], quantized[2] );
			areas[i].bounds[1].Set( quantized[3], quantized[4], quantized[5] );
		}
		if( reader.file->Tell() != reader.file->Length() )
		{
			break;
		}

		name = logicalName;
		crc = diskCRC;
		const idDeclEntityDef* profile = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, diskSettings.extension, false ) );
		if( profile != NULL )
		{
			settings.FromDict( diskSettings.extension, &profile->dict );
		}
		else
		{
			settings.fileExtension = diskSettings.extension;
			settings.boundingBoxes[0] = diskSettings.bounds;
			settings.gravityDir = diskSettings.gravityDir;
			settings.invGravityDir = -diskSettings.gravityDir;
			settings.gravityValue = diskSettings.gravityValue;
			settings.gravity = diskSettings.gravityDir * diskSettings.gravityValue;
			settings.maxStepHeight = diskSettings.maxStepHeight;
			settings.maxBarrierHeight = diskSettings.maxBarrierHeight;
			settings.maxWaterJumpHeight = diskSettings.maxWaterJumpHeight;
			settings.maxFallHeight = diskSettings.maxFallHeight;
			settings.minFloorCos = diskSettings.minFloorCos;
			settings.obstaclePVSRadius = diskSettings.obstaclePVSRadius;
		}

		bool oneBasedClusterAreas = false;
		for( int i = 1; i < areas.Num(); ++i )
		{
			const int clusterNum = areas[i].cluster;
			if( clusterNum > 0 && clusterNum < clusters.Num() && areas[i].clusterAreaNum >= clusters[clusterNum].numReachableAreas )
			{
				oneBasedClusterAreas = true;
			}
		}
		for( int i = 1; i < areas.Num(); ++i )
		{
			aas2Area_t& area = areas[i];
			if( area.numEdges < 3 || area.firstEdge < 0 || area.firstEdge + area.numEdges > edgeIndex.Num() )
			{
				goto loadFailed;
			}
			if( oneBasedClusterAreas )
			{
				--area.clusterAreaNum;
			}
			area.flags |= AAS2_AREA_REACHABLE;
			if( area.flags & AAS2_AREA_CLUSTER_PORTAL )
			{
				area.contents |= BIT( 2 );
			}
			if( area.flags & AAS2_AREA_OBSTACLE )
			{
				area.contents |= BIT( 3 );
			}
			idVec3 firstVertex;
			idVec3 previousVertex;
			bool haveFloorPlane = false;
			area.center.Zero();
			area.bounds.Clear();
			for( int j = 0; j < area.numEdges; ++j )
			{
				const int signedEdge = edgeIndex[area.firstEdge + j];
				const int edgeNum = idMath::Abs( signedEdge );
				if( edgeNum <= 0 || edgeNum >= edges.Num() )
				{
					goto loadFailed;
				}
				const int vertexNum = edges[edgeNum].vertexNum[signedEdge < 0 ? 1 : 0];
				if( vertexNum < 0 || vertexNum >= vertices.Num() )
				{
					goto loadFailed;
				}
				const idVec3& vertex = vertices[vertexNum];
				if( j == 0 )
				{
					firstVertex = vertex;
				}
				else if( j >= 2 && !haveFloorPlane )
				{
					haveFloorPlane = area.floorPlane.FromPoints( firstVertex, previousVertex, vertex );
				}
				previousVertex = vertex;
				area.center += vertex;
				area.bounds.AddPoint( vertex );
			}
			// Convex merged floors may retain collinear boundary subdivisions. The
			// first three vertices are therefore not guaranteed to define a plane.
			if( !haveFloorPlane )
			{
				goto loadFailed;
			}
			if( area.floorPlane.Normal() * settings.gravityDir > 0.0f )
			{
				area.floorPlane = -area.floorPlane;
			}
			area.center /= static_cast<float>( area.numEdges );
		}
		for( int i = 0; i < reachabilities.Num(); ++i )
		{
			aas2Reachability_t& reach = reachabilities[i];
			if( reach.fromAreaNum == 0 || reach.fromAreaNum >= areas.Num() || reach.toAreaNum == 0 || reach.toAreaNum >= areas.Num() )
			{
				goto loadFailed;
			}
			reach.edgeNum = SharedEdge( areas[reach.fromAreaNum], areas[reach.toAreaNum], edgeIndex );
			if( reach.travelFlags & AAS2_TRAVEL_WALK_OFF_LEDGE )
			{
				reach.travelType = BIT( 3 );
			}
			else if( reach.travelFlags & AAS2_TRAVEL_LEDGE_GRAB )
			{
				reach.travelType = BIT( 4 );
			}
			else if( reach.travelFlags & AAS2_TRAVEL_WATER_JUMP )
			{
				reach.travelType = BIT( 8 );
			}
			else if( reach.travelFlags & AAS2_TRAVEL_LADDER )
			{
				reach.travelType = BIT( 6 );
			}
			else if( reach.travelFlags & AAS2_TRAVEL_FLY )
			{
				reach.travelType = BIT( 11 );
			}
			else
			{
				reach.travelType = BIT( 1 );
			}
			reach.next = diskNext[i] >= 0 ? &reachabilities[diskNext[i]] : NULL;
			reach.reverseNext = diskReverseNext[i] >= 0 ? &reachabilities[diskReverseNext[i]] : NULL;
		}
		for( int i = 1; i < areas.Num(); ++i )
		{
			areas[i].reach = firstReach[i] >= 0 ? &reachabilities[firstReach[i]] : NULL;
			areas[i].reverseReach = firstReverseReach[i] >= 0 ? &reachabilities[firstReverseReach[i]] : NULL;
			int ordinal = 0;
			for( aas2Reachability_t* reach = areas[i].reach; reach != NULL; reach = reach->next )
			{
				if( ordinal >= MAX_AAS2_REACH_PER_AREA )
				{
					goto loadFailed;
				}
				reach->number = static_cast<unsigned char>( ordinal++ );
			}
		}
		BuildSpatialIndex();
		common->Printf( "loaded native AAS2 '%s': %d areas, %d reachabilities\n", binaryName.c_str(), areas.Num() - 1, reachabilities.Num() );
		ok = true;
	}
	while( false );
loadFailed:
	fileSystem->CloseFile( source );
	if( !ok )
	{
		Clear();
		common->Warning( "Invalid native AAS2 file '%s'", binaryName.c_str() );
	}
	return ok;
}

/*
============
idAAS2File::PrintInfo
============
*/
void idAAS2File::PrintInfo() const
{
	common->Printf( "%6d areas\n%6d reachabilities\n%6d vertices\n%6d edges\n", areas.Num() - 1, reachabilities.Num(), vertices.Num(), edges.Num() - 1 );
}
