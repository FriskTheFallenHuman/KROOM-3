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

#ifndef __AAS2_BRUSH_SURFACE_H__
#define __AAS2_BRUSH_SURFACE_H__

#include "AAS2_Cspace.h"
#include "AAS2_Types.h"

#include <cstddef>
#include <vector>

enum class BrushSurfaceError
{
	none,
	invalidPlane,
	unboundedOrDegenerateBrush,
	fragmentLimit
};

struct BrushSurfaceResult
{
	BrushSurfaceError error = BrushSurfaceError::none;
	size_t sourceBrush = 0;
	size_t sourcePlane = 0;
	size_t boundaryFaces = 0;
	size_t floorFaces = 0;
	size_t floorTriangles = 0;
	size_t discardedDegenerateTriangles = 0;
	size_t candidatePairs = 0;
	size_t rejectedCandidatePairs = 0;
	size_t peakFragments = 0;
	size_t peakFragmentPoints = 0;

	explicit operator bool() const
	{
		return error == BrushSurfaceError::none;
	}
};

// Intersects normalized convex-brush half-spaces, reconstructs each finite
// boundary polygon, and emits walkable outward-facing polygons as triangles.
// Floor polygons are CSG-subtracted against the other convex brushes before
// triangulation. Full solid BSP node construction and clearance pruning remain
// subsequent stages.
BrushSurfaceResult BuildBrushFloorGeometry(	const std::vector<ConvexBrush>& brushes, float maxFloorSlopeDegrees, float epsilon,	SourceGeometry& output,	ProgressCallback progress = nullptr, void* progressUserData = nullptr );

const char* BrushSurfaceErrorName( BrushSurfaceError error );

#endif /* !__AAS2_BRUSH_SURFACE_H__ */