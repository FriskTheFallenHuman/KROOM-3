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

#ifndef __AAS2_RUNTIME_H__
#define __AAS2_RUNTIME_H__

#include "../../aas2file/AAS2File.h"

class idEntity;

// Stable Doom game-facing travel and area bits. The native runtime translates
// these to the AAS2 disk flags at its boundary.
#define TFL_INVALID				BIT(0)
#define TFL_WALK				BIT(1)
#define TFL_CROUCH				BIT(2)
#define TFL_WALKOFFLEDGE		BIT(3)
#define TFL_BARRIERJUMP			BIT(4)
#define TFL_JUMP				BIT(5)
#define TFL_LADDER				BIT(6)
#define TFL_SWIM				BIT(7)
#define TFL_WATERJUMP			BIT(8)
#define TFL_TELEPORT			BIT(9)
#define TFL_ELEVATOR			BIT(10)
#define TFL_FLY					BIT(11)
#define TFL_SPECIAL				BIT(12)
#define TFL_WATER				BIT(21)
#define TFL_AIR					BIT(22)

#define AREA_FLOOR				BIT(0)
#define AREA_GAP				BIT(1)
#define AREA_LEDGE				BIT(2)
#define AREA_LADDER				BIT(3)
#define AREA_LIQUID				BIT(4)
#define AREA_CROUCH				BIT(5)
#define AREA_REACHABLE_WALK		BIT(6)
#define AREA_REACHABLE_FLY		BIT(7)

#define AREACONTENTS_SOLID			BIT(0)
#define AREACONTENTS_WATER			BIT(1)
#define AREACONTENTS_CLUSTERPORTAL	BIT(2)
#define AREACONTENTS_OBSTACLE		BIT(3)
#define AREACONTENTS_TELEPORTER		BIT(4)

enum aas2PathType_t
{
	AAS2_PATHTYPE_WALK,
	AAS2_PATHTYPE_WALKOFFLEDGE,
	AAS2_PATHTYPE_BARRIERJUMP,
	AAS2_PATHTYPE_JUMP,
	AAS2_PATHTYPE_FLY
};

struct idAAS2Path
{
	int							type;
	idVec3						moveGoal;
	int							moveAreaNum;
	idVec3						secondaryGoal;
	const aas2Reachability_t* 	reachability;
	struct obstacleRoute_t
	{
		struct routeArea_t
		{
			int areaNum;
			idVec3 start;
			idVec3 end;
		};
		int numAreas;
		idVec3 endAlignDir;
		float endTurnRadius;
		routeArea_t areas[11];
	} obstacleRoute;
	idVec3					obstacleGoal;
	int						obstacleAreaNum;
};

struct idAAS2AvoidanceObstacle
{
	idBounds bounds;
	idEntity* entity;
	bool soft;
};

struct idAAS2AvoidancePath
{
	idVec3 seekPos[2];
	idPlane seekPosPlane;
	idEntity* firstObstacle;
	bool hasValidPath;
};

struct idAAS2Goal
{
	int areaNum;
	idVec3 origin;
};
struct idAAS2Obstacle
{
	idBounds absBounds;
	idBounds expAbsBounds;
};
typedef int aas2Handle_t;

class idAAS2Runtime;
class idAAS2Callback
{
public:
	virtual ~idAAS2Callback() {}
	virtual bool TestArea( const idAAS2Runtime* aas, int areaNum ) = 0;
};

class idAAS2Runtime
{
public:
	static idAAS2Runtime* Alloc();
	virtual ~idAAS2Runtime() {}
	virtual bool Init( const idStr& mapName, unsigned int mapFileCRC ) = 0;
	virtual void Stats() const = 0;
	virtual void Test( const idVec3& origin ) = 0;
	virtual const idAAS2Settings* GetSettings() const = 0;
	virtual int PointAreaNum( const idVec3& origin ) const = 0;
	virtual int PointReachableAreaNum( const idVec3& origin, const idBounds& bounds, int areaFlags ) const = 0;
	virtual int BoundsReachableAreaNum( const idBounds& bounds, int areaFlags ) const = 0;
	virtual void PushPointIntoAreaNum( int areaNum, idVec3& origin ) const = 0;
	virtual idVec3 AreaCenter( int areaNum ) const = 0;
	virtual int AreaFlags( int areaNum ) const = 0;
	virtual int AreaTravelFlags( int areaNum ) const = 0;
	virtual bool Trace( aas2Trace_t& trace, const idVec3& start, const idVec3& end ) const = 0;
	virtual const idPlane& GetPlane( int planeNum ) const = 0;
	virtual int GetWallEdges( int areaNum, const idBounds& bounds, int travelFlags, int* edges, int maxEdges ) const = 0;
	virtual void SortWallEdges( int* edges, int numEdges ) const = 0;
	virtual void GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const = 0;
	virtual void GetEdge( int edgeNum, idVec3& start, idVec3& end ) const = 0;
	virtual bool SetAreaState( const idBounds& bounds, int areaContents, bool disabled ) = 0;
	virtual aas2Handle_t AddObstacle( const idBounds& bounds ) = 0;
	virtual void RemoveObstacle( aas2Handle_t handle ) = 0;
	virtual void RemoveAllObstacles() = 0;
	virtual int TravelTimeToGoalArea( int areaNum, const idVec3& origin, int goalAreaNum, int travelFlags ) const = 0;
	virtual bool RouteToGoalArea( int areaNum, const idVec3& origin, int goalAreaNum, int travelFlags, int& travelTime, aas2Reachability_t** reach ) const = 0;
	virtual bool WalkPathToGoal( idAAS2Path& path, int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags ) const = 0;
	virtual bool WalkPathValid( int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags, idVec3& endPos, int& endAreaNum ) const = 0;
	virtual bool FlyPathToGoal( idAAS2Path& path, int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags ) const = 0;
	virtual bool FlyPathValid( int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags, idVec3& endPos, int& endAreaNum ) const = 0;
	virtual void ShowWalkPath( const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin ) const = 0;
	virtual void ShowFlyPath( const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin ) const = 0;
	virtual bool FindNearestGoal( idAAS2Goal& goal, int areaNum, const idVec3& origin, const idVec3& target, int travelFlags, idAAS2Obstacle* obstacles, int numObstacles, idAAS2Callback& callback ) const = 0;
	virtual bool FindPathAroundObstacles( idAAS2AvoidancePath& avoidancePath,
										  const idAAS2Path& routePath, int startAreaNum, const idVec3& startOrigin,
										  const idBounds& actorBounds, const idVec3& lastDirection,
										  const idAAS2AvoidanceObstacle* obstacles, int numObstacles ) const = 0;
};

// Compatibility names used by the Doom game. These aliases preserve the
// source and module ABI while ownership and implementation are entirely AAS2.
typedef idAAS2Runtime idAAS;
typedef idAAS2Settings idAASSettings;
typedef aas2Reachability_t idReachability;
typedef aas2Trace_t aasTrace_t;
typedef idAAS2Path aasPath_t;
typedef idAAS2Goal aasGoal_t;
typedef idAAS2Obstacle aasObstacle_t;
typedef idAAS2Callback idAASCallback;
typedef aas2Handle_t aasHandle_t;

extern idCVar aas2_debugPathing;

#define PATHTYPE_WALK			AAS2_PATHTYPE_WALK
#define PATHTYPE_WALKOFFLEDGE	AAS2_PATHTYPE_WALKOFFLEDGE
#define PATHTYPE_BARRIERJUMP	AAS2_PATHTYPE_BARRIERJUMP
#define PATHTYPE_JUMP			AAS2_PATHTYPE_JUMP
#define PATHTYPE_FLY			AAS2_PATHTYPE_FLY

#endif
