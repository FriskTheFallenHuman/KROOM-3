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

#ifndef __RENDERINTERNAL_H__
#define __RENDERINTERNAL_H__

/*
================================================================================================

renderInternal_t

================================================================================================
*/

#include "RenderCommon.h"

class CommandlineProgressBar;

#define RENDER_INTERNAL_FUNCS( X ) \
	/* tr_trisurf.cpp */ \
	X( srfTriangles_t*,	R_AllocStaticTriSurf,				( ),																( ) ) \
	X( void,			R_AllocStaticTriSurfVerts,			( srfTriangles_t* tri, int numVerts ),								( tri, numVerts ) ) \
	X( void,			R_AllocStaticTriSurfIndexes,		( srfTriangles_t* tri, int numIndexes ),							( tri, numIndexes ) ) \
	X( void,			R_FreeStaticTriSurfSilIndexes,		( srfTriangles_t* tri ),											( tri ) ) \
	X( void,			R_FreeStaticTriSurf,				( srfTriangles_t* tri ),											( tri ) ) \
	X( void,			R_CreateSilIndexes,					( srfTriangles_t* tri ),											( tri ) ) \
	X( void,			R_RemoveDegenerateTriangles,		( srfTriangles_t* tri ),											( tri ) ) \
	X( void,			R_RangeCheckIndexes,				( const srfTriangles_t* tri ),										( tri ) ) \
	/* lights / derived data (dmap) */ \
	X( void,			R_RenderLightFrustum,				( const renderLight_t& renderLight, idPlane lightFrustum[6] ),		( renderLight, lightFrustum ) ) \
	X( void,			R_DeriveLightData,					( idRenderLightLocal* light ),										( light ) ) \
	X( void,			R_FreeLightDefDerivedData,			( idRenderLightLocal* light ),										( light ) ) \
	X( void,			R_FreeDerivedData,					( ),																( ) ) \
	/* editors */ \
	X( void,			R_SetupViewMatrix,					( viewDef_t* viewDef ),												( viewDef ) ) \
	X( void,			R_SetupProjectionMatrix,			( viewDef_t* viewDef ),												( viewDef ) ) \
	/* images */ \
	X( void,			R_LoadImage,						( const char* name, byte** pic, int* width, int* height, ID_TIME_T* timestamp, bool makePowerOf2, textureUsage_t* usage ), \
															( name, pic, width, height, timestamp, makePowerOf2, usage ) ) \
	X( void,			Image_Preload,						( const idPreloadManifest& manifest, bool mapPreload ),				( manifest, mapPreload ) ) \
	X( void,			Image_Purge,						( idImage* image ),													( image ) ) \
	/* gui / imgui */ \
	X( void,			ImGui_RenderDrawLists,				( ImDrawData* drawData ),											( drawData ) ) \
	X( void,			GuiModel_Clear,						( ),																( ) ) \
	X( void,			GuiModel_EmitFullScreen,			( ),																( ) ) \
	X( idDrawVert*,		GuiModel_AllocTris,					( int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material, const uint64 glState ), \
															( numVerts, indexes, numIndexes, material, glState ) ) \
	/* renderer memory */ \
	X( void,			R_StaticFree,						( void* data ),														( data ) ) \
	/* light defs for dmap (idRenderLightLocal has virtuals: it has to be constructed by the module) */ \
	X( idRenderLightLocal*,	R_AllocLightDef,				( ),																( ) ) \
	X( void,			R_FreeLightDef,						( idRenderLightLocal* light ),										( light ) ) \
	/* idMaterial / idImage / Framebuffer members the executable calls (stubs in framework/RenderLoader.cpp) */ \
	X( int,				Material_CinematicLength,			( const idMaterial* material ),										( material ) ) \
	X( void,			Material_ResetCinematicTime,		( const idMaterial* material, int time ),							( material, time ) ) \
	X( int,				Material_GetCinematicStartTime,		( const idMaterial* material ),										( material ) ) \
	X( bool,			Material_CinematicIsPlaying,		( const idMaterial* material ),										( material ) ) \
	X( idImage*,		Material_GetEditorImage,			( const idMaterial* material ),										( material ) ) \
	X( bool,			Material_Save,						( idMaterial* material, const char* fileName ),						( material, fileName ) ) \
	X( bool,			Image_IsLoaded,						( const idImage* image ),											( image ) ) \
	X( void,			Image_ActuallyLoadImage,			( idImage* image, bool fromBackEnd ),								( image, fromBackEnd ) ) \
	X( bool,			Render_EditorTargetBegin,			( int width, int height, const float* clearColor ),				( width, height, clearColor ) ) \
	X( bool,			Render_EditorTargetEnd,				( unsigned char* rgba ),											( rgba ) ) \
	X( void,			CmdlineProgress_Start,				( class CommandlineProgressBar* bar ),									( bar ) ) \
	X( void,			CmdlineProgress_Increment,			( class CommandlineProgressBar* bar, bool updateScreen ),					( bar, updateScreen ) ) \
	/* editor access to renderer state */ \
	X( float,			Render_GetFrameShaderTime,			( ),																( ) ) \
	X( void,			Render_SetFrameShaderTime,			( float time ),														( time ) ) \
	X( void,			Render_SetEditorRenderCrop,			( int x1, int y1, int x2, int y2 ),									( x1, y1, x2, y2 ) ) \
	/* stats */ \
	X( void,			VertexCache_GetStaticMemKB,			( int* vertexKB, int* indexKB ),									( vertexKB, indexKB ) ) \
	/* tool GL contexts (OpenGL module only, no-ops elsewhere) */ \
	X( void,			RenderContext_MakeCurrent,			( ),																( ) ) \
	X( void,			RenderContext_Disable,				( ),																( ) ) \
	/* copy of the renderer's glConfig for exe-side readers (About dialog, ...) */ \
	X( void,			Render_GetGLConfig,					( glconfig_t* out ),												( out ) )

#define RENDER_INTERNAL_DECLARE_FUNC( ret, name, params, args )	ret name params;
RENDER_INTERNAL_FUNCS( RENDER_INTERNAL_DECLARE_FUNC )
#undef RENDER_INTERNAL_DECLARE_FUNC

struct renderInternal_t
{
#define RENDER_INTERNAL_DECLARE_PTR( ret, name, params, args )	ret ( *name ) params;
	RENDER_INTERNAL_FUNCS( RENDER_INTERNAL_DECLARE_PTR )
#undef RENDER_INTERNAL_DECLARE_PTR
};

extern renderInternal_t* renderInternal;

#endif /* !__RENDERINTERNAL_H__ */
