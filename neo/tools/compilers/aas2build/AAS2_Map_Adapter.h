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

#ifndef __AAS2_MAP_ADAPTER_H__
#define __AAS2_MAP_ADAPTER_H__

#include "AAS2_CSpace.h"

#include <cstddef>
#include <vector>

class idMapFile;

struct AAS2MapStatistics
{
	size_t entities = 0;
	size_t brushPrimitives = 0;
	size_t patchPrimitives = 0;
	size_t acceptedBrushes = 0;
	size_t ignoredBrushes = 0;
	size_t ignoredPatches = 0;
	size_t acceptedPatchBrushes = 0;
	size_t areaVolumes = 0;
	size_t taggedAreas = 0;
};

struct AAS2PrimitivePolicy
{
	bool includeBrushes = true;
	bool includePatches = false;
};

struct AAS2AreaVolume
{
	idBounds bounds;
	unsigned short areaFlags = 0;
};

// Converts worldspawn and func_aas_obstacle brushes from Doom's parsed map
// representation into the portable compiler model. Material contents determine
// whether a brush participates in AAS construction.
bool CollectAAS2Brushes( const idMapFile& mapFile, const AAS2PrimitivePolicy& primitivePolicy,	std::vector<ConvexBrush>& output, std::vector<AAS2AreaVolume>& areaVolumes, AAS2MapStatistics& statistics );
void ApplyAAS2AreaVolumes( File& file, const std::vector<AAS2AreaVolume>& areaVolumes, const AgentBounds& agent, AAS2MapStatistics& statistics );

#endif /* !__AAS2_MAP_ADAPTER_H__ */