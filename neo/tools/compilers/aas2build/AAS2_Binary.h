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

#ifndef __AAS2_BINARY_H__
#define __AAS2_BINARY_H__

#include "AAS2_Cspace.h"
#include "AAS2_Types.h"

#include <cstdint>
#include <string>
#include <vector>

struct BinaryMetadata
{
	uint32 mapCRC = 0;
	uint32 sourceTimestamp = 0;
	std::string name;
	std::string sourcePath;
	AgentBounds agent{{ -16.0f, -16.0f, 0.0f},
		{16.0f, 16.0f, 72.0f}};
	float maxStepHeight = 18.0f;
	float maxBarrierHeight = 32.0f;
	int32 maxFallHeight = 128;
	bool usePatches = true;
	std::string fileExtensionAAS = "aas48";
	std::string groupName = "aas";
	std::string explicitGroupName = "aas48";
	int32 type = 1;
	int32 primitiveModeBrush = 0;
	int32 primitiveModePatch = 1;
	int32 primitiveModeModel = 1;
	Vec3 gravityDir{0.0f, 0.0f, -1.0f};
	float gravityValue = 1066.0f;
	float maxWaterJumpHeight = 20.0f;
	float minFloorCos = 0.7f;
	float minHighCeiling = 80.0f;
	float groundSpeed = 250.0f;
	float waterSpeed = 150.0f;
	float ladderSpeed = 50.0f;
	float wallCornerEdgeRadius = 16.0f;
	float ledgeCornerEdgeRadius = 16.0f;
	float obstaclePVSRadius = 1024.0f;
	float minCrouchingCoverHeight = 32.0f;
	float minStandingCoverHeight = 64.0f;
	float crouchingFireHeight = 48.0f;
	float standingFireHeight = 72.0f;
	float minWallWidth = 8.0f;
	float maxWallWidth = 32.0f;
	float minDoorWidth = 32.0f;
	float maxDoorWidth = 80.0f;
	float coverCornerDistance = 8.0f;
	float coverWallDistance = 8.0f;
	float chokePointWidth = 96.0f;
	int32 ttBarrierJump = 100;
	int32 ttWaterJump = 100;
	int32 ttStartWalkOffLedge = 100;
	int32 ttStartLadderClimb = 100;
};

enum class BinaryError
{
	none,
	invalidFile,
	tooManyAreas,
	tooManyReachabilities,
	unorientedArea,
	tooLarge,
	ioError
};

BinaryError SerializeAAS2( const File& file, const BinaryMetadata& metadata, std::vector<uint8>& output );
BinaryError DeserializeAAS2( const std::vector<uint8>& input, File& file, BinaryMetadata& metadata );
BinaryError WriteAAS2( const char* path, const File& file, const BinaryMetadata& metadata );

const char* BinaryErrorName( BinaryError error );

#endif /* !__AAS2_BINARY_H__ */