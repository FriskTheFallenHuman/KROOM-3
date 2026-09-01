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

#include "AAS2_Map_Adapter.h"

const unsigned short AAS2_CLUSTER_PORTAL_FLAG = 0x0400;
const unsigned short AAS2_OBSTACLE_FLAG = 0x0800;

/*
============
IsAAS2Entity
============
*/
static bool IsAAS2Entity( const idMapEntity& entity, int entityIndex )
{
	if( entityIndex == 0 )
	{
		return true;
	}
	return idStr::Icmp( entity.epairs.GetString( "classname" ),
						"func_aas_obstacle" ) == 0;
}

/*
============
BrushBounds
============
*/
static bool BrushBounds( const ConvexBrush& brush, idBounds& bounds )
{
	bounds.Clear();
	for( size_t first = 0; first < brush.planes.size(); ++first )
	{
		const BrushPlane& a = brush.planes[first];
		const idVec3 an( a.normal.x, a.normal.y, a.normal.z );
		for( size_t second = first + 1; second < brush.planes.size(); ++second )
		{
			const BrushPlane& b = brush.planes[second];
			const idVec3 bn( b.normal.x, b.normal.y, b.normal.z );
			for( size_t third = second + 1; third < brush.planes.size(); ++third )
			{
				const BrushPlane& c = brush.planes[third];
				const idVec3 cn( c.normal.x, c.normal.y, c.normal.z );
				const idVec3 bCrossC = bn.Cross( cn );
				const float denominator = an * bCrossC;
				if( idMath::Fabs( denominator ) <= 1.0e-6f )
				{
					continue;
				}
				const idVec3 point = ( bCrossC * a.distance +
									   cn.Cross( an ) * b.distance + an.Cross( bn ) * c.distance ) /
									 denominator;
				bool inside = true;
				for( const BrushPlane& plane : brush.planes )
				{
					if( idVec3( plane.normal.x, plane.normal.y, plane.normal.z ) * point >
							plane.distance + 0.1f )
					{
						inside = false;
						break;
					}
				}
				if( inside )
				{
					bounds.AddPoint( point );
				}
			}
		}
	}
	return !bounds.IsCleared();
}

/*
============
Intersects
============
*/
static bool Intersects( const Bounds& area, const idBounds& volume )
{
	return area.maxs.x >= volume[0].x && area.mins.x <= volume[1].x &&
		   area.maxs.y >= volume[0].y && area.mins.y <= volume[1].y &&
		   area.maxs.z >= volume[0].z && area.mins.z <= volume[1].z;
}

/*
============
AddPatchTriangle
============
*/
static void AddPatchTriangle( const idVec3& first, const idVec3& second, const idVec3& third, int contents, int entityIndex, int primitiveIndex, std::vector<ConvexBrush>& output, AAS2MapStatistics& statistics )
{
	idVec3 normal = ( second - first ).Cross( third - first );
	if( normal.Normalize() <= 1.0e-6f )
	{
		return;
	}

	ConvexBrush brush;
	brush.contents = static_cast<unsigned int>( contents );
	brush.sourceEntity = static_cast<unsigned int>( entityIndex );
	brush.sourcePrimitive = static_cast<unsigned int>( primitiveIndex );

	const idVec3 points[3] = {first, second, third};
	BrushPlane top;
	top.normal = {normal.x, normal.y, normal.z};
	top.distance = normal * first;
	brush.planes.push_back( top );

	BrushPlane bottom;
	bottom.normal = { -normal.x, -normal.y, -normal.z};
	bottom.distance = -( normal * first ) + 1.0f;

	brush.planes.push_back( bottom );
	for( int edgeIndex = 0; edgeIndex < 3; ++edgeIndex )
	{
		const idVec3& start = points[edgeIndex];
		const idVec3& end = points[( edgeIndex + 1 ) % 3];
		idVec3 sideNormal = ( end - start ).Cross( normal );

		if( sideNormal.Normalize() <= 1.0e-6f )
		{
			return;
		}

		BrushPlane side;
		side.normal = {sideNormal.x, sideNormal.y, sideNormal.z};
		side.distance = sideNormal * start;
		brush.planes.push_back( side );
	}
	output.push_back( std::move( brush ) );
	++statistics.acceptedPatchBrushes;
}

/*
============
CollectAAS2Brushes
============
*/
bool CollectAAS2Brushes( const idMapFile& mapFile, const AAS2PrimitivePolicy& primitivePolicy, std::vector<ConvexBrush>& output, std::vector<AAS2AreaVolume>& areaVolumes, AAS2MapStatistics& statistics )
{
	output.clear();
	areaVolumes.clear();
	statistics = AAS2MapStatistics{};

	const int solidContents = CONTENTS_SOLID | CONTENTS_AAS_SOLID | CONTENTS_MONSTERCLIP;
	const int volumeContents = CONTENTS_AREAPORTAL | CONTENTS_AAS_OBSTACLE;
	const int aasContents = solidContents | volumeContents;

	for( int entityIndex = 0; entityIndex < mapFile.GetNumEntities(); ++entityIndex )
	{
		const idMapEntity* entity = mapFile.GetEntity( entityIndex );
		if( entity == nullptr || !IsAAS2Entity( *entity, entityIndex ) )
		{
			continue;
		}

		++statistics.entities;

		idVec3 origin;
		idMat3 axis;
		entity->epairs.GetVector( "origin", "0 0 0", origin );
		if( !entity->epairs.GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", axis ) )
		{
			const float angle = entity->epairs.GetFloat( "angle" );
			axis = angle != 0.0f ? idAngles( 0.0f, angle, 0.0f ).ToMat3() : mat3_identity;
		}

		for( int primitiveIndex = 0; primitiveIndex < entity->GetNumPrimitives(); ++primitiveIndex )
		{
			const idMapPrimitive* primitive = entity->GetPrimitive( primitiveIndex );
			if( primitive == nullptr )
			{
				continue;
			}

			if( primitive->GetType() == idMapPrimitive::TYPE_PATCH )
			{
				++statistics.patchPrimitives;
				if( !primitivePolicy.includePatches )
				{
					++statistics.ignoredPatches;
					continue;
				}

				const idMapPatch* mapPatch = static_cast<const idMapPatch*>( primitive );
				const idMaterial* material = declManager->FindMaterial( mapPatch->GetMaterial() );
				const int contents = material != nullptr ? material->GetContentFlags() : 0;

				if( ( contents & aasContents ) == 0 )
				{
					++statistics.ignoredPatches;
					continue;
				}

				idSurface_Patch mesh( *mapPatch );
				if( mapPatch->GetExplicitlySubdivided() )
				{
					mesh.SubdivideExplicit( mapPatch->GetHorzSubdivisions(),
											mapPatch->GetVertSubdivisions(),
											false, true );
				}
				else
				{
					mesh.Subdivide( DEFAULT_CURVE_MAX_ERROR_CD,
									DEFAULT_CURVE_MAX_ERROR_CD,
									DEFAULT_CURVE_MAX_LENGTH_CD, false );
				}

				for( int x = 0; x < mesh.GetWidth() - 1; ++x )
				{
					for( int y = 0; y < mesh.GetHeight() - 1; ++y )
					{
						const int firstIndex = y * mesh.GetWidth() + x;
						const int secondIndex = firstIndex + 1;
						const int thirdIndex = firstIndex + mesh.GetWidth() + 1;
						const int fourthIndex = firstIndex + mesh.GetWidth();
						const idVec3 first = mesh[firstIndex].xyz * axis + origin;
						const idVec3 second = mesh[secondIndex].xyz * axis + origin;
						const idVec3 third = mesh[thirdIndex].xyz * axis + origin;
						const idVec3 fourth = mesh[fourthIndex].xyz * axis + origin;
						AddPatchTriangle( first, second, third, contents,
										  entityIndex, primitiveIndex,
										  output, statistics );
						AddPatchTriangle( first, third, fourth, contents,
										  entityIndex, primitiveIndex,
										  output, statistics );
					}
				}
				continue;
			}

			if( primitive->GetType() != idMapPrimitive::TYPE_BRUSH )
			{
				continue;
			}

			++statistics.brushPrimitives;

			if( !primitivePolicy.includeBrushes )
			{
				++statistics.ignoredBrushes;
				continue;
			}

			const idMapBrush* mapBrush = static_cast<const idMapBrush*>( primitive );
			int contents = 0;

			for( int sideIndex = 0; sideIndex < mapBrush->GetNumSides(); ++sideIndex )
			{
				const idMapBrushSide* side = mapBrush->GetSide( sideIndex );
				const idMaterial* material = declManager->FindMaterial( side->GetMaterial() );

				if( material != nullptr )
				{
					contents |= material->GetContentFlags();
				}
			}

			if( ( contents & aasContents ) == 0 )
			{
				++statistics.ignoredBrushes;
				continue;
			}

			ConvexBrush brush;
			brush.contents = static_cast<unsigned int>( contents );
			brush.sourceEntity = static_cast<unsigned int>( entityIndex );
			brush.sourcePrimitive = static_cast<unsigned int>( primitiveIndex );
			brush.planes.reserve( mapBrush->GetNumSides() );

			for( int sideIndex = 0; sideIndex < mapBrush->GetNumSides(); ++sideIndex )
			{
				const idPlane& localPlane = mapBrush->GetSide( sideIndex )->GetPlane();
				const idVec3 worldNormal = localPlane.Normal() * axis;
				BrushPlane plane;
				plane.normal = {worldNormal.x, worldNormal.y, worldNormal.z};
				plane.distance = -localPlane[3] + worldNormal * origin;
				brush.planes.push_back( plane );
			}

			unsigned short areaFlags = 0;
			if( contents & CONTENTS_AREAPORTAL )
			{
				areaFlags |= AAS2_CLUSTER_PORTAL_FLAG;
			}
			if( contents & CONTENTS_AAS_OBSTACLE )
			{
				areaFlags |= AAS2_OBSTACLE_FLAG;
			}

			if( areaFlags != 0 )
			{
				AAS2AreaVolume volume;
				if( BrushBounds( brush, volume.bounds ) )
				{
					volume.areaFlags = areaFlags;
					areaVolumes.push_back( volume );
					++statistics.areaVolumes;
				}
			}

			if( ( contents & solidContents ) == 0 )
			{
				continue;
			}

			output.push_back( std::move( brush ) );
			++statistics.acceptedBrushes;
		}
	}

	return !output.empty();
}

/*
============
ApplyAAS2AreaVolumes
============
*/
void ApplyAAS2AreaVolumes( File& file, const std::vector<AAS2AreaVolume>& areaVolumes, const AgentBounds& agent, AAS2MapStatistics& statistics )
{
	for( const AAS2AreaVolume& source : areaVolumes )
	{
		idBounds expanded;
		expanded[0].Set( source.bounds[0].x - agent.maxs.x,
						 source.bounds[0].y - agent.maxs.y,
						 source.bounds[0].z - agent.maxs.z );
		expanded[1].Set( source.bounds[1].x - agent.mins.x,
						 source.bounds[1].y - agent.mins.y,
						 source.bounds[1].z - agent.mins.z );
		for( Area& area : file.areas )
		{
			const unsigned short before = area.flags;
			if( Intersects( area.bounds, expanded ) )
			{
				area.flags = static_cast<unsigned short>( area.flags | source.areaFlags );
			}
			if( before != area.flags )
			{
				++statistics.taggedAreas;
			}
		}
	}
}
