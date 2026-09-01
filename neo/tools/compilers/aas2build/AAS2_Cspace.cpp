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

#include "AAS2_Cspace.h"

#include <algorithm>
#include <cmath>
#include <utility>

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
Length
============
*/
static float Length( const Vec3& value )
{
	return std::sqrt( value.x * value.x + value.y * value.y + value.z * value.z );
}

/*
============
ReflectedBoundsSupport
============
*/
static float ReflectedBoundsSupport( const AgentBounds& bounds, const Vec3& normal )
{
	const float x = normal.x >= 0.0f ? -bounds.mins.x : -bounds.maxs.x;
	const float y = normal.y >= 0.0f ? -bounds.mins.y : -bounds.maxs.y;
	const float z = normal.z >= 0.0f ? -bounds.mins.z : -bounds.maxs.z;
	return normal.x * x + normal.y * y + normal.z * z;
}

/*
============
MakeAgentBounds
============
*/
AgentBounds MakeAgentBounds( float radius, float height )
{
	const float safeRadius = Max( 0.0f, radius );
	const float safeHeight = Max( 0.0f, height );
	AgentBounds bounds;
	bounds.mins = { -safeRadius, -safeRadius, 0.0f};
	bounds.maxs = {safeRadius, safeRadius, safeHeight};
	return bounds;
}

/*
============
ExpandConfigurationSpace
============
*/
CSpaceResult ExpandConfigurationSpace( const std::vector<ConvexBrush>& input, const AgentBounds& agent, std::vector<ConvexBrush>& output )
{
	output.clear();
	CSpaceResult result;
	if( !Finite( agent.mins ) || !Finite( agent.maxs ) ||
			agent.mins.x > agent.maxs.x ||
			agent.mins.y > agent.maxs.y ||
			agent.mins.z > agent.maxs.z )
	{
		result.error = CSpaceError::invalidAgentBounds;
		return result;
	}

	output.reserve( input.size() );
	for( size_t brushIndex = 0; brushIndex < input.size(); ++brushIndex )
	{
		const ConvexBrush& source = input[brushIndex];
		if( source.planes.empty() )
		{
			output.clear();
			result.error = CSpaceError::emptyBrush;
			result.sourceBrush = brushIndex;
			return result;
		}

		ConvexBrush expanded;
		expanded.contents = source.contents;
		expanded.sourceEntity = source.sourceEntity;
		expanded.sourcePrimitive = source.sourcePrimitive;
		expanded.planes.reserve( source.planes.size() );

		for( size_t planeIndex = 0; planeIndex < source.planes.size(); ++planeIndex )
		{
			const BrushPlane& sourcePlane = source.planes[planeIndex];
			const float normalLength = Length( sourcePlane.normal );
			if( !Finite( sourcePlane.normal ) || !Finite( sourcePlane.distance ) ||
					!Finite( normalLength ) || normalLength <= 1.0e-8f )
			{
				output.clear();
				result.error = CSpaceError::invalidPlane;
				result.sourceBrush = brushIndex;
				result.sourcePlane = planeIndex;
				return result;
			}

			BrushPlane plane;
			plane.normal = {sourcePlane.normal.x / normalLength,
							sourcePlane.normal.y / normalLength,
							sourcePlane.normal.z / normalLength
						   };
			plane.distance = sourcePlane.distance / normalLength;
			plane.distance += ReflectedBoundsSupport( agent, plane.normal );
			expanded.planes.push_back( plane );
			++result.planes;
		}
		output.push_back( std::move( expanded ) );
	}
	result.brushes = output.size();
	return result;
}

/*
============
CSpaceErrorName
============
*/
const char* CSpaceErrorName( CSpaceError error )
{
	switch( error )
	{
		case CSpaceError::none:
			return "none";
		case CSpaceError::invalidAgentBounds:
			return "invalid agent bounds";
		case CSpaceError::emptyBrush:
			return "empty brush";
		case CSpaceError::invalidPlane:
			return "invalid brush plane";
	}
	return "unknown";
}