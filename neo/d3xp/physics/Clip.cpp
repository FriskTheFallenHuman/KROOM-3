/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#include "precompiled.h"
#pragma hdrstop


#include "../Game_local.h"

#define CLIP_BROADPHASE_MARGIN		8.0f
#define CLIP_BROADPHASE_PREDICTION	2.0f

typedef struct clipBroadPhaseNode_s
{
	idBounds				bounds;
	idClipModel* 			clipModel;
	int						parent;
	int						children[2];
	int						height;
	int						next;
} clipBroadPhaseNode_t;

class idClipBroadPhase
{
public:
	idClipBroadPhase();

	int						CreateProxy( const idBounds& bounds, idClipModel* clipModel );
	void					DestroyProxy( int proxy );
	void					MoveProxy( int proxy, const idBounds& bounds, const idVec3& displacement );
	int						Query( const idBounds& bounds, int contentMask, idClipModel** list, int maxCount ) const;

private:
	idList<clipBroadPhaseNode_t> nodes;
	int						root;
	int						freeList;

	int						AllocNode();
	void					FreeNode( int node );
	void					InsertLeaf( int leaf );
	void					RemoveLeaf( int leaf );
	int						Balance( int node );

	static bool				IsLeaf( const clipBroadPhaseNode_t& node );
	static bool				ContainsBounds( const idBounds& outer, const idBounds& inner );
	static idBounds			UnionBounds( const idBounds& a, const idBounds& b );
	static float			SurfaceArea( const idBounds& bounds );
};

typedef struct trmCache_s
{
	idTraceModel			trm;
	int						refCount;
	float					volume;
	idVec3					centerOfMass;
	idMat3					inertiaTensor;
} trmCache_t;

idVec3 vec3_boxEpsilon( CM_BOX_EPSILON, CM_BOX_EPSILON, CM_BOX_EPSILON );

/*
===============
idClipBroadPhase::idClipBroadPhase
===============
*/
idClipBroadPhase::idClipBroadPhase()
{
	nodes.SetGranularity( 256 );
	root = -1;
	freeList = -1;
}

/*
===============
idClipBroadPhase::IsLeaf
===============
*/
bool idClipBroadPhase::IsLeaf( const clipBroadPhaseNode_t& node )
{
	return node.children[0] == -1;
}

/*
===============
idClipBroadPhase::ContainsBounds
===============
*/
bool idClipBroadPhase::ContainsBounds( const idBounds& outer, const idBounds& inner )
{
	return outer[0][0] <= inner[0][0] && outer[0][1] <= inner[0][1] && outer[0][2] <= inner[0][2] &&
		   outer[1][0] >= inner[1][0] && outer[1][1] >= inner[1][1] && outer[1][2] >= inner[1][2];
}

/*
===============
idClipBroadPhase::UnionBounds
===============
*/
idBounds idClipBroadPhase::UnionBounds( const idBounds& a, const idBounds& b )
{
	idBounds bounds = a;
	bounds.AddBounds( b );
	return bounds;
}

/*
===============
idClipBroadPhase::SurfaceArea
===============
*/
float idClipBroadPhase::SurfaceArea( const idBounds& bounds )
{
	const idVec3 size = bounds[1] - bounds[0];
	return 2.0f * ( size[0] * size[1] + size[1] * size[2] + size[2] * size[0] );
}

/*
===============
idClipBroadPhase::AllocNode
===============
*/
int idClipBroadPhase::AllocNode( void )
{
	int nodeNum;
	if( freeList != -1 )
	{
		nodeNum = freeList;
		freeList = nodes[nodeNum].next;
	}
	else
	{
		clipBroadPhaseNode_t node;
		memset( &node, 0, sizeof( node ) );
		nodeNum = nodes.Append( node );
	}

	clipBroadPhaseNode_t& node = nodes[nodeNum];
	node.clipModel = NULL;
	node.parent = -1;
	node.children[0] = node.children[1] = -1;
	node.height = 0;
	node.next = -1;
	return nodeNum;
}

/*
===============
idClipBroadPhase::FreeNode
===============
*/
void idClipBroadPhase::FreeNode( int nodeNum )
{
	clipBroadPhaseNode_t& node = nodes[nodeNum];
	node.clipModel = NULL;
	node.parent = -1;
	node.children[0] = node.children[1] = -1;
	node.height = -1;
	node.next = freeList;
	freeList = nodeNum;
}

/*
===============
idClipBroadPhase::Balance
===============
*/
int idClipBroadPhase::Balance( int nodeNum )
{
	clipBroadPhaseNode_t& a = nodes[nodeNum];
	if( IsLeaf( a ) || a.height < 2 )
	{
		return nodeNum;
	}

	const int bNum = a.children[0];
	const int cNum = a.children[1];
	clipBroadPhaseNode_t& b = nodes[bNum];
	clipBroadPhaseNode_t& c = nodes[cNum];
	const int balance = c.height - b.height;

	if( balance > 1 )
	{
		const int fNum = c.children[0];
		const int gNum = c.children[1];
		clipBroadPhaseNode_t& f = nodes[fNum];
		clipBroadPhaseNode_t& g = nodes[gNum];

		c.children[0] = nodeNum;
		c.parent = a.parent;
		a.parent = cNum;
		if( c.parent != -1 )
		{
			clipBroadPhaseNode_t& parent = nodes[c.parent];
			parent.children[parent.children[1] == nodeNum] = cNum;
		}
		else
		{
			root = cNum;
		}

		if( f.height > g.height )
		{
			c.children[1] = fNum;
			a.children[1] = gNum;
			g.parent = nodeNum;
			a.bounds = UnionBounds( b.bounds, g.bounds );
			c.bounds = UnionBounds( a.bounds, f.bounds );
			a.height = 1 + Max( b.height, g.height );
			c.height = 1 + Max( a.height, f.height );
		}
		else
		{
			c.children[1] = gNum;
			a.children[1] = fNum;
			f.parent = nodeNum;
			a.bounds = UnionBounds( b.bounds, f.bounds );
			c.bounds = UnionBounds( a.bounds, g.bounds );
			a.height = 1 + Max( b.height, f.height );
			c.height = 1 + Max( a.height, g.height );
		}
		return cNum;
	}

	if( balance < -1 )
	{
		const int dNum = b.children[0];
		const int eNum = b.children[1];
		clipBroadPhaseNode_t& d = nodes[dNum];
		clipBroadPhaseNode_t& e = nodes[eNum];

		b.children[0] = nodeNum;
		b.parent = a.parent;
		a.parent = bNum;
		if( b.parent != -1 )
		{
			clipBroadPhaseNode_t& parent = nodes[b.parent];
			parent.children[parent.children[1] == nodeNum] = bNum;
		}
		else
		{
			root = bNum;
		}

		if( d.height > e.height )
		{
			b.children[1] = dNum;
			a.children[0] = eNum;
			e.parent = nodeNum;
			a.bounds = UnionBounds( c.bounds, e.bounds );
			b.bounds = UnionBounds( a.bounds, d.bounds );
			a.height = 1 + Max( c.height, e.height );
			b.height = 1 + Max( a.height, d.height );
		}
		else
		{
			b.children[1] = eNum;
			a.children[0] = dNum;
			d.parent = nodeNum;
			a.bounds = UnionBounds( c.bounds, d.bounds );
			b.bounds = UnionBounds( a.bounds, e.bounds );
			a.height = 1 + Max( c.height, d.height );
			b.height = 1 + Max( a.height, e.height );
		}
		return bNum;
	}

	return nodeNum;
}

/*
===============
idClipBroadPhase::InsertLeaf
===============
*/
void idClipBroadPhase::InsertLeaf( int leaf )
{
	if( root == -1 )
	{
		root = leaf;
		nodes[root].parent = -1;
		return;
	}

	const idBounds leafBounds = nodes[leaf].bounds;
	int sibling = root;
	while( !IsLeaf( nodes[sibling] ) )
	{
		const int child0 = nodes[sibling].children[0];
		const int child1 = nodes[sibling].children[1];
		const float area = SurfaceArea( nodes[sibling].bounds );
		const idBounds combined = UnionBounds( nodes[sibling].bounds, leafBounds );
		const float combinedArea = SurfaceArea( combined );
		const float parentCost = 2.0f * combinedArea;
		const float inheritanceCost = 2.0f * ( combinedArea - area );

		const idBounds combined0 = UnionBounds( leafBounds, nodes[child0].bounds );
		const float cost0 = ( IsLeaf( nodes[child0] ) ? SurfaceArea( combined0 ) : SurfaceArea( combined0 ) - SurfaceArea( nodes[child0].bounds ) ) + inheritanceCost;
		const idBounds combined1 = UnionBounds( leafBounds, nodes[child1].bounds );
		const float cost1 = ( IsLeaf( nodes[child1] ) ? SurfaceArea( combined1 ) : SurfaceArea( combined1 ) - SurfaceArea( nodes[child1].bounds ) ) + inheritanceCost;

		if( parentCost < cost0 && parentCost < cost1 )
		{
			break;
		}
		sibling = cost0 < cost1 ? child0 : child1;
	}

	const int oldParent = nodes[sibling].parent;
	const int newParent = AllocNode();
	nodes[newParent].parent = oldParent;
	nodes[newParent].bounds = UnionBounds( leafBounds, nodes[sibling].bounds );
	nodes[newParent].height = nodes[sibling].height + 1;
	nodes[newParent].children[0] = sibling;
	nodes[newParent].children[1] = leaf;
	nodes[sibling].parent = newParent;
	nodes[leaf].parent = newParent;

	if( oldParent != -1 )
	{
		clipBroadPhaseNode_t& parent = nodes[oldParent];
		parent.children[parent.children[1] == sibling] = newParent;
	}
	else
	{
		root = newParent;
	}

	int index = nodes[leaf].parent;
	while( index != -1 )
	{
		index = Balance( index );
		clipBroadPhaseNode_t& node = nodes[index];
		const clipBroadPhaseNode_t& child0 = nodes[node.children[0]];
		const clipBroadPhaseNode_t& child1 = nodes[node.children[1]];
		node.height = 1 + Max( child0.height, child1.height );
		node.bounds = UnionBounds( child0.bounds, child1.bounds );
		index = node.parent;
	}
}

/*
===============
idClipBroadPhase::RemoveLeaf
===============
*/
void idClipBroadPhase::RemoveLeaf( int leaf )
{
	if( leaf == root )
	{
		root = -1;
		return;
	}

	const int parent = nodes[leaf].parent;
	const int grandParent = nodes[parent].parent;
	const int sibling = nodes[parent].children[nodes[parent].children[0] == leaf];
	if( grandParent != -1 )
	{
		clipBroadPhaseNode_t& grand = nodes[grandParent];
		grand.children[grand.children[1] == parent] = sibling;
		nodes[sibling].parent = grandParent;
		FreeNode( parent );

		int index = grandParent;
		while( index != -1 )
		{
			index = Balance( index );
			clipBroadPhaseNode_t& node = nodes[index];
			const clipBroadPhaseNode_t& child0 = nodes[node.children[0]];
			const clipBroadPhaseNode_t& child1 = nodes[node.children[1]];
			node.bounds = UnionBounds( child0.bounds, child1.bounds );
			node.height = 1 + Max( child0.height, child1.height );
			index = node.parent;
		}
	}
	else
	{
		root = sibling;
		nodes[sibling].parent = -1;
		FreeNode( parent );
	}
	nodes[leaf].parent = -1;
}

/*
===============
idClipBroadPhase::CreateProxy
===============
*/
int idClipBroadPhase::CreateProxy( const idBounds& bounds, idClipModel* clipModel )
{
	const int proxy = AllocNode();
	nodes[proxy].bounds = bounds.Expand( CLIP_BROADPHASE_MARGIN );
	nodes[proxy].clipModel = clipModel;
	nodes[proxy].height = 0;
	InsertLeaf( proxy );
	return proxy;
}

/*
===============
idClipBroadPhase::DestroyProxy
===============
*/
void idClipBroadPhase::DestroyProxy( int proxy )
{
	assert( proxy >= 0 && proxy < nodes.Num() && IsLeaf( nodes[proxy] ) );
	RemoveLeaf( proxy );
	FreeNode( proxy );
}

/*
===============
idClipBroadPhase::MoveProxy
===============
*/
void idClipBroadPhase::MoveProxy( int proxy, const idBounds& bounds, const idVec3& displacement )
{
	assert( proxy >= 0 && proxy < nodes.Num() && IsLeaf( nodes[proxy] ) );
	if( ContainsBounds( nodes[proxy].bounds, bounds ) )
	{
		return;
	}

	RemoveLeaf( proxy );
	idBounds fatBounds = bounds.Expand( CLIP_BROADPHASE_MARGIN );
	for( int i = 0; i < 3; i++ )
	{
		const float prediction = displacement[i] * CLIP_BROADPHASE_PREDICTION;
		if( prediction < 0.0f )
		{
			fatBounds[0][i] += prediction;
		}
		else
		{
			fatBounds[1][i] += prediction;
		}
	}
	nodes[proxy].bounds = fatBounds;
	InsertLeaf( proxy );
}

/*
===============
idClipBroadPhase::Query
===============
*/
int idClipBroadPhase::Query( const idBounds& bounds, int contentMask, idClipModel** list, int maxCount ) const
{
	if( root == -1 )
	{
		return 0;
	}

	int count = 0;
	int stack[256];
	int stackCount = 0;
	stack[stackCount++] = root;
	while( stackCount > 0 )
	{
		const int nodeNum = stack[--stackCount];
		const clipBroadPhaseNode_t& node = nodes[nodeNum];
		if( !node.bounds.IntersectsBounds( bounds ) )
		{
			continue;
		}
		if( IsLeaf( node ) )
		{
			idClipModel* clipModel = node.clipModel;
			if( !clipModel->IsEnabled() || !( clipModel->GetContents() & contentMask ) || !clipModel->GetAbsBounds().IntersectsBounds( bounds ) )
			{
				continue;
			}
			if( count >= maxCount )
			{
				gameLocal.Warning( "idClipBroadPhase::Query: max count" );
				return count;
			}
			list[count++] = clipModel;
		}
		else
		{
			assert( stackCount + 2 <= ( int )( sizeof( stack ) / sizeof( stack[0] ) ) );
			stack[stackCount++] = node.children[0];
			stack[stackCount++] = node.children[1];
		}
	}
	return count;
}


/*
===============================================================

	idClipModel trace model cache

===============================================================
*/

static idList<trmCache_s*>		traceModelCache;
static idList<trmCache_s*>		traceModelCache_Unsaved;
static idHashIndex				traceModelHash;
static idHashIndex				traceModelHash_Unsaved;
const static int				TRACE_MODEL_SAVED = BIT( 16 );

// SRS - statically define the default trace model used for the default clip model
const static idTraceModel		defaultTraceModel = idBounds( idVec3( 0, 0, 0 ) ).Expand( 8 );

/*
===============
idClipModel::ClearTraceModelCache
===============
*/
void idClipModel::ClearTraceModelCache()
{
	traceModelCache.DeleteContents( true );
	traceModelCache_Unsaved.DeleteContents( true );
	traceModelHash.Free();
	traceModelHash_Unsaved.Free();
}

/*
===============
idClipModel::TraceModelCacheSize
===============
*/
int idClipModel::TraceModelCacheSize()
{
	return traceModelCache.Num() * sizeof( idTraceModel );
}

/*
===============
idClipModel::AllocTraceModel
===============
*/
int idClipModel::AllocTraceModel( const idTraceModel& trm, bool persistantThroughSaves )
{
	int i, hashKey, traceModelIndex;
	trmCache_t* entry;

	hashKey = GetTraceModelHashKey( trm );

	if( persistantThroughSaves )
	{
		// Look Inside the saved list.
		for( i = traceModelHash.First( hashKey ); i >= 0; i = traceModelHash.Next( i ) )
		{
			if( traceModelCache[i]->trm == trm )
			{
				traceModelCache[i]->refCount++;
				int flagged_index = i | TRACE_MODEL_SAVED;
				return flagged_index;
			}
		}
	}
	else
	{

		// Look inside the unsaved list.
		for( i = traceModelHash_Unsaved.First( hashKey ); i >= 0; i = traceModelHash_Unsaved.Next( i ) )
		{
			if( traceModelCache_Unsaved[i]->trm == trm )
			{
				traceModelCache_Unsaved[i]->refCount++;
				return i;
			}
		}
	}


	entry = new( TAG_PHYSICS_CLIP ) trmCache_t;
	entry->trm = trm;
	entry->trm.GetMassProperties( 1.0f, entry->volume, entry->centerOfMass, entry->inertiaTensor );
	entry->refCount = 1;

	if( persistantThroughSaves )
	{
		traceModelIndex = traceModelCache.Append( entry );
		traceModelHash.Add( hashKey, traceModelIndex );

		// Set the saved bit.
		traceModelIndex |= TRACE_MODEL_SAVED;

	}
	else
	{
		traceModelIndex = traceModelCache_Unsaved.Append( entry );
		traceModelHash_Unsaved.Add( hashKey, traceModelIndex );

		// remove the saved bit
		traceModelIndex &= ~TRACE_MODEL_SAVED;

	}

	return traceModelIndex;
}

/*
===============
idClipModel::FreeTraceModel
===============
*/
void idClipModel::FreeTraceModel( int traceModelIndex )
{

	int realTraceModelIndex = traceModelIndex & ~TRACE_MODEL_SAVED;

	// Check which cache we are using.
	if( traceModelIndex & TRACE_MODEL_SAVED )
	{

		if( realTraceModelIndex < 0 || realTraceModelIndex >= traceModelCache.Num() || traceModelCache[realTraceModelIndex]->refCount <= 0 )
		{
			gameLocal.Warning( "idClipModel::FreeTraceModel: tried to free uncached trace model" );
			return;
		}
		traceModelCache[realTraceModelIndex]->refCount--;

	}
	else
	{

		if( realTraceModelIndex < 0 || realTraceModelIndex >= traceModelCache_Unsaved.Num() || traceModelCache_Unsaved[realTraceModelIndex]->refCount <= 0 )
		{
			gameLocal.Warning( "idClipModel::FreeTraceModel: tried to free uncached trace model" );
			return;
		}
		traceModelCache_Unsaved[realTraceModelIndex]->refCount--;

	}
}

/*
===============
idClipModel::GetCachedTraceModel
===============
*/
idTraceModel* idClipModel::GetCachedTraceModel( int traceModelIndex )
{
	int realTraceModelIndex = traceModelIndex & ~TRACE_MODEL_SAVED;

	if( traceModelIndex & TRACE_MODEL_SAVED )
	{
		return &traceModelCache[realTraceModelIndex]->trm;
	}
	else
	{
		return &traceModelCache_Unsaved[realTraceModelIndex]->trm;
	}
}

/*
===============
idClipModel::GetCachedTraceModel
===============
*/
trmCache_t* idClipModel::GetTraceModelEntry( int traceModelIndex )
{

	int realTraceModelIndex = traceModelIndex & ~TRACE_MODEL_SAVED;

	if( traceModelIndex & TRACE_MODEL_SAVED )
	{
		return traceModelCache[realTraceModelIndex];
	}
	else
	{
		return traceModelCache_Unsaved[realTraceModelIndex];
	}
}

/*
===============
idClipModel::GetTraceModelHashKey
===============
*/
int idClipModel::GetTraceModelHashKey( const idTraceModel& trm )
{
	const idVec3& v = trm.bounds[0];
	return ( trm.type << 8 ) ^ ( trm.numVerts << 4 ) ^ ( trm.numEdges << 2 ) ^ ( trm.numPolys << 0 ) ^ idMath::FloatHash( v.ToFloatPtr(), v.GetDimension() );
}

/*
===============
idClipModel::SaveTraceModels
===============
*/
void idClipModel::SaveTraceModels( idSaveGame* savefile )
{
	int i;

	savefile->WriteInt( traceModelCache.Num() );
	for( i = 0; i < traceModelCache.Num(); i++ )
	{
		trmCache_t* entry = traceModelCache[i];

		savefile->WriteTraceModel( entry->trm );
		savefile->WriteFloat( entry->volume );
		savefile->WriteVec3( entry->centerOfMass );
		savefile->WriteMat3( entry->inertiaTensor );
	}
}

/*
===============
idClipModel::RestoreTraceModels
===============
*/
void idClipModel::RestoreTraceModels( idRestoreGame* savefile )
{
	int i, num;

	ClearTraceModelCache();

	savefile->ReadInt( num );
	traceModelCache.SetNum( num );

	for( i = 0; i < num; i++ )
	{
		trmCache_t* entry = new( TAG_PHYSICS_CLIP ) trmCache_t;

		savefile->ReadTraceModel( entry->trm );

		savefile->ReadFloat( entry->volume );
		savefile->ReadVec3( entry->centerOfMass );
		savefile->ReadMat3( entry->inertiaTensor );
		entry->refCount = 0;

		traceModelCache[i] = entry;
		traceModelHash.Add( GetTraceModelHashKey( entry->trm ), i );
	}

	// SRS - find or allocate default trace model here since it's not referenced by objects
	gameLocal.clip.DefaultClipModel()->traceModelIndex = AllocTraceModel( defaultTraceModel );
}


/*
===============================================================

	idClipModel

===============================================================
*/

/*
================
idClipModel::LoadModel
================
*/
bool idClipModel::LoadModel( const char* name )
{
	renderModelHandle = -1;
	if( traceModelIndex != -1 )
	{
		FreeTraceModel( traceModelIndex );
		traceModelIndex = -1;
	}
	collisionModelHandle = collisionModelManager->LoadModel( name, false );
	if( collisionModelHandle )
	{
		collisionModelManager->GetModelBounds( collisionModelHandle, bounds );
		collisionModelManager->GetModelContents( collisionModelHandle, contents );
		return true;
	}
	else
	{
		bounds.Zero();
		return false;
	}
}

/*
================
idClipModel::LoadModel
================
*/
void idClipModel::LoadModel( const idTraceModel& trm, bool persistantThroughSave )
{
	collisionModelHandle = 0;
	renderModelHandle = -1;
	if( traceModelIndex != -1 )
	{
		FreeTraceModel( traceModelIndex );
	}
	traceModelIndex = AllocTraceModel( trm, persistantThroughSave );
	bounds = trm.bounds;
}

/*
================
idClipModel::LoadModel
================
*/
void idClipModel::LoadModel( const int renderModelHandle )
{
	collisionModelHandle = 0;
	this->renderModelHandle = renderModelHandle;
	if( renderModelHandle != -1 )
	{
		const renderEntity_t* renderEntity = gameRenderWorld->GetRenderEntity( renderModelHandle );
		if( renderEntity )
		{
			bounds = renderEntity->bounds;
		}
	}
	if( traceModelIndex != -1 )
	{
		FreeTraceModel( traceModelIndex );
		traceModelIndex = -1;
	}
}

/*
================
idClipModel::Init
================
*/
void idClipModel::Init()
{
	enabled = true;
	entity = NULL;
	id = 0;
	owner = NULL;
	origin.Zero();
	axis.Identity();
	bounds.Zero();
	absBounds.Zero();
	material = NULL;
	contents = CONTENTS_BODY;
	collisionModelHandle = 0;
	renderModelHandle = -1;
	traceModelIndex = -1;
	linkedClip = NULL;
	broadPhaseHandle = -1;
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel()
{
	Init();
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const char* name )
{
	Init();
	LoadModel( name );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idTraceModel& trm )
{
	Init();
	LoadModel( trm, true );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idTraceModel& trm, bool persistantThroughSave )
{
	Init();
	LoadModel( trm, persistantThroughSave );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const int renderModelHandle )
{
	Init();
	contents = CONTENTS_RENDERMODEL;
	LoadModel( renderModelHandle );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idClipModel* model )
{
	enabled = model->enabled;
	entity = model->entity;
	id = model->id;
	owner = model->owner;
	origin = model->origin;
	axis = model->axis;
	bounds = model->bounds;
	absBounds = model->absBounds;
	material = model->material;
	contents = model->contents;
	collisionModelHandle = model->collisionModelHandle;
	traceModelIndex = -1;
	if( model->traceModelIndex != -1 )
	{
		LoadModel( *GetCachedTraceModel( model->traceModelIndex ) );
	}
	renderModelHandle = model->renderModelHandle;
	linkedClip = NULL;
	broadPhaseHandle = -1;
}

/*
================
idClipModel::~idClipModel
================
*/
idClipModel::~idClipModel()
{
	// make sure the clip model is no longer linked
	Unlink();
	if( traceModelIndex != -1 )
	{
		FreeTraceModel( traceModelIndex );
	}
}

/*
================
idClipModel::Save
================
*/
void idClipModel::Save( idSaveGame* savefile ) const
{
	savefile->WriteBool( enabled );
	savefile->WriteObject( entity );
	savefile->WriteInt( id );
	savefile->WriteObject( owner );
	savefile->WriteVec3( origin );
	savefile->WriteMat3( axis );
	savefile->WriteBounds( bounds );
	savefile->WriteBounds( absBounds );
	savefile->WriteMaterial( material );
	savefile->WriteInt( contents );
	if( collisionModelHandle >= 0 )
	{
		savefile->WriteString( collisionModelManager->GetModelName( collisionModelHandle ) );
	}
	else
	{
		savefile->WriteString( "" );
	}
	savefile->WriteInt( traceModelIndex );
	savefile->WriteInt( renderModelHandle );
	savefile->WriteBool( IsLinked() );
}

/*
================
idClipModel::Restore
================
*/
void idClipModel::Restore( idRestoreGame* savefile )
{
	idStr collisionModelName;
	bool linked;

	savefile->ReadBool( enabled );
	savefile->ReadObject( reinterpret_cast<idClass*&>( entity ) );
	savefile->ReadInt( id );
	savefile->ReadObject( reinterpret_cast<idClass*&>( owner ) );
	savefile->ReadVec3( origin );
	savefile->ReadMat3( axis );
	savefile->ReadBounds( bounds );
	savefile->ReadBounds( absBounds );
	savefile->ReadMaterial( material );
	savefile->ReadInt( contents );
	savefile->ReadString( collisionModelName );
	if( collisionModelName.Length() )
	{
		collisionModelHandle = collisionModelManager->LoadModel( collisionModelName, false );
	}
	else
	{
		collisionModelHandle = -1;
	}
	savefile->ReadInt( traceModelIndex );
	if( traceModelIndex >= 0 )
	{
		int realIndex = traceModelIndex & ~TRACE_MODEL_SAVED;
		traceModelCache[realIndex]->refCount++;
	}
	savefile->ReadInt( renderModelHandle );
	savefile->ReadBool( linked );

	// the render model will be set when the clip model is linked
	renderModelHandle = -1;
	linkedClip = NULL;
	broadPhaseHandle = -1;

	if( linked )
	{
		Link( gameLocal.clip, entity, id, origin, axis, renderModelHandle );
	}
}

/*
================
idClipModel::SetPosition
================
*/
void idClipModel::SetPosition( const idVec3& newOrigin, const idMat3& newAxis )
{
	if( IsLinked() )
	{
		Unlink();	// unlink from old position
	}
	origin = newOrigin;
	axis = newAxis;
}

/*
================
idClipModel::Handle
================
*/
cmHandle_t idClipModel::Handle() const
{
	assert( renderModelHandle == -1 );
	if( collisionModelHandle )
	{
		return collisionModelHandle;
	}
	else if( traceModelIndex != -1 )
	{
		return collisionModelManager->SetupTrmModel( *GetCachedTraceModel( traceModelIndex ), material );
	}
	else
	{
		// this happens in multiplayer on the combat models
		gameLocal.Warning( "idClipModel::Handle: clip model %d on '%s' (%x) is not a collision or trace model", id, entity->name.c_str(), entity->entityNumber );
		return 0;
	}
}

/*
================
idClipModel::GetMassProperties
================
*/
void idClipModel::GetMassProperties( const float density, float& mass, idVec3& centerOfMass, idMat3& inertiaTensor ) const
{
	if( traceModelIndex == -1 )
	{
		gameLocal.Error( "idClipModel::GetMassProperties: clip model %d on '%s' is not a trace model\n", id, entity->name.c_str() );
	}

	trmCache_t* entry = GetTraceModelEntry( traceModelIndex ); //traceModelCache[traceModelIndex];
	mass = entry->volume * density;
	centerOfMass = entry->centerOfMass;
	inertiaTensor = density * entry->inertiaTensor;
}

/*
===============
idClipModel::Unlink
===============
*/
void idClipModel::Unlink()
{
	if( broadPhaseHandle != -1 )
	{
		assert( linkedClip != NULL && linkedClip->broadPhase != NULL );
		linkedClip->broadPhase->DestroyProxy( broadPhaseHandle );
		broadPhaseHandle = -1;
		linkedClip = NULL;
	}
}

/*
===============
idClipModel::Link
===============
*/
void idClipModel::Link( idClip& clp )
{
	idBounds oldAbsBounds = absBounds;
	const bool updateProxy = ( linkedClip == &clp && broadPhaseHandle != -1 );

	assert( idClipModel::entity );
	if( !idClipModel::entity )
	{
		return;
	}

	if( broadPhaseHandle != -1 && !updateProxy )
	{
		Unlink();
	}

	if( bounds.IsCleared() )
	{
		if( updateProxy )
		{
			Unlink();
		}
		return;
	}

	// set the abs box
	if( axis.IsRotated() )
	{
		// expand for rotation
		absBounds.FromTransformedBounds( bounds, origin, axis );
	}
	else
	{
		// normal
		absBounds[0] = bounds[0] + origin;
		absBounds[1] = bounds[1] + origin;
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	absBounds[0] -= vec3_boxEpsilon;
	absBounds[1] += vec3_boxEpsilon;

	if( updateProxy )
	{
		clp.broadPhase->MoveProxy( broadPhaseHandle, absBounds, absBounds.GetCenter() - oldAbsBounds.GetCenter() );
	}
	else
	{
		linkedClip = &clp;
		broadPhaseHandle = clp.broadPhase->CreateProxy( absBounds, this );
	}
}

/*
===============
idClipModel::Link
===============
*/
void idClipModel::Link( idClip& clp, idEntity* ent, int newId, const idVec3& newOrigin, const idMat3& newAxis, int renderModelHandle )
{

	this->entity = ent;
	this->id = newId;
	this->origin = newOrigin;
	this->axis = newAxis;
	if( renderModelHandle != -1 )
	{
		this->renderModelHandle = renderModelHandle;
		const renderEntity_t* renderEntity = gameRenderWorld->GetRenderEntity( renderModelHandle );
		if( renderEntity )
		{
			this->bounds = renderEntity->bounds;
		}
	}
	this->Link( clp );
}

/*
============
idClipModel::CheckModel
============
*/
cmHandle_t idClipModel::CheckModel( const char* name )
{
	return collisionModelManager->LoadModel( name, false );
}


/*
===============================================================

	idClip

===============================================================
*/

/*
===============
idClip::idClip
===============
*/
idClip::idClip()
{
	broadPhase = NULL;
	worldBounds.Zero();
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
===============
idClip::Init
===============
*/
void idClip::Init()
{
	cmHandle_t h;
	idVec3 size;

	delete broadPhase;
	broadPhase = new idClipBroadPhase;

	// get world map bounds
	h = collisionModelManager->LoadModel( "worldMap", false );
	collisionModelManager->GetModelBounds( h, worldBounds );

	size = worldBounds[1] - worldBounds[0];
	gameLocal.Printf( "map bounds are (%1.1f, %1.1f, %1.1f)\n", size[0], size[1], size[2] );
	gameLocal.Printf( "dynamic AABB tree broad phase initialized\n" );

	// initialize a default clip model
	defaultClipModel.LoadModel( defaultTraceModel );

	// set counters to zero
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
===============
idClip::Shutdown
===============
*/
void idClip::Shutdown()
{
	delete broadPhase;
	broadPhase = NULL;

	// free the trace model used for the temporaryClipModel
	if( temporaryClipModel.traceModelIndex != -1 )
	{
		idClipModel::FreeTraceModel( temporaryClipModel.traceModelIndex );
		temporaryClipModel.traceModelIndex = -1;
	}

	// free the trace model used for the defaultClipModel
	if( defaultClipModel.traceModelIndex != -1 )
	{
		idClipModel::FreeTraceModel( defaultClipModel.traceModelIndex );
		defaultClipModel.traceModelIndex = -1;
	}
}

/*
================
idClip::ClipModelsTouchingBounds
================
*/
int idClip::ClipModelsTouchingBounds( const idBounds& bounds, int contentMask, idClipModel** clipModelList, int maxCount ) const
{
	if(	bounds[0][0] > bounds[1][0] ||
			bounds[0][1] > bounds[1][1] ||
			bounds[0][2] > bounds[1][2] )
	{
		// we should not go through the tree for degenerate or backwards bounds
		assert( false );
		return 0;
	}

	idBounds queryBounds( bounds[0] - vec3_boxEpsilon, bounds[1] + vec3_boxEpsilon );
	return broadPhase ? broadPhase->Query( queryBounds, contentMask, clipModelList, maxCount ) : 0;
}

/*
================
idClip::EntitiesTouchingBounds
================
*/
int idClip::EntitiesTouchingBounds( const idBounds& bounds, int contentMask, idEntity** entityList, int maxCount ) const
{
	idClipModel* clipModelList[MAX_GENTITIES];
	int i, j, count, entCount;

	count = idClip::ClipModelsTouchingBounds( bounds, contentMask, clipModelList, MAX_GENTITIES );
	entCount = 0;
	for( i = 0; i < count; i++ )
	{
		// entity could already be in the list because an entity can use multiple clip models
		for( j = 0; j < entCount; j++ )
		{
			if( entityList[j] == clipModelList[i]->entity )
			{
				break;
			}
		}
		if( j >= entCount )
		{
			if( entCount >= maxCount )
			{
				gameLocal.Warning( "idClip::EntitiesTouchingBounds: max count" );
				return entCount;
			}
			entityList[entCount] = clipModelList[i]->entity;
			entCount++;
		}
	}

	return entCount;
}

/*
====================
idClip::GetTraceClipModels

  an ent will be excluded from testing if:
  cm->entity == passEntity ( don't clip against the pass entity )
  cm->entity == passOwner ( missiles don't clip with owner )
  cm->owner == passEntity ( don't interact with your own missiles )
  cm->owner == passOwner ( don't interact with other missiles from same owner )
====================
*/
int idClip::GetTraceClipModels( const idBounds& bounds, int contentMask, const idEntity* passEntity, idClipModel** clipModelList ) const
{
	int i, num;
	idClipModel*	cm;
	idEntity* passOwner;

	num = ClipModelsTouchingBounds( bounds, contentMask, clipModelList, MAX_GENTITIES );

	if( !passEntity )
	{
		return num;
	}

	if( passEntity->GetPhysics()->GetNumClipModels() > 0 )
	{
		passOwner = passEntity->GetPhysics()->GetClipModel()->GetOwner();
	}
	else
	{
		passOwner = NULL;
	}

	for( i = 0; i < num; i++ )
	{

		cm = clipModelList[i];

		// check if we should ignore this entity
		if( cm->entity == passEntity )
		{
			clipModelList[i] = NULL;			// don't clip against the pass entity
		}
		else if( cm->entity == passOwner )
		{
			clipModelList[i] = NULL;			// missiles don't clip with their owner
		}
		else if( cm->owner )
		{
			if( cm->owner == passEntity )
			{
				clipModelList[i] = NULL;		// don't clip against own missiles
			}
			else if( cm->owner == passOwner )
			{
				clipModelList[i] = NULL;		// don't clip against other missiles from same owner
			}
		}
	}

	return num;
}

/*
============
idClip::TraceRenderModel
============
*/
void idClip::TraceRenderModel( trace_t& trace, const idVec3& start, const idVec3& end, const float radius, const idMat3& axis, idClipModel* touch ) const
{
	trace.fraction = 1.0f;

	// if the trace is passing through the bounds
	if( touch->absBounds.Expand( radius ).LineIntersection( start, end ) )
	{
		modelTrace_t modelTrace;

		// test with exact render model and modify trace_t structure accordingly
		if( gameRenderWorld->ModelTrace( modelTrace, touch->renderModelHandle, start, end, radius ) )
		{
			trace.fraction = modelTrace.fraction;
			trace.endAxis = axis;
			trace.endpos = modelTrace.point;
			trace.c.normal = modelTrace.normal;
			trace.c.dist = modelTrace.point * modelTrace.normal;
			trace.c.point = modelTrace.point;
			trace.c.type = CONTACT_TRMVERTEX;
			trace.c.modelFeature = 0;
			trace.c.trmFeature = 0;
			trace.c.contents = modelTrace.material->GetContentFlags();
			trace.c.material = modelTrace.material;
			// NOTE: trace.c.id will be the joint number
			touch->id = JOINT_HANDLE_TO_CLIPMODEL_ID( modelTrace.jointNumber );
		}
	}
}

/*
============
idClip::TraceModelForClipModel
============
*/
const idTraceModel* idClip::TraceModelForClipModel( const idClipModel* mdl ) const
{
	if( !mdl )
	{
		return NULL;
	}
	else
	{
		if( !mdl->IsTraceModel() )
		{
			if( mdl->GetEntity() )
			{
				gameLocal.Error( "TraceModelForClipModel: clip model %d on '%s' is not a trace model\n", mdl->GetId(), mdl->GetEntity()->name.c_str() );
			}
			else
			{
				gameLocal.Error( "TraceModelForClipModel: clip model %d is not a trace model\n", mdl->GetId() );
			}
		}
		return idClipModel::GetCachedTraceModel( mdl->traceModelIndex );
	}
}

/*
============
idClip::TestHugeTranslation
============
*/
ID_INLINE bool TestHugeTranslation( trace_t& results, const idClipModel* mdl, const idVec3& start, const idVec3& end, const idMat3& trmAxis )
{
	if( mdl != NULL && ( end - start ).LengthSqr() > Square( CM_MAX_TRACE_DIST ) )
	{
		results.fraction = 0.0f;
		results.endpos = start;
		results.endAxis = trmAxis;
		memset( &results.c, 0, sizeof( results.c ) );
		results.c.point = start;

		if( mdl->GetEntity() )
		{
			gameLocal.Printf( "huge translation for clip model %d on entity %d '%s'\n", mdl->GetId(), mdl->GetEntity()->entityNumber, mdl->GetEntity()->GetName() );
		}
		else
		{
			gameLocal.Printf( "huge translation for clip model %d\n", mdl->GetId() );
		}
		gameLocal.Printf( "  from (%.2f %.2f %.2f) to (%.2f %.2f %.2f)\n", start.x, start.y, start.z, end.x, end.y, end.z );

#ifndef CTF
		// May be important: This occurs in CTF when a player connects and spawns
		// in the PVS of a player that has a flag that is spawning the idMoveableItem
		// "nuggets".  The error seems benign and the assert was getting in the way
		// of testing.
		assert( 0 );
#endif
		return true;
	}
	return false;
}

/*
============
idClip::TranslationEntities
============
*/
void idClip::TranslationEntities( trace_t& results, const idVec3& start, const idVec3& end,
								  const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, num;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	float radius;
	trace_t trace;
	const idTraceModel* trm;

	if( TestHugeTranslation( results, mdl, start, end, trmAxis ) )
	{
		return;
	}

	trm = TraceModelForClipModel( mdl );

	results.fraction = 1.0f;
	results.endpos = end;
	results.endAxis = trmAxis;

	if( !trm )
	{
		traceBounds.FromPointTranslation( start, end - start );
		radius = 0.0f;
	}
	else
	{
		traceBounds.FromBoundsTranslation( trm->bounds, start, trmAxis, end - start );
		radius = trm->bounds.GetRadius();
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

	for( i = 0; i < num; i++ )
	{
		touch = clipModelList[i];

		if( !touch )
		{
			continue;
		}

		if( touch->renderModelHandle != -1 )
		{
			idClip::numRenderModelTraces++;
			TraceRenderModel( trace, start, end, radius, trmAxis, touch );
		}
		else
		{
			idClip::numTranslations++;
			collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
												touch->Handle(), touch->origin, touch->axis );
		}

		if( trace.fraction < results.fraction )
		{
			results = trace;
			results.c.entityNum = touch->entity->entityNumber;
			results.c.id = touch->id;
			if( results.fraction == 0.0f )
			{
				break;
			}
		}
	}
}

/*
============
idClip::Translation
============
*/
bool idClip::Translation( trace_t& results, const idVec3& start, const idVec3& end,
						  const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, num;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	float radius;
	trace_t trace;
	const idTraceModel* trm;

	if( TestHugeTranslation( results, mdl, start, end, trmAxis ) )
	{
		return true;
	}

	trm = TraceModelForClipModel( mdl );

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// test world
		idClip::numTranslations++;
		collisionModelManager->Translation( &results, start, end, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
		results.c.entityNum = results.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
		if( results.fraction == 0.0f )
		{
			return true;		// blocked immediately by the world
		}
	}
	else
	{
		memset( &results, 0, sizeof( results ) );
		results.fraction = 1.0f;
		results.endpos = end;
		results.endAxis = trmAxis;
	}

	if( !trm )
	{
		traceBounds.FromPointTranslation( start, results.endpos - start );
		radius = 0.0f;
	}
	else
	{
		traceBounds.FromBoundsTranslation( trm->bounds, start, trmAxis, results.endpos - start );
		radius = trm->bounds.GetRadius();
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

	for( i = 0; i < num; i++ )
	{
		touch = clipModelList[i];

		if( !touch )
		{
			continue;
		}

		if( touch->renderModelHandle != -1 )
		{
			idClip::numRenderModelTraces++;
			TraceRenderModel( trace, start, end, radius, trmAxis, touch );
		}
		else
		{
			idClip::numTranslations++;
			collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
												touch->Handle(), touch->origin, touch->axis );
		}

		if( trace.fraction < results.fraction )
		{
			results = trace;
			results.c.entityNum = touch->entity->entityNumber;
			results.c.id = touch->id;
			if( results.fraction == 0.0f )
			{
				break;
			}
		}
	}

	return ( results.fraction < 1.0f );
}

/*
============
idClip::Rotation
============
*/
bool idClip::Rotation( trace_t& results, const idVec3& start, const idRotation& rotation,
					   const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, num;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	trace_t trace;
	const idTraceModel* trm;

	trm = TraceModelForClipModel( mdl );

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// test world
		idClip::numRotations++;
		collisionModelManager->Rotation( &results, start, rotation, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
		results.c.entityNum = results.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
		if( results.fraction == 0.0f )
		{
			return true;		// blocked immediately by the world
		}
	}
	else
	{
		memset( &results, 0, sizeof( results ) );
		results.fraction = 1.0f;
		results.endpos = start;
		results.endAxis = trmAxis * rotation.ToMat3();
	}

	if( !trm )
	{
		traceBounds.FromPointRotation( start, rotation );
	}
	else
	{
		traceBounds.FromBoundsRotation( trm->bounds, start, trmAxis, rotation );
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

	for( i = 0; i < num; i++ )
	{
		touch = clipModelList[i];

		if( !touch )
		{
			continue;
		}

		// no rotational collision with render models
		if( touch->renderModelHandle != -1 )
		{
			continue;
		}

		idClip::numRotations++;
		collisionModelManager->Rotation( &trace, start, rotation, trm, trmAxis, contentMask,
										 touch->Handle(), touch->origin, touch->axis );

		if( trace.fraction < results.fraction )
		{
			results = trace;
			results.c.entityNum = touch->entity->entityNumber;
			results.c.id = touch->id;
			if( results.fraction == 0.0f )
			{
				break;
			}
		}
	}

	return ( results.fraction < 1.0f );
}

/*
============
idClip::Motion
============
*/
bool idClip::Motion( trace_t& results, const idVec3& start, const idVec3& end, const idRotation& rotation,
					 const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, num;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idVec3 dir, endPosition;
	idBounds traceBounds;
	float radius;
	trace_t translationalTrace, rotationalTrace, trace;
	idRotation endRotation;
	const idTraceModel* trm;

	assert( rotation.GetOrigin() == start );

	if( TestHugeTranslation( results, mdl, start, end, trmAxis ) )
	{
		return true;
	}

	if( mdl != NULL && rotation.GetAngle() != 0.0f && rotation.GetVec() != vec3_origin )
	{
		// if no translation
		if( start == end )
		{
			// pure rotation
			return Rotation( results, start, rotation, mdl, trmAxis, contentMask, passEntity );
		}
	}
	else if( start != end )
	{
		// pure translation
		return Translation( results, start, end, mdl, trmAxis, contentMask, passEntity );
	}
	else
	{
		// no motion
		results.fraction = 1.0f;
		results.endpos = start;
		results.endAxis = trmAxis;
		return false;
	}

	trm = TraceModelForClipModel( mdl );

	radius = trm->bounds.GetRadius();

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// translational collision with world
		idClip::numTranslations++;
		collisionModelManager->Translation( &translationalTrace, start, end, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
		translationalTrace.c.entityNum = translationalTrace.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	}
	else
	{
		memset( &translationalTrace, 0, sizeof( translationalTrace ) );
		translationalTrace.fraction = 1.0f;
		translationalTrace.endpos = end;
		translationalTrace.endAxis = trmAxis;
	}

	if( translationalTrace.fraction != 0.0f )
	{

		traceBounds.FromBoundsRotation( trm->bounds, start, trmAxis, rotation );
		dir = translationalTrace.endpos - start;
		for( i = 0; i < 3; i++ )
		{
			if( dir[i] < 0.0f )
			{
				traceBounds[0][i] += dir[i];
			}
			else
			{
				traceBounds[1][i] += dir[i];
			}
		}

		num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

		for( i = 0; i < num; i++ )
		{
			touch = clipModelList[i];

			if( !touch )
			{
				continue;
			}

			if( touch->renderModelHandle != -1 )
			{
				idClip::numRenderModelTraces++;
				TraceRenderModel( trace, start, end, radius, trmAxis, touch );
			}
			else
			{
				idClip::numTranslations++;
				collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
													touch->Handle(), touch->origin, touch->axis );
			}

			if( trace.fraction < translationalTrace.fraction )
			{
				translationalTrace = trace;
				translationalTrace.c.entityNum = touch->entity->entityNumber;
				translationalTrace.c.id = touch->id;
				if( translationalTrace.fraction == 0.0f )
				{
					break;
				}
			}
		}
	}
	else
	{
		num = -1;
	}

	endPosition = translationalTrace.endpos;
	endRotation = rotation;
	endRotation.SetOrigin( endPosition );

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// rotational collision with world
		idClip::numRotations++;
		collisionModelManager->Rotation( &rotationalTrace, endPosition, endRotation, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
		rotationalTrace.c.entityNum = rotationalTrace.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	}
	else
	{
		memset( &rotationalTrace, 0, sizeof( rotationalTrace ) );
		rotationalTrace.fraction = 1.0f;
		rotationalTrace.endpos = endPosition;
		rotationalTrace.endAxis = trmAxis * rotation.ToMat3();
	}

	if( rotationalTrace.fraction != 0.0f )
	{

		if( num == -1 )
		{
			traceBounds.FromBoundsRotation( trm->bounds, endPosition, trmAxis, endRotation );
			num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );
		}

		for( i = 0; i < num; i++ )
		{
			touch = clipModelList[i];

			if( !touch )
			{
				continue;
			}

			// no rotational collision detection with render models
			if( touch->renderModelHandle != -1 )
			{
				continue;
			}

			idClip::numRotations++;
			collisionModelManager->Rotation( &trace, endPosition, endRotation, trm, trmAxis, contentMask,
											 touch->Handle(), touch->origin, touch->axis );

			if( trace.fraction < rotationalTrace.fraction )
			{
				rotationalTrace = trace;
				rotationalTrace.c.entityNum = touch->entity->entityNumber;
				rotationalTrace.c.id = touch->id;
				if( rotationalTrace.fraction == 0.0f )
				{
					break;
				}
			}
		}
	}

	if( rotationalTrace.fraction < 1.0f )
	{
		results = rotationalTrace;
	}
	else
	{
		results = translationalTrace;
		results.endAxis = rotationalTrace.endAxis;
	}

	results.fraction = Max( translationalTrace.fraction, rotationalTrace.fraction );

	return ( translationalTrace.fraction < 1.0f || rotationalTrace.fraction < 1.0f );
}

/*
============
idClip::Contacts
============
*/
int idClip::Contacts( contactInfo_t* contacts, const int maxContacts, const idVec3& start, const idVec6& dir, const float depth,
					  const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, j, num, n, numContacts;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	const idTraceModel* trm;

	trm = TraceModelForClipModel( mdl );

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// test world
		idClip::numContacts++;
		numContacts = collisionModelManager->Contacts( contacts, maxContacts, start, dir, depth, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
	}
	else
	{
		numContacts = 0;
	}

	for( i = 0; i < numContacts; i++ )
	{
		contacts[i].entityNum = ENTITYNUM_WORLD;
		contacts[i].id = 0;
	}

	if( numContacts >= maxContacts )
	{
		return numContacts;
	}

	if( !trm )
	{
		traceBounds = idBounds( start ).Expand( depth );
	}
	else
	{
		traceBounds.FromTransformedBounds( trm->bounds, start, trmAxis );
		traceBounds.ExpandSelf( depth );
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

	for( i = 0; i < num; i++ )
	{
		touch = clipModelList[i];

		if( !touch )
		{
			continue;
		}

		// no contacts with render models
		if( touch->renderModelHandle != -1 )
		{
			continue;
		}

		idClip::numContacts++;
		n = collisionModelManager->Contacts( contacts + numContacts, maxContacts - numContacts,
											 start, dir, depth, trm, trmAxis, contentMask,
											 touch->Handle(), touch->origin, touch->axis );

		for( j = 0; j < n; j++ )
		{
			contacts[numContacts].entityNum = touch->entity->entityNumber;
			contacts[numContacts].id = touch->id;
			numContacts++;
		}

		if( numContacts >= maxContacts )
		{
			break;
		}
	}

	return numContacts;
}

/*
============
idClip::Contents
============
*/
int idClip::Contents( const idVec3& start, const idClipModel* mdl, const idMat3& trmAxis, int contentMask, const idEntity* passEntity )
{
	int i, num, contents;
	idClipModel* touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	const idTraceModel* trm;

	trm = TraceModelForClipModel( mdl );

	if( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD )
	{
		// test world
		idClip::numContents++;
		contents = collisionModelManager->Contents( start, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
	}
	else
	{
		contents = 0;
	}

	if( !trm )
	{
		traceBounds[0] = start;
		traceBounds[1] = start;
	}
	else if( trmAxis.IsRotated() )
	{
		traceBounds.FromTransformedBounds( trm->bounds, start, trmAxis );
	}
	else
	{
		traceBounds[0] = trm->bounds[0] + start;
		traceBounds[1] = trm->bounds[1] + start;
	}

	num = GetTraceClipModels( traceBounds, -1, passEntity, clipModelList );

	for( i = 0; i < num; i++ )
	{
		touch = clipModelList[i];

		if( !touch )
		{
			continue;
		}

		// no contents test with render models
		if( touch->renderModelHandle != -1 )
		{
			continue;
		}

		// if the entity does not have any contents we are looking for
		if( ( touch->contents & contentMask ) == 0 )
		{
			continue;
		}

		// if the entity has no new contents flags
		if( ( touch->contents & contents ) == touch->contents )
		{
			continue;
		}

		idClip::numContents++;
		if( collisionModelManager->Contents( start, trm, trmAxis, contentMask, touch->Handle(), touch->origin, touch->axis ) )
		{
			contents |= ( touch->contents & contentMask );
		}
	}

	return contents;
}

/*
============
idClip::TranslationModel
============
*/
void idClip::TranslationModel( trace_t& results, const idVec3& start, const idVec3& end,
							   const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
							   cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{
	const idTraceModel* trm = TraceModelForClipModel( mdl );
	idClip::numTranslations++;
	collisionModelManager->Translation( &results, start, end, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::RotationModel
============
*/
void idClip::RotationModel( trace_t& results, const idVec3& start, const idRotation& rotation,
							const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
							cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{
	const idTraceModel* trm = TraceModelForClipModel( mdl );
	idClip::numRotations++;
	collisionModelManager->Rotation( &results, start, rotation, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::ContactsModel
============
*/
int idClip::ContactsModel( contactInfo_t* contacts, const int maxContacts, const idVec3& start, const idVec6& dir, const float depth,
						   const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
						   cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{
	const idTraceModel* trm = TraceModelForClipModel( mdl );
	idClip::numContacts++;
	return collisionModelManager->Contacts( contacts, maxContacts, start, dir, depth, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::ContentsModel
============
*/
int idClip::ContentsModel( const idVec3& start,
						   const idClipModel* mdl, const idMat3& trmAxis, int contentMask,
						   cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{
	const idTraceModel* trm = TraceModelForClipModel( mdl );
	idClip::numContents++;
	return collisionModelManager->Contents( start, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::GetModelContactFeature
============
*/
bool idClip::GetModelContactFeature( const contactInfo_t& contact, const idClipModel* clipModel, idFixedWinding& winding ) const
{
	int i;
	cmHandle_t handle;
	idVec3 start, end;

	handle = -1;
	winding.Clear();

	if( clipModel == NULL )
	{
		handle = 0;
	}
	else
	{
		if( clipModel->renderModelHandle != -1 )
		{
			winding += contact.point;
			return true;
		}
		else if( clipModel->traceModelIndex != -1 )
		{
			handle = collisionModelManager->SetupTrmModel( *idClipModel::GetCachedTraceModel( clipModel->traceModelIndex ), clipModel->material );
		}
		else
		{
			handle = clipModel->collisionModelHandle;
		}
	}

	// if contact with a collision model
	if( handle != -1 )
	{
		switch( contact.type )
		{
			case CONTACT_EDGE:
			{
				// the model contact feature is a collision model edge
				collisionModelManager->GetModelEdge( handle, contact.modelFeature, start, end );
				winding += start;
				winding += end;
				break;
			}
			case CONTACT_MODELVERTEX:
			{
				// the model contact feature is a collision model vertex
				collisionModelManager->GetModelVertex( handle, contact.modelFeature, start );
				winding += start;
				break;
			}
			case CONTACT_TRMVERTEX:
			{
				// the model contact feature is a collision model polygon
				collisionModelManager->GetModelPolygon( handle, contact.modelFeature, winding );
				break;
			}
		}
	}

	// transform the winding to world space
	if( clipModel )
	{
		for( i = 0; i < winding.GetNumPoints(); i++ )
		{
			winding[i].ToVec3() *= clipModel->axis;
			winding[i].ToVec3() += clipModel->origin;
		}
	}

	return true;
}

/*
============
idClip::PrintStatistics
============
*/
void idClip::PrintStatistics()
{
	gameLocal.Printf( "t = %-3d, r = %-3d, m = %-3d, render = %-3d, contents = %-3d, contacts = %-3d\n",
					  numTranslations, numRotations, numMotions, numRenderModelTraces, numContents, numContacts );
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
============
idClip::DrawClipModels
============
*/
void idClip::DrawClipModels( const idVec3& eye, const float radius, const idEntity* passEntity )
{
	int				i, num;
	idBounds		bounds;
	idClipModel*		clipModelList[MAX_GENTITIES];
	idClipModel*		clipModel;

	bounds = idBounds( eye ).Expand( radius );

	num = idClip::ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	for( i = 0; i < num; i++ )
	{
		clipModel = clipModelList[i];
		if( clipModel->GetEntity() == passEntity )
		{
			continue;
		}
		if( clipModel->renderModelHandle != -1 )
		{
			gameRenderWorld->DebugBounds( colorCyan, clipModel->GetAbsBounds() );
		}
		else
		{
			collisionModelManager->DrawModel( clipModel->Handle(), clipModel->GetOrigin(), clipModel->GetAxis(), eye, radius );
		}
	}
}

/*
============
idClip::DrawModelContactFeature
============
*/
bool idClip::DrawModelContactFeature( const contactInfo_t& contact, const idClipModel* clipModel, int lifetime ) const
{
	int i;
	idMat3 axis;
	idFixedWinding winding;

	if( !GetModelContactFeature( contact, clipModel, winding ) )
	{
		return false;
	}

	axis = contact.normal.ToMat3();

	if( winding.GetNumPoints() == 1 )
	{
		gameRenderWorld->DebugLine( colorCyan, winding[0].ToVec3(), winding[0].ToVec3() + 2.0f * axis[0], lifetime );
		gameRenderWorld->DebugLine( colorWhite, winding[0].ToVec3() - 1.0f * axis[1], winding[0].ToVec3() + 1.0f * axis[1], lifetime );
		gameRenderWorld->DebugLine( colorWhite, winding[0].ToVec3() - 1.0f * axis[2], winding[0].ToVec3() + 1.0f * axis[2], lifetime );
	}
	else
	{
		for( i = 0; i < winding.GetNumPoints(); i++ )
		{
			gameRenderWorld->DebugLine( colorCyan, winding[i].ToVec3(), winding[( i + 1 ) % winding.GetNumPoints()].ToVec3(), lifetime );
		}
	}

	axis[0] = -axis[0];
	axis[2] = -axis[2];
	gameRenderWorld->DrawText( contact.material->GetName(), winding.GetCenter() - 4.0f * axis[2], 0.1f, colorWhite, axis, 1, 5000 );

	return true;
}
