/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "RenderCommon.h"

idVertexCache vertexCache;

idCVar r_showVertexCache( "r_showVertexCache", "0", CVAR_RENDERER | CVAR_BOOL, "Print stats about the vertex cache every frame" );
idCVar r_showVertexCacheTimings( "r_showVertexCacheTimings", "0", CVAR_RENDERER | CVAR_BOOL, "Print stats about the vertex cache every frame" );


/*
==============
ClearGeoBufferSet
==============
*/
static void ClearGeoBufferSet( geoBufferSet_t& gbs )
{
	gbs.indexMemUsed.SetValue( 0 );
	gbs.vertexMemUsed.SetValue( 0 );
	gbs.jointMemUsed.SetValue( 0 );
	gbs.allocations = 0;
}

/*
==============
MapGeoBufferSet
==============
*/
static void MapGeoBufferSet( geoBufferSet_t& gbs )
{
	if( gbs.mappedVertexBase == NULL )
	{
		gbs.mappedVertexBase = ( byte* )gbs.vertexBuffer.MapBuffer( BM_WRITE );
	}

	if( gbs.mappedIndexBase == NULL )
	{
		gbs.mappedIndexBase = ( byte* )gbs.indexBuffer.MapBuffer( BM_WRITE );
	}

	if( gbs.mappedJointBase == NULL && gbs.jointBuffer.GetAllocedSize() != 0 )
	{
		gbs.mappedJointBase = ( byte* )gbs.jointBuffer.MapBuffer( BM_WRITE );
	}
}

/*
==============
UnmapGeoBufferSet
==============
*/
static void UnmapGeoBufferSet( geoBufferSet_t& gbs )
{
	if( gbs.mappedVertexBase != NULL )
	{
		gbs.vertexBuffer.UnmapBuffer();
		gbs.mappedVertexBase = NULL;
	}

	if( gbs.mappedIndexBase != NULL )
	{
		gbs.indexBuffer.UnmapBuffer();
		gbs.mappedIndexBase = NULL;
	}

	if( gbs.mappedJointBase != NULL )
	{
		gbs.jointBuffer.UnmapBuffer();
		gbs.mappedJointBase = NULL;
	}
}

/*
==============
AllocGeoBufferSet
==============
*/
static void AllocGeoBufferSet( geoBufferSet_t& gbs, const int vertexBytes, const int indexBytes, const int jointBytes, bufferUsageType_t usage )
{
	gbs.vertexBuffer.AllocBufferObject( NULL, vertexBytes, usage );
	gbs.indexBuffer.AllocBufferObject( NULL, indexBytes, usage );
	if( jointBytes > 0 )
	{
		gbs.jointBuffer.AllocBufferObject( NULL, jointBytes, usage );
	}

	ClearGeoBufferSet( gbs );
}

/*
==============
idVertexCache::Init
==============
*/
void idVertexCache::Init( int _uniformBufferOffsetAlignment )
{
	currentFrame = 0;
	listNum = 0;

	uniformBufferOffsetAlignment = _uniformBufferOffsetAlignment;

	mostUsedVertex = 0;
	mostUsedIndex = 0;
	mostUsedJoint = 0;

	for( int i = 0; i < NUM_FRAME_DATA; i++ )
	{
		AllocGeoBufferSet( frameData[i], VERTCACHE_VERTEX_MEMORY_PER_FRAME, VERTCACHE_INDEX_MEMORY_PER_FRAME, VERTCACHE_JOINT_MEMORY_PER_FRAME, BU_DYNAMIC );
	}
#if 1
	AllocGeoBufferSet( staticData, STATIC_VERTEX_MEMORY, STATIC_INDEX_MEMORY, 0, BU_STATIC );
#else
	AllocGeoBufferSet( staticData, STATIC_VERTEX_MEMORY, STATIC_INDEX_MEMORY, 0, BU_DYNAMIC );
#endif

	MapGeoBufferSet( frameData[ listNum ] );
}

/*
==============
idVertexCache::Shutdown
==============
*/
void idVertexCache::Shutdown()
{
	for( int i = 0; i < NUM_FRAME_DATA; i++ )
	{
		frameData[i].vertexBuffer.FreeBufferObject();
		frameData[i].indexBuffer.FreeBufferObject();
		frameData[i].jointBuffer.FreeBufferObject();
	}

	// SRS - free static buffers to avoid Vulkan validation layer errors on shutdown
	staticData.vertexBuffer.FreeBufferObject();
	staticData.indexBuffer.FreeBufferObject();
	staticData.jointBuffer.FreeBufferObject();
}

/*
==============
idVertexCache::PurgeAll
==============
*/
void idVertexCache::PurgeAll()
{
	Shutdown();
	Init( uniformBufferOffsetAlignment );
}

/*
==============
idVertexCache::FreeStaticData

call on loading a new map
==============
*/
void idVertexCache::FreeStaticData()
{
	ClearGeoBufferSet( staticData );
	mostUsedVertex = 0;
	mostUsedIndex = 0;
	mostUsedJoint = 0;
}

/*
==============
idVertexCache::ActuallyAlloc
==============
*/
vertCacheHandle_t idVertexCache::ActuallyAlloc( geoBufferSet_t& vcs, const void* data, int bytes, cacheType_t type )
{
	if( bytes == 0 )
	{
		return ( vertCacheHandle_t )0;
	}

	// RB: changed UINT_PTR to uintptr_t
	assert( ( ( ( uintptr_t )( data ) ) & 15 ) == 0 );
	// RB end

	assert( ( bytes & 15 ) == 0 );

	int	endPos = 0;
	int offset = 0;

	switch( type )
	{
		case CACHE_INDEX:
		{
			endPos = vcs.indexMemUsed.Add( bytes );
			if( endPos > vcs.indexBuffer.GetAllocedSize() )
			{
				idLib::Error( "Out of index cache" );
			}

			offset = endPos - bytes;

			if( data != NULL )
			{
				if( vcs.indexBuffer.GetUsage() == BU_DYNAMIC )
				{
					MapGeoBufferSet( vcs );
				}
				vcs.indexBuffer.Update( data, bytes, offset );
			}

			break;
		}
		case CACHE_VERTEX:
		{
			endPos = vcs.vertexMemUsed.Add( bytes );
			if( endPos > vcs.vertexBuffer.GetAllocedSize() )
			{
				idLib::Error( "Out of vertex cache" );
			}

			offset = endPos - bytes;

			if( data != NULL )
			{
				if( vcs.vertexBuffer.GetUsage() == BU_DYNAMIC )
				{
					MapGeoBufferSet( vcs );
				}
				vcs.vertexBuffer.Update( data, bytes, offset );
			}

			break;
		}
		case CACHE_JOINT:
		{
			endPos = vcs.jointMemUsed.Add( bytes );
			if( endPos > vcs.jointBuffer.GetAllocedSize() )
			{
				idLib::Error( "Out of joint buffer cache" );
			}

			offset = endPos - bytes;

			if( data != NULL )
			{
				if( vcs.jointBuffer.GetUsage() == BU_DYNAMIC )
				{
					MapGeoBufferSet( vcs );
				}
				vcs.jointBuffer.Update( data, bytes, offset );
			}

			break;
		}
		default:
			assert( false );
	}

	vcs.allocations++;

	vertCacheHandle_t handle =	( ( uint64_t )( currentFrame & VERTCACHE_FRAME_MASK ) << VERTCACHE_FRAME_SHIFT ) |
								( ( uint64_t )( offset & VERTCACHE_OFFSET_MASK ) << VERTCACHE_OFFSET_SHIFT ) |
								( ( uint64_t )( bytes & VERTCACHE_SIZE_MASK ) << VERTCACHE_SIZE_SHIFT );
	if( &vcs == &staticData )
	{
		handle |= VERTCACHE_STATIC;
	}
	return handle;
}

/*
==============
idVertexCache::AllocVertex
==============
*/
vertCacheHandle_t idVertexCache::AllocVertex( const void* data, int num, size_t size /*= sizeof( idDrawVert ) */ )
{
	return ActuallyAlloc( frameData[ listNum ], data, ALIGN( num * size, VERTEX_CACHE_ALIGN ), CACHE_VERTEX );
}

/*
==============
idVertexCache::AllocIndex
==============
*/
vertCacheHandle_t idVertexCache::AllocIndex( const void* data, int num, size_t size /*= sizeof( triIndex_t ) */ )
{
	return ActuallyAlloc( frameData[ listNum ], data, ALIGN( num * size, INDEX_CACHE_ALIGN ), CACHE_INDEX );
}

/*
==============
idVertexCache::AllocJoint
==============
*/
vertCacheHandle_t idVertexCache::AllocJoint( const void* data, int num, size_t size /*= sizeof( idJointMat ) */ )
{
	return ActuallyAlloc( frameData[ listNum ], data, ALIGN( num * size, uniformBufferOffsetAlignment ), CACHE_JOINT );
}

/*
==============
idVertexCache::AllocStaticVertex
==============
*/
vertCacheHandle_t idVertexCache::AllocStaticVertex( const void* data, int bytes )
{
	if( staticData.vertexMemUsed.GetValue() + bytes > STATIC_VERTEX_MEMORY )
	{
		idLib::FatalError( "AllocStaticVertex failed, increase STATIC_VERTEX_MEMORY" );
	}
	return ActuallyAlloc( staticData, data, bytes, CACHE_VERTEX );
}

/*
==============
idVertexCache::AllocStaticIndex
==============
*/
vertCacheHandle_t idVertexCache::AllocStaticIndex( const void* data, int bytes )
{
	if( staticData.indexMemUsed.GetValue() + bytes > STATIC_INDEX_MEMORY )
	{
		idLib::FatalError( "AllocStaticIndex failed, increase STATIC_INDEX_MEMORY" );
	}
	return ActuallyAlloc( staticData, data, bytes, CACHE_INDEX );
}

/*
==============
idVertexCache::MappedVertexBuffer
==============
*/
byte* idVertexCache::MappedVertexBuffer( vertCacheHandle_t handle )
{
	release_assert( !CacheIsStatic( handle ) );
	const uint64_t offset = ( int )( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64_t frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	release_assert( frameNum == ( currentFrame & VERTCACHE_FRAME_MASK ) );
	return frameData[ listNum ].mappedVertexBase + offset;
}

/*
==============
idVertexCache::MappedIndexBuffer
==============
*/
byte* idVertexCache::MappedIndexBuffer( vertCacheHandle_t handle )
{
	release_assert( !CacheIsStatic( handle ) );
	const uint64_t offset = ( int )( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64_t frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	release_assert( frameNum == ( currentFrame & VERTCACHE_FRAME_MASK ) );
	return frameData[ listNum ].mappedIndexBase + offset;
}

/*
==============
idVertexCache::CacheIsCurrent
==============
*/
bool idVertexCache::CacheIsCurrent( const vertCacheHandle_t handle )
{
	const int isStatic = handle & VERTCACHE_STATIC;
	if( isStatic )
	{
		return true;
	}
	const uint64_t frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	if( frameNum != ( currentFrame & VERTCACHE_FRAME_MASK ) )
	{
		return false;
	}
	return true;
}

/*
==============
idVertexCache::GetVertexBuffer
==============
*/
bool idVertexCache::GetVertexBuffer( vertCacheHandle_t handle, idVertexBuffer* vb )
{
	const int isStatic = handle & VERTCACHE_STATIC;
	const uint64_t size = ( int )( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const uint64_t offset = ( int )( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64_t frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	if( isStatic )
	{
		vb->Reference( staticData.vertexBuffer, offset, size );
		return true;
	}
	if( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
	{
		return false;
	}
	vb->Reference( frameData[ drawListNum ].vertexBuffer, offset, size );
	return true;
}

/*
==============
idVertexCache::GetIndexBuffer
==============
*/
bool idVertexCache::GetIndexBuffer( vertCacheHandle_t handle, idIndexBuffer* ib )
{
	const int isStatic = handle & VERTCACHE_STATIC;
	const uint64_t size = ( int )( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const uint64_t offset = ( int )( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64_t frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	if( isStatic )
	{
		ib->Reference( staticData.indexBuffer, offset, size );
		return true;
	}
	if( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
	{
		return false;
	}
	ib->Reference( frameData[ drawListNum ].indexBuffer, offset, size );
	return true;
}

/*
==============
idVertexCache::GetJointBuffer
==============
*/
bool idVertexCache::GetJointBuffer( vertCacheHandle_t handle, idUniformBuffer* jb )
{
	const int isStatic = handle & VERTCACHE_STATIC;
	const uint64_t numBytes = ( int )( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const uint64_t jointOffset = ( int )( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64_t frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	if( isStatic )
	{
		jb->Reference( staticData.jointBuffer, jointOffset, numBytes );
		return true;
	}
	if( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
	{
		return false;
	}
	jb->Reference( frameData[ drawListNum ].jointBuffer, jointOffset, numBytes );
	return true;
}

/*
==============
idVertexCache::BeginBackEnd
==============
*/
void idVertexCache::BeginBackEnd()
{
	mostUsedVertex = Max( mostUsedVertex, frameData[ listNum ].vertexMemUsed.GetValue() );
	mostUsedIndex = Max( mostUsedIndex, frameData[ listNum ].indexMemUsed.GetValue() );
	mostUsedJoint = Max( mostUsedJoint, frameData[ listNum ].jointMemUsed.GetValue() );

	if( r_showVertexCache.GetBool() )
	{
		idLib::Printf( "%08d: %d allocations, %dkB vertex, %dkB index, %ikB joint : %dkB vertex, %dkB index, %ikB joint\n",
					   currentFrame, frameData[ listNum ].allocations,
					   frameData[ listNum ].vertexMemUsed.GetValue() / 1024,
					   frameData[ listNum ].indexMemUsed.GetValue() / 1024,
					   frameData[ listNum ].jointMemUsed.GetValue() / 1024,
					   mostUsedVertex / 1024,
					   mostUsedIndex / 1024,
					   mostUsedJoint / 1024 );
	}

	// unmap the current frame so the GPU can read it
	const int startUnmap = Sys_Milliseconds();
	UnmapGeoBufferSet( frameData[ listNum ] );
	UnmapGeoBufferSet( staticData );
	const int endUnmap = Sys_Milliseconds();
	if( endUnmap - startUnmap > 1 )
	{
		idLib::PrintfIf( r_showVertexCacheTimings.GetBool(), "idVertexCache::unmap took %i msec\n", endUnmap - startUnmap );
	}
	drawListNum = listNum;

	// prepare the next frame for writing to by the CPU
	currentFrame++;

	listNum = currentFrame % NUM_FRAME_DATA;
	const int startMap = Sys_Milliseconds();
	MapGeoBufferSet( frameData[ listNum ] );
	const int endMap = Sys_Milliseconds();
	if( endMap - startMap > 1 )
	{
		idLib::PrintfIf( r_showVertexCacheTimings.GetBool(), "idVertexCache::map took %i msec\n", endMap - startMap );
	}

	ClearGeoBufferSet( frameData[ listNum ] );
}

