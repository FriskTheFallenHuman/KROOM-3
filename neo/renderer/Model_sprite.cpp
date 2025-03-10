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
#include "Model_local.h"


/*

A simple sprite model that always faces the view axis.

*/

static const char* sprite_SnapshotName = "_sprite_Snapshot_";

/*
===============
idRenderModelSprite::IsDynamicModel
===============
*/
dynamicModel_t idRenderModelSprite::IsDynamicModel() const
{
	return DM_CONTINUOUS;
}

/*
===============
idRenderModelSprite::IsLoaded
===============
*/
bool idRenderModelSprite::IsLoaded() const
{
	return true;
}

/*
===============
idRenderModelSprite::InstantiateDynamicModel
===============
*/
idRenderModel* 	idRenderModelSprite::InstantiateDynamicModel( const struct renderEntity_s* renderEntity, const viewDef_t* viewDef, idRenderModel* cachedModel )
{
	idRenderModelStatic* staticModel;
	srfTriangles_t* tri;
	modelSurface_t surf;

	if( cachedModel && !r_useCachedDynamicModels.GetBool() )
	{
		delete cachedModel;
		cachedModel = NULL;
	}

	if( renderEntity == NULL || viewDef == NULL )
	{
		delete cachedModel;
		return NULL;
	}

	if( cachedModel != NULL )
	{

		assert( dynamic_cast<idRenderModelStatic*>( cachedModel ) != NULL );
		assert( idStr::Icmp( cachedModel->Name(), sprite_SnapshotName ) == 0 );

		staticModel = static_cast<idRenderModelStatic*>( cachedModel );
		surf = *staticModel->Surface( 0 );
		tri = surf.geometry;

	}
	else
	{

		staticModel = new idRenderModelStatic;
		staticModel->InitEmpty( sprite_SnapshotName );

		tri = R_AllocStaticTriSurf();
		R_AllocStaticTriSurfVerts( tri, 4 );
		R_AllocStaticTriSurfIndexes( tri, 6 );

		tri->verts[ 0 ].Clear();
		tri->verts[ 0 ].SetNormal( 1.0f, 0.0f, 0.0f );
		tri->verts[ 0 ].SetTangent( 0.0f, 1.0f, 0.0f );
		tri->verts[ 0 ].SetBiTangent( 0.0f, 0.0f, 1.0f );
		tri->verts[ 0 ].SetTexCoord( 0.0f, 0.0f );

		tri->verts[ 1 ].Clear();
		tri->verts[ 1 ].SetNormal( 1.0f, 0.0f, 0.0f );
		tri->verts[ 1 ].SetTangent( 0.0f, 1.0f, 0.0f );
		tri->verts[ 1 ].SetBiTangent( 0.0f, 0.0f, 1.0f );
		tri->verts[ 1 ].SetTexCoord( 1.0f, 0.0f );

		tri->verts[ 2 ].Clear();
		tri->verts[ 2 ].SetNormal( 1.0f, 0.0f, 0.0f );
		tri->verts[ 2 ].SetTangent( 0.0f, 1.0f, 0.0f );
		tri->verts[ 2 ].SetBiTangent( 0.0f, 0.0f, 1.0f );
		tri->verts[ 2 ].SetTexCoord( 1.0f, 1.0f );

		tri->verts[ 3 ].Clear();
		tri->verts[ 3 ].SetNormal( 1.0f, 0.0f, 0.0f );
		tri->verts[ 3 ].SetTangent( 0.0f, 1.0f, 0.0f );
		tri->verts[ 3 ].SetBiTangent( 0.0f, 0.0f, 1.0f );
		tri->verts[ 3 ].SetTexCoord( 0.0f, 1.0f );

		tri->indexes[ 0 ] = 0;
		tri->indexes[ 1 ] = 1;
		tri->indexes[ 2 ] = 3;
		tri->indexes[ 3 ] = 1;
		tri->indexes[ 4 ] = 2;
		tri->indexes[ 5 ] = 3;

		tri->numVerts = 4;
		tri->numIndexes = 6;

		surf.geometry = tri;
		surf.id = 0;
		surf.shader = tr.defaultMaterial;
		staticModel->AddSurface( surf );
	}

	int	red			= idMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_RED ] * 255.0f );
	int green		= idMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_GREEN ] * 255.0f );
	int	blue		= idMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_BLUE ] * 255.0f );
	int	alpha		= idMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_ALPHA ] * 255.0f );

	idVec3 right	= idVec3( 0.0f, renderEntity->shaderParms[ SHADERPARM_SPRITE_WIDTH ] * 0.5f, 0.0f );
	idVec3 up		= idVec3( 0.0f, 0.0f, renderEntity->shaderParms[ SHADERPARM_SPRITE_HEIGHT ] * 0.5f );

	tri->verts[ 0 ].xyz = up + right;
	tri->verts[ 0 ].color[ 0 ] = red;
	tri->verts[ 0 ].color[ 1 ] = green;
	tri->verts[ 0 ].color[ 2 ] = blue;
	tri->verts[ 0 ].color[ 3 ] = alpha;

	tri->verts[ 1 ].xyz = up - right;
	tri->verts[ 1 ].color[ 0 ] = red;
	tri->verts[ 1 ].color[ 1 ] = green;
	tri->verts[ 1 ].color[ 2 ] = blue;
	tri->verts[ 1 ].color[ 3 ] = alpha;

	tri->verts[ 2 ].xyz = - right - up;
	tri->verts[ 2 ].color[ 0 ] = red;
	tri->verts[ 2 ].color[ 1 ] = green;
	tri->verts[ 2 ].color[ 2 ] = blue;
	tri->verts[ 2 ].color[ 3 ] = alpha;

	tri->verts[ 3 ].xyz = right - up;
	tri->verts[ 3 ].color[ 0 ] = red;
	tri->verts[ 3 ].color[ 1 ] = green;
	tri->verts[ 3 ].color[ 2 ] = blue;
	tri->verts[ 3 ].color[ 3 ] = alpha;

	R_BoundTriSurf( tri );

	staticModel->bounds = tri->bounds;

	return staticModel;
}

/*
===============
idRenderModelSprite::Bounds
===============
*/
idBounds idRenderModelSprite::Bounds( const struct renderEntity_s* renderEntity ) const
{
	idBounds b;

	b.Zero();
	if( renderEntity == NULL )
	{
		b.ExpandSelf( 8.0f );
	}
	else
	{
		b.ExpandSelf( Max( renderEntity->shaderParms[ SHADERPARM_SPRITE_WIDTH ], renderEntity->shaderParms[ SHADERPARM_SPRITE_HEIGHT ] ) * 0.5f );
	}
	return b;
}
