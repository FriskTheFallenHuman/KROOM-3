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

#ifndef __AAS2_CSPACE_H__
#define __AAS2_CSPACE_H__

#include "AAS2_Types.h"

#include <cstddef>
#include <cstdint>
#include <vector>

// Plane convention: points inside a convex brush satisfy
// Dot(plane.normal, point) <= plane.distance.
struct BrushPlane
{
	Vec3 normal{};
	float distance = 0.0f;
};

struct ConvexBrush
{
	std::vector<BrushPlane> planes;
	uint32 contents = 0;
	uint32 sourceEntity = 0;
	uint32 sourcePrimitive = 0;
};

struct AgentBounds
{
	Vec3 mins{};
	Vec3 maxs{};
};

enum class CSpaceError
{
	none,
	invalidAgentBounds,
	emptyBrush,
	invalidPlane
};

struct CSpaceResult
{
	CSpaceError error = CSpaceError::none;
	size_t sourceBrush = 0;
	size_t sourcePlane = 0;
	size_t brushes = 0;
	size_t planes = 0;

	explicit operator bool() const
	{
		return error == CSpaceError::none;
	}
};

AgentBounds MakeAgentBounds( float radius, float height );

// Builds obstacle configuration space for an agent whose origin is translated
// through the world. This is the convex-brush Minkowski expansion that precedes
// BSP construction; brush contents and source provenance are preserved.
CSpaceResult ExpandConfigurationSpace( const std::vector<ConvexBrush>& input, const AgentBounds& agent, std::vector<ConvexBrush>& output );

const char* CSpaceErrorName( CSpaceError error );

#endif /* !__AAS2_CSPACE_H__ */