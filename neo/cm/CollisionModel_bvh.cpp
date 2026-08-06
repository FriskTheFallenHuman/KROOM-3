/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2026 - Justin Marshall(aka IceColdDuke).

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

/*
===============================================================================

	Runtime BVHs for collision-model polygons and brushes.

	The axial tree is retained as the editable/on-disk representation.  Once a
	model is complete, its unique primitives are packed into immutable BVHs for
	queries.  Exact collision tests remain in CollisionModel_translate.cpp,
	CollisionModel_rotate.cpp and CollisionModel_contents.cpp.

===============================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "CollisionModel_local.h"

static const int CM_POLYGON_BVH_LEAF_SIZE = 4;
static const int CM_BRUSH_BVH_LEAF_SIZE = 4;
static const int CM_BVH_STACK_SIZE = 64;

typedef struct cm_bvhBuildRef_s
{
	void* 		primitive;
	idBounds	bounds;
	idVec3		center;
	int			contents;
	int			ordinal;
} cm_bvhBuildRef_t;

/*
================
CM_CollectPolygons
================
*/
static void CM_CollectPolygons( cm_node_t* node, int checkCount, idList<cm_bvhBuildRef_t>& refs )
{
	cm_polygonRef_t* pref;

	while( node )
	{
		for( pref = node->polygons; pref; pref = pref->next )
		{
			cm_polygon_t* polygon = pref->p;
			if( polygon->checkcount == checkCount )
			{
				continue;
			}
			polygon->checkcount = checkCount;

			cm_bvhBuildRef_t ref;
			ref.primitive = polygon;
			ref.bounds = polygon->bounds;
			ref.center = polygon->bounds.GetCenter();
			ref.contents = polygon->contents;
			ref.ordinal = refs.Num();
			refs.Append( ref );
		}

		if( node->planeType == -1 )
		{
			return;
		}
		CM_CollectPolygons( node->children[1], checkCount, refs );
		node = node->children[0];
	}
}

/*
================
CM_CollectBrushes
================
*/
static void CM_CollectBrushes( cm_node_t* node, int checkCount, idList<cm_bvhBuildRef_t>& refs )
{
	cm_brushRef_t* bref;

	while( node )
	{
		for( bref = node->brushes; bref; bref = bref->next )
		{
			cm_brush_t* brush = bref->b;
			if( brush->checkcount == checkCount )
			{
				continue;
			}
			brush->checkcount = checkCount;

			cm_bvhBuildRef_t ref;
			ref.primitive = brush;
			ref.bounds = brush->bounds;
			ref.center = brush->bounds.GetCenter();
			ref.contents = brush->contents;
			ref.ordinal = refs.Num();
			refs.Append( ref );
		}

		if( node->planeType == -1 )
		{
			return;
		}
		CM_CollectBrushes( node->children[1], checkCount, refs );
		node = node->children[0];
	}
}

static ID_INLINE bool CM_BVHRefLess( const cm_bvhBuildRef_t& a, const cm_bvhBuildRef_t& b, int axis )
{
	if( a.center[axis] < b.center[axis] )
	{
		return true;
	}
	if( a.center[axis] > b.center[axis] )
	{
		return false;
	}
	return a.ordinal < b.ordinal;
}

static int CM_CountBVHNodes( int primitiveCount, int leafSize )
{
	if( primitiveCount <= leafSize )
	{
		return 1;
	}
	const int leftCount = primitiveCount >> 1;
	return 1 + CM_CountBVHNodes( leftCount, leafSize ) + CM_CountBVHNodes( primitiveCount - leftCount, leafSize );
}

/*
================
CM_BVHTraceBounds

Returns the segment entry fraction for a translated trace model.  Position
and rotational tests already carry conservative world-space bounds and use a
plain overlap test.
================
*/
static bool CM_BVHTraceBounds( const cm_traceWork_t* tw, const idBounds& bounds, float& entryFraction )
{
	if( !bounds.IntersectsBounds( tw->bounds ) )
	{
		return false;
	}
	if( tw->positionTest || tw->rotation )
	{
		entryFraction = 0.0f;
		return true;
	}

	float enter = 0.0f;
	float leave = 1.0f;
	for( int axis = 0; axis < 3; axis++ )
	{
		const float minimum = bounds[0][axis] - tw->size[1][axis] - CM_BOX_EPSILON;
		const float maximum = bounds[1][axis] - tw->size[0][axis] + CM_BOX_EPSILON;
		const float direction = tw->end[axis] - tw->start[axis];
		if( idMath::Fabs( direction ) < 1e-8f )
		{
			if( tw->start[axis] < minimum || tw->start[axis] > maximum )
			{
				return false;
			}
			continue;
		}

		float fraction0 = ( minimum - tw->start[axis] ) / direction;
		float fraction1 = ( maximum - tw->start[axis] ) / direction;
		if( fraction0 > fraction1 )
		{
			const float temp = fraction0;
			fraction0 = fraction1;
			fraction1 = temp;
		}
		enter = Max( enter, fraction0 );
		leave = Min( leave, fraction1 );
		if( enter > leave )
		{
			return false;
		}
	}

	if( !tw->getContacts && enter > tw->trace.fraction )
	{
		return false;
	}
	entryFraction = enter;
	return true;
}

/*
================
CM_SortBVHRefs
================
*/
static void CM_SortBVHRefs( cm_bvhBuildRef_t* refs, int left, int right, int axis )
{
	int i = left;
	int j = right;
	const cm_bvhBuildRef_t pivot = refs[( left + right ) >> 1];

	while( i <= j )
	{
		while( CM_BVHRefLess( refs[i], pivot, axis ) )
		{
			i++;
		}
		while( CM_BVHRefLess( pivot, refs[j], axis ) )
		{
			j--;
		}
		if( i <= j )
		{
			const cm_bvhBuildRef_t temp = refs[i];
			refs[i] = refs[j];
			refs[j] = temp;
			i++;
			j--;
		}
	}
	if( left < j )
	{
		CM_SortBVHRefs( refs, left, j, axis );
	}
	if( i < right )
	{
		CM_SortBVHRefs( refs, i, right, axis );
	}
}

/*
================
CM_BuildBVHNode
================
*/
static int CM_BuildBVHNode( cm_bvhNode_t* nodes, int& numNodes, cm_bvhBuildRef_t* refs,
							int first, int count, int leafSize )
{
	const int nodeNum = numNodes++;
	cm_bvhNode_t& node = nodes[nodeNum];
	idBounds centerBounds;
	int i;

	node.bounds = refs[first].bounds;
	centerBounds.Clear();
	node.contents = 0;
	for( i = first; i < first + count; i++ )
	{
		node.bounds.AddBounds( refs[i].bounds );
		centerBounds.AddPoint( refs[i].center );
		node.contents |= refs[i].contents;
	}

	if( count <= leafSize )
	{
		node.children[0] = node.children[1] = -1;
		node.firstPrimitive = first;
		node.numPrimitives = count;
		return nodeNum;
	}

	const idVec3 centerSize = centerBounds[1] - centerBounds[0];
	int axis = 0;
	if( centerSize[1] > centerSize[axis] )
	{
		axis = 1;
	}
	if( centerSize[2] > centerSize[axis] )
	{
		axis = 2;
	}

	CM_SortBVHRefs( refs, first, first + count - 1, axis );
	const int leftCount = count >> 1;
	node.firstPrimitive = 0;
	node.numPrimitives = 0;
	node.children[0] = CM_BuildBVHNode( nodes, numNodes, refs, first, leftCount, leafSize );
	node.children[1] = CM_BuildBVHNode( nodes, numNodes, refs, first + leftCount, count - leftCount, leafSize );
	return nodeNum;
}

/*
================
idCollisionModelManagerLocal::FreeModelBVHs
================
*/
void idCollisionModelManagerLocal::FreeModelBVHs( cm_model_t* model )
{
	if( model->polygonBvh.nodes )
	{
		Mem_Free( model->polygonBvh.nodes );
	}
	if( model->polygonBvh.polygons )
	{
		Mem_Free( model->polygonBvh.polygons );
	}
	if( model->brushBvh.nodes )
	{
		Mem_Free( model->brushBvh.nodes );
	}
	if( model->brushBvh.brushes )
	{
		Mem_Free( model->brushBvh.brushes );
	}
	memset( &model->polygonBvh, 0, sizeof( model->polygonBvh ) );
	memset( &model->brushBvh, 0, sizeof( model->brushBvh ) );
}

/*
================
idCollisionModelManagerLocal::BuildModelBVHs
================
*/
void idCollisionModelManagerLocal::BuildModelBVHs( cm_model_t* model )
{
	idList<cm_bvhBuildRef_t> refs;
	int i;

	FreeModelBVHs( model );
	if( !model->node )
	{
		return;
	}

	checkCount++;
	CM_CollectPolygons( model->node, checkCount, refs );
	if( refs.Num() > 0 )
	{
		const int maxNodes = CM_CountBVHNodes( refs.Num(), CM_POLYGON_BVH_LEAF_SIZE );
		model->polygonBvh.nodes = ( cm_bvhNode_t* )Mem_ClearedAlloc( maxNodes * sizeof( cm_bvhNode_t ), TAG_COLLISION );
		model->polygonBvh.polygons = ( cm_polygon_t** )Mem_Alloc( refs.Num() * sizeof( cm_polygon_t* ), TAG_COLLISION );
		model->polygonBvh.numPolygons = refs.Num();
		CM_BuildBVHNode( model->polygonBvh.nodes, model->polygonBvh.numNodes, refs.Ptr(), 0, refs.Num(), CM_POLYGON_BVH_LEAF_SIZE );
		for( i = 0; i < refs.Num(); i++ )
		{
			model->polygonBvh.polygons[i] = ( cm_polygon_t* )refs[i].primitive;
		}
	}

	refs.Clear();
	checkCount++;
	CM_CollectBrushes( model->node, checkCount, refs );
	if( refs.Num() > 0 )
	{
		const int maxNodes = CM_CountBVHNodes( refs.Num(), CM_BRUSH_BVH_LEAF_SIZE );
		model->brushBvh.nodes = ( cm_bvhNode_t* )Mem_ClearedAlloc( maxNodes * sizeof( cm_bvhNode_t ), TAG_COLLISION );
		model->brushBvh.brushes = ( cm_brush_t** )Mem_Alloc( refs.Num() * sizeof( cm_brush_t* ), TAG_COLLISION );
		model->brushBvh.numBrushes = refs.Num();
		CM_BuildBVHNode( model->brushBvh.nodes, model->brushBvh.numNodes, refs.Ptr(), 0, refs.Num(), CM_BRUSH_BVH_LEAF_SIZE );
		for( i = 0; i < refs.Num(); i++ )
		{
			model->brushBvh.brushes[i] = ( cm_brush_t* )refs[i].primitive;
		}
	}
}

/*
================
idCollisionModelManagerLocal::PointContentsThroughModel
================
*/
int idCollisionModelManagerLocal::PointContentsThroughModel( const idVec3& p, cm_model_t* model )
{
	int i;
	float d;

	if( model->brushBvh.numNodes > 0 )
	{
		int stack[CM_BVH_STACK_SIZE];
		int stackDepth = 0;
		stack[stackDepth++] = 0;

		while( stackDepth > 0 )
		{
			const cm_bvhNode_t& node = model->brushBvh.nodes[stack[--stackDepth]];
			if( !node.bounds.ContainsPoint( p ) )
			{
				continue;
			}
			if( node.numPrimitives > 0 )
			{
				for( i = node.firstPrimitive; i < node.firstPrimitive + node.numPrimitives; i++ )
				{
					cm_brush_t* brush = model->brushBvh.brushes[i];
					if( !brush->bounds.ContainsPoint( p ) )
					{
						continue;
					}
					int planeNum;
					for( planeNum = 0; planeNum < brush->numPlanes; planeNum++ )
					{
						d = brush->planes[planeNum].Distance( p );
						if( d >= 0.0f )
						{
							break;
						}
					}
					if( planeNum == brush->numPlanes )
					{
						return brush->contents;
					}
				}
			}
			else
			{
				assert( stackDepth + 2 <= CM_BVH_STACK_SIZE );
				stack[stackDepth++] = node.children[1];
				stack[stackDepth++] = node.children[0];
			}
		}
		return 0;
	}

	// The shared temporary trace-model handle deliberately has no BVH.
	for( cm_brushRef_t* bref = model->node ? model->node->brushes : NULL; bref; bref = bref->next )
	{
		cm_brush_t* brush = bref->b;
		if( !brush->bounds.ContainsPoint( p ) )
		{
			continue;
		}
		for( i = 0; i < brush->numPlanes; i++ )
		{
			if( brush->planes[i].Distance( p ) >= 0.0f )
			{
				break;
			}
		}
		if( i == brush->numPlanes )
		{
			return brush->contents;
		}
	}
	return 0;
}

/*
================
idCollisionModelManagerLocal::TraceThroughModel
================
*/
void idCollisionModelManagerLocal::TraceThroughModel( cm_traceWork_t* tw )
{
	int i;

	// Position tests use solid brushes first, just as the old axial traversal did.
	if( tw->positionTest )
	{
		if( tw->model->brushBvh.numNodes > 0 )
		{
			int stack[CM_BVH_STACK_SIZE];
			int stackDepth = 0;
			stack[stackDepth++] = 0;
			while( stackDepth > 0 && tw->trace.fraction != 0.0f )
			{
				const cm_bvhNode_t& node = tw->model->brushBvh.nodes[stack[--stackDepth]];
				if( !( node.contents & tw->contents ) || !node.bounds.IntersectsBounds( tw->bounds ) )
				{
					continue;
				}
				if( node.numPrimitives > 0 )
				{
					for( i = node.firstPrimitive; i < node.firstPrimitive + node.numPrimitives; i++ )
					{
						if( TestTrmVertsInBrush( tw, tw->model->brushBvh.brushes[i] ) )
						{
							break;
						}
					}
				}
				else
				{
					assert( stackDepth + 2 <= CM_BVH_STACK_SIZE );
					stack[stackDepth++] = node.children[1];
					stack[stackDepth++] = node.children[0];
				}
			}
		}
		else
		{
			for( cm_brushRef_t* bref = tw->model->node ? tw->model->node->brushes : NULL; bref; bref = bref->next )
			{
				if( TestTrmVertsInBrush( tw, bref->b ) )
				{
					break;
				}
			}
		}

		if( tw->trace.fraction == 0.0f || tw->pointTrace )
		{
			return;
		}
	}

	if( tw->model->polygonBvh.numNodes > 0 )
	{
		int stack[CM_BVH_STACK_SIZE];
		int stackDepth = 0;
		stack[stackDepth++] = 0;
		while( stackDepth > 0 )
		{
			const cm_bvhNode_t& node = tw->model->polygonBvh.nodes[stack[--stackDepth]];
			float nodeEntry;
			if( tw->quickExit || ( tw->positionTest && tw->trace.fraction == 0.0f ) ||
					( tw->rotation && tw->maxTan == 0.0f ) )
			{
				return;
			}
			if( !( node.contents & tw->contents ) || !CM_BVHTraceBounds( tw, node.bounds, nodeEntry ) )
			{
				continue;
			}
			if( node.numPrimitives > 0 )
			{
				for( i = node.firstPrimitive; i < node.firstPrimitive + node.numPrimitives; i++ )
				{
					cm_polygon_t* polygon = tw->model->polygonBvh.polygons[i];
					bool stop;
					if( tw->positionTest )
					{
						stop = TestTrmInPolygon( tw, polygon );
					}
					else if( tw->rotation )
					{
						stop = RotateTrmThroughPolygon( tw, polygon );
					}
					else
					{
						stop = TranslateTrmThroughPolygon( tw, polygon );
					}
					if( stop || tw->quickExit )
					{
						return;
					}
				}
			}
			else
			{
				const cm_bvhNode_t& child0 = tw->model->polygonBvh.nodes[node.children[0]];
				const cm_bvhNode_t& child1 = tw->model->polygonBvh.nodes[node.children[1]];
				float entry0 = 0.0f;
				float entry1 = 0.0f;
				const bool visit0 = ( child0.contents & tw->contents ) && CM_BVHTraceBounds( tw, child0.bounds, entry0 );
				const bool visit1 = ( child1.contents & tw->contents ) && CM_BVHTraceBounds( tw, child1.bounds, entry1 );
				assert( stackDepth + ( visit0 ? 1 : 0 ) + ( visit1 ? 1 : 0 ) <= CM_BVH_STACK_SIZE );
				if( visit0 && visit1 )
				{
					// The nearer child is popped first so early hits tighten tw->bounds.
					if( entry0 <= entry1 )
					{
						stack[stackDepth++] = node.children[1];
						stack[stackDepth++] = node.children[0];
					}
					else
					{
						stack[stackDepth++] = node.children[0];
						stack[stackDepth++] = node.children[1];
					}
				}
				else if( visit0 )
				{
					stack[stackDepth++] = node.children[0];
				}
				else if( visit1 )
				{
					stack[stackDepth++] = node.children[1];
				}
			}
		}
		return;
	}

	// Temporary trace models are a single node whose references change per call.
	for( cm_polygonRef_t* pref = tw->model->node ? tw->model->node->polygons : NULL; pref; pref = pref->next )
	{
		bool stop;
		if( tw->positionTest )
		{
			stop = TestTrmInPolygon( tw, pref->p );
		}
		else if( tw->rotation )
		{
			stop = RotateTrmThroughPolygon( tw, pref->p );
		}
		else
		{
			stop = TranslateTrmThroughPolygon( tw, pref->p );
		}
		if( stop || tw->quickExit )
		{
			return;
		}
	}
}
