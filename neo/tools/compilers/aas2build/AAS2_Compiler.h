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

#ifndef __AAS2_COMPILER_H__
#define __AAS2_COMPILER_H__

#include "AAS2_Types.h"

#include <cstddef>

struct Settings
{
	float maxFloorSlopeDegrees = 45.0f;
	float maxStepHeight = 18.0f;
	float maxFallHeight = 128.0f;
	float agentHeight = 72.0f;
	float agentRadius = 16.0f;
	float weldEpsilon = 0.01f;
	bool mergeCoplanarFloors = true;
	float floorMergeNormalDegrees = 1.0f;
	float floorMergeDistanceEpsilon = 0.05f;
};

struct Statistics
{
	size_t inputTriangles = 0;
	size_t walkableTriangles = 0;
	size_t rejectedSteepTriangles = 0;
	size_t rejectedDegenerateTriangles = 0;
	size_t rejectedDuplicateTriangles = 0;
	size_t nonManifoldEdges = 0;
	size_t mergedFloorPairs = 0;
	size_t vertices = 0;
	size_t edges = 0;
	size_t areas = 0;
	size_t reachabilities = 0;
	size_t stepReachabilities = 0;
	size_t fallReachabilities = 0;
	size_t ledgeGrabReachabilities = 0;
};

enum class Error
{
	none,
	emptyGeometry,
	invalidVertexIndex,
	degenerateTriangle,
	nonManifoldEdge,
	invalidOutput
};

struct Result
{
	Error error = Error::none;
	size_t sourceTriangle = 0;
	Statistics statistics{};

	explicit operator bool() const
	{
		return error == Error::none;
	}
};

// Reconstructed surface/floor slice: converts indexed walkable triangle
// geometry into the portable AAS2 model, merges convex coplanar floors, and
// builds shared-boundary reachability. Configuration-space expansion, BSP,
// obstacle PVS, and spatial analysis remain subsequent pipeline stages.
Result CompileSurface( const SourceGeometry& source, const Settings& settings, File& output, ProgressCallback progress = nullptr, void* progressUserData = nullptr );

// Adds endpoint/T-junction transitions for areas tagged by dynamic portal or
// obstacle volumes after those tags have been applied by the map adapter.
size_t AddDynamicVolumeReachabilities( File& file, const Settings& settings );

bool ValidateFile( const File& file );
const char* ErrorName( Error error );

#endif /* !__AAS2_COMPILER_H__ */