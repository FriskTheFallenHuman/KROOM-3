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

#ifndef __AAS2_TYPES_H__
#define __AAS2_TYPES_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

typedef void ( *ProgressCallback )( const char* stage,
									size_t current,
									size_t total,
									void* userData );

struct Vec3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

struct Bounds
{
	Vec3 mins{};
	Vec3 maxs{};
};

struct SourceTriangle
{
	uint32 vertices[3] {};
	uint32 flags = 0;
};

struct SourceGeometry
{
	std::vector<Vec3> vertices;
	std::vector<SourceTriangle> triangles;
};

struct Edge
{
	uint32 vertices[2] {};
	uint32 flags = 0;
};

struct Reachability
{
	uint32 fromArea = 0;
	uint32 toArea = 0;
	uint32 travelFlags = 0;
	Vec3 start{};
	Vec3 end{};
	std::string name;
};

struct Area
{
	Bounds bounds{};
	Vec3 center{};
	Vec3 floorNormal{0.0f, 0.0f, 1.0f};
	uint16 flags = 0;
	uint32 travelFlags = 0;
	uint32 tree = 0;
	std::vector<uint32> edges;
	std::vector<uint32> reachabilities;
};

struct Node
{
	Vec3 normal{};
	float distance = 0.0f;
	int32 children[2] {};
	Bounds bounds{};
};

struct Tree
{
	int32 rootNode = -1;
	Vec3 floorNormal{0.0f, 0.0f, 1.0f};
	std::vector<uint32> areas;
};

struct File
{
	std::vector<Vec3> vertices;
	std::vector<Edge> edges;
	std::vector<Area> areas;
	std::vector<Node> nodes;
	std::vector<Reachability> reachabilities;
	std::vector<Tree> trees;
};

#endif /* !__AAS2_TYPES_H__ */