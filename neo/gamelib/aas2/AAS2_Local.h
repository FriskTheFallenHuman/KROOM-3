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

#ifndef __AAS2_LOCAL_H__
#define __AAS2_LOCAL_H__

#include "AAS2.h"

extern idCVar aas2_debugPathing;

class idAAS2RuntimeLocal : public idAAS2Runtime
{
public:
	idAAS2RuntimeLocal();
	virtual ~idAAS2RuntimeLocal();
	bool Init( const idStr& mapName, unsigned int mapFileCRC ) override;
	void Stats() const override;
	void Test( const idVec3& origin ) override;
	const idAAS2Settings* GetSettings() const override;
	int PointAreaNum( const idVec3& origin ) const override;
	int PointReachableAreaNum( const idVec3& origin, const idBounds& bounds, int areaFlags ) const override;
	int BoundsReachableAreaNum( const idBounds& bounds, int areaFlags ) const override;
	void PushPointIntoAreaNum( int areaNum, idVec3& origin ) const override;
	idVec3 AreaCenter( int areaNum ) const override;
	int AreaFlags( int areaNum ) const override;
	int AreaTravelFlags( int areaNum ) const override;
	bool Trace( aas2Trace_t& trace, const idVec3& start, const idVec3& end ) const override;
	const idPlane& GetPlane( int planeNum ) const override;
	int GetWallEdges( int areaNum, const idBounds& bounds, int travelFlags, int* edges, int maxEdges ) const override;
	void SortWallEdges( int* edges, int numEdges ) const override;
	void GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const override;
	void GetEdge( int edgeNum, idVec3& start, idVec3& end ) const override;
	bool SetAreaState( const idBounds& bounds, int areaContents, bool disabled ) override;
	aas2Handle_t AddObstacle( const idBounds& bounds ) override;
	void RemoveObstacle( aas2Handle_t handle ) override;
	void RemoveAllObstacles() override;
	int TravelTimeToGoalArea( int areaNum, const idVec3& origin, int goalAreaNum, int travelFlags ) const override;
	bool RouteToGoalArea( int areaNum, const idVec3& origin, int goalAreaNum, int travelFlags, int& travelTime, aas2Reachability_t** reach ) const override;
	bool WalkPathToGoal( idAAS2Path& path, int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags ) const override;
	bool WalkPathValid( int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags, idVec3& endPos, int& endAreaNum ) const override;
	bool FlyPathToGoal( idAAS2Path& path, int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags ) const override;
	bool FlyPathValid( int areaNum, const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin, int travelFlags, idVec3& endPos, int& endAreaNum ) const override;
	void ShowWalkPath( const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin ) const override;
	void ShowFlyPath( const idVec3& origin, int goalAreaNum, const idVec3& goalOrigin ) const override;
	bool FindNearestGoal( idAAS2Goal& goal, int areaNum, const idVec3& origin, const idVec3& target, int travelFlags, idAAS2Obstacle* obstacles, int numObstacles, idAAS2Callback& callback ) const override;
	bool FindPathAroundObstacles( idAAS2AvoidancePath& avoidancePath, const idAAS2Path& routePath, int startAreaNum, const idVec3& startOrigin,
								  const idBounds& actorBounds, const idVec3& lastDirection, const idAAS2AvoidanceObstacle* obstacles, int numObstacles ) const override;

private:
	struct obstacle_t
	{
		idBounds bounds;
		idList<int, TAG_AAS> areas;
	};
	bool RouteToGoalAreaChain( int areaNum, const idVec3& origin, int goalAreaNum,
							   int travelFlags, int& travelTime, aas2Reachability_t** route,
							   int maxRoute, int& routeCount ) const;
	bool WalkPathValidFromArea( int areaNum, const idVec3& origin,
								int goalAreaNum, const idVec3& goalOrigin, int travelFlags,
								idVec3& endPos, int& endAreaNum ) const;
	idVec3 SubSampleWalkPath( int areaNum, const idVec3& origin,
							  int pathAreaNum, const idVec3& pathStart, const idVec3& pathEnd,
							  int travelFlags, int& endAreaNum ) const;
	void Shutdown();
	bool TravelAllowed( const aas2Reachability_t& reach, int travelFlags ) const;
	bool AreaBlockedByQueryObstacles( int areaNum, const idAAS2Obstacle* obstacles, int numObstacles ) const;

	idAAS2File* file;
	idList<obstacle_t*, TAG_AAS> obstacleList;
};

#endif /* !__AAS2_LOCAL_H__ */
