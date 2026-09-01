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

#ifndef __AAS2FILE_H__
#define __AAS2FILE_H__

enum aas2NativeTravelFlags_t
{
	AAS2_TRAVEL_INVALID			= BIT( 0 ),
	AAS2_TRAVEL_WALK			= BIT( 5 ),
	AAS2_TRAVEL_WALK_OFF_LEDGE	= BIT( 6 ),
	AAS2_TRAVEL_LEDGE_GRAB		= BIT( 7 ),
	AAS2_TRAVEL_WATER_JUMP		= BIT( 8 ),
	AAS2_TRAVEL_LADDER			= BIT( 10 ),
	AAS2_TRAVEL_FLY				= BIT( 11 ),
	AAS2_TRAVEL_SPECIAL			= BIT( 12 )
};

enum aas2NativeAreaFlags_t
{
	AAS2_AREA_LEDGE				= BIT( 0 ),
	AAS2_AREA_REACHABLE			= BIT( 1 ),
	AAS2_AREA_OUTSIDE			= BIT( 2 ),
	AAS2_AREA_HIGH_CEILING		= BIT( 3 ),
	AAS2_AREA_NO_PUSH			= BIT( 4 ),
	AAS2_AREA_CLUSTER_PORTAL	= BIT( 10 ),
	AAS2_AREA_OBSTACLE			= BIT( 11 )
};

#define MAX_AAS2_BOUNDING_BOXES		4
#define MAX_AAS2_REACH_PER_AREA		256

typedef int aas2Index_t;

struct aas2Edge_t
{
	int						vertexNum[2];
	int						flags;
};

struct aas2Reachability_t
{
	int						travelFlags;
	int						travelType;
	unsigned short			travelTime;
	unsigned short			fromAreaNum;
	unsigned short			toAreaNum;
	idVec3					start;
	idVec3					end;
	int						edgeNum;
	unsigned char			number;
	unsigned char			disableCount;
	aas2Reachability_t* 	next;
	aas2Reachability_t* 	reverseNext;
};

struct aas2Area_t
{
	int						travelFlags;
	unsigned short			flags;
	unsigned short			contents;
	int						numEdges;
	int						firstEdge;
	short					cluster;
	short					clusterAreaNum;
	idBounds				bounds;
	idVec3					center;
	idPlane					floorPlane;
	aas2Reachability_t* 	reach;
	aas2Reachability_t* 	reverseReach;
	unsigned short			disableCount;
	bool					stateDisabled;
	bool					IsDisabled() const
	{
		return stateDisabled || disableCount != 0;
	}
};

struct aas2Node_t
{
	unsigned int			planeNum;
	unsigned int			flags;
	int						children[2];
};

struct aas2Portal_t
{
	unsigned short			areaNum;
	short					clusters[2];
	short					clusterAreaNum[2];
	unsigned short			maxAreaTravelTime;
};

struct aas2Cluster_t
{
	int						numAreas;
	int						numReachableAreas;
	int						numPortals;
	int						firstPortal;
};

struct aas2Tree_t
{
	idVec3					floorNormal;
	int						headNode;
	int						firstArea;
	int						lastArea;
};

struct aas2Trace_t
{
	int						flags;
	int						travelFlags;
	int						maxAreas;
	bool					getOutOfSolid;
	float					fraction;
	idVec3					endpos;
	int						planeNum;
	int						lastAreaNum;
	int						blockingAreaNum;
	int						numAreas;
	int* 					areas;
	idVec3* 				points;

	aas2Trace_t() : flags( 0 ), travelFlags( 0 ), maxAreas( 0 ), getOutOfSolid( false ),
		fraction( 0.0f ), planeNum( 0 ), lastAreaNum( 0 ), blockingAreaNum( 0 ),
		numAreas( 0 ), areas( NULL ), points( NULL ) {}
};

class idAAS2Settings
{
public:
	idAAS2Settings();
	bool					FromDict( const char* name, const idDict* dict );
	bool					ValidForBounds( const idBounds& bounds ) const;

	int						numBoundingBoxes;
	idBounds				boundingBoxes[MAX_AAS2_BOUNDING_BOXES];
	bool					usePatches;
	bool					writeBrushMap;
	bool					playerFlood;
	bool					noOptimize;
	bool					allowSwimReachabilities;
	bool					allowFlyReachabilities;
	idStr					fileExtension;
	idVec3					gravity;
	idVec3					gravityDir;
	idVec3					invGravityDir;
	float					gravityValue;
	float					maxStepHeight;
	float					maxBarrierHeight;
	float					maxWaterJumpHeight;
	float					maxFallHeight;
	float					minFloorCos;
	float					obstaclePVSRadius;
	int						tt_barrierJump;
	int						tt_startCrouching;
	int						tt_waterJump;
	int						tt_startWalkOffLedge;
};

class idAAS2File
{
public:
	idAAS2File();
	~idAAS2File();

	bool						Load( const idStr& logicalName, unsigned int mapFileCRC );
	void						Clear();
	void						PrintInfo() const;

	const char* 				GetName() const
	{
		return name.c_str();
	}
	unsigned int				GetCRC() const
	{
		return crc;
	}
	const idAAS2Settings& 		GetSettings() const
	{
		return settings;
	}
	int							GetNumAreas() const
	{
		return areas.Num();
	}
	int							GetNumReachabilities() const
	{
		return reachabilities.Num();
	}
	int							GetNumEdges() const
	{
		return edges.Num();
	}
	const aas2Area_t& 			GetArea( int areaNum ) const
	{
		return areas[areaNum];
	}
	aas2Area_t& 				GetArea( int areaNum )
	{
		return areas[areaNum];
	}
	const aas2Reachability_t& 	GetReachability( int index ) const
	{
		return reachabilities[index];
	}
	const aas2Edge_t& 			GetEdge( int index ) const
	{
		return edges[index];
	}
	int							GetEdgeIndex( int index ) const
	{
		return edgeIndex[index];
	}
	const idVec3& 				GetVertex( int index ) const
	{
		return vertices[index];
	}
	const idPlane& 				GetPlane( int index ) const
	{
		return planes[index];
	}
	const aas2Cluster_t& 		GetCluster( int index ) const
	{
		return clusters[index];
	}
	const aas2Portal_t& 		GetPortal( int index ) const
	{
		return portals[index];
	}
	int							GetNumClusters() const
	{
		return clusters.Num();
	}
	int							GetNumPortals() const
	{
		return portals.Num();
	}
	int							GetPortalIndex( int index ) const
	{
		return portalIndex[index];
	}

	idVec3						AreaCenter( int areaNum ) const;
	idBounds					AreaBounds( int areaNum ) const;
	int							PointAreaNum( const idVec3& origin ) const;
	int							PointAreaNumMaxHeight( const idVec3& origin, float maxHeight ) const;
	int							PointAreaCandidates( const idVec3& origin, float maxHeight, int* areaNums, int maxAreaNums ) const;
	int							PointReachableAreaNum( const idVec3& origin, const idBounds& searchBounds, int areaFlags, int excludeTravelFlags ) const;
	int							BoundsReachableAreaNum( const idBounds& bounds, int areaFlags, int excludeTravelFlags ) const;
	bool						PushPointIntoAreaNum( int areaNum, idVec3& point ) const;
	bool						Trace( aas2Trace_t& trace, const idVec3& start, const idVec3& end ) const;

	void						SetAreaTravelFlag( int areaNum, int flag )
	{
		areas[areaNum].travelFlags |= flag;
	}
	void						RemoveAreaTravelFlag( int areaNum, int flag )
	{
		areas[areaNum].travelFlags &= ~flag;
	}

private:
	struct spatialRef_t
	{
		int x;
		int y;
		int areaNum;
	};
	static int					SpatialKey( int x, int y );
	void						BuildSpatialIndex();
	bool						PointInsideArea( int areaNum, const idVec3& point, float& floorHeight ) const;
	float						PointAreaDistanceSqr( int areaNum, const idVec3& point ) const;
	int							BestReachableAreaInBounds( const idBounds& bounds, const idVec3& queryPoint, int areaFlags, int excludeTravelFlags ) const;
	idStr						BinaryName( const idStr& logicalName ) const;

	idStr						name;
	unsigned int				crc;
	idAAS2Settings				settings;
	idList<idPlane, TAG_AAS>	planes;
	idList<idVec3, TAG_AAS>		vertices;
	idList<aas2Edge_t, TAG_AAS>	edges;
	idList<aas2Index_t, TAG_AAS>	edgeIndex;
	idList<aas2Reachability_t, TAG_AAS>	reachabilities;
	idList<aas2Area_t, TAG_AAS>	areas;
	idList<aas2Node_t, TAG_AAS>	nodes;
	idList<aas2Portal_t, TAG_AAS>	portals;
	idList<aas2Index_t, TAG_AAS>	portalIndex;
	idList<aas2Cluster_t, TAG_AAS>	clusters;
	idList<aas2Tree_t, TAG_AAS>	trees;
	idList<spatialRef_t, TAG_AAS>	spatialRefs;
	idHashIndex					spatialHash;
	idList<int, TAG_AAS>			globalAreas;
};

#endif /* !__AAS2FILE_H__ */
