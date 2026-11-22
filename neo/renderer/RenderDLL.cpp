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

#include "RenderCommon.h"
#include "GLMatrix.h"
#include "RenderAPI.h"
#include "RenderInternal.h"
#include "CmdlineProgressbar.h"

idSys* 						sys = NULL;
idCommon* 					common = NULL;
idCmdSystem* 				cmdSystem = NULL;
idCVarSystem* 				cvarSystem = NULL;
idFileSystem* 				fileSystem = NULL;
idSoundSystem* 				soundSystem = NULL;
idUserInterfaceManager* 	uiManager = NULL;
idDeclManager* 				declManager = NULL;
idImGuiSystem*				imguiSystem = NULL;
idCVar* 					idCVar::staticVars = NULL;

idRenderHost*				renderHost = NULL;
renderHostCVars_t			renderHostCVars;

static renderExport_t		renderExport;
static renderInternal_t		renderInternalTable;

unsigned int Sys_Milliseconds()
{
	return renderHost->Milliseconds();
}

uint64 Sys_Microseconds()
{
	return renderHost->Microseconds();
}

void Image_Preload( const idPreloadManifest& manifest, bool mapPreload )
{
	globalImages->Preload( manifest, mapPreload );
}

void Image_Purge( idImage* image )
{
	image->PurgeImage();
}

void ImGui_RenderDrawLists( ImDrawData* drawData )
{
	idRenderBackend::ImGui_RenderDrawLists( drawData );
}

void GuiModel_Clear()
{
	tr.guiModel->Clear();
}

void GuiModel_EmitFullScreen()
{
	tr.guiModel->EmitFullScreen();
}

idDrawVert* GuiModel_AllocTris( int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material, const uint64 glState )
{
	return tr.guiModel->AllocTris( numVerts, indexes, numIndexes, material, glState );
}

void VertexCache_GetStaticMemKB( int* vertexKB, int* indexKB )
{
	*vertexKB = vertexCache.staticData.vertexMemUsed.GetValue() / 1024;
	*indexKB = vertexCache.staticData.indexMemUsed.GetValue() / 1024;
}

void RenderContext_MakeCurrent()
{
	R_API_MakeContextCurrent();
}

void RenderContext_Disable()
{
	R_API_DisableContext();
}

void Render_GetGLConfig( glconfig_t* out )
{
	*out = glConfig;
}

idRenderLightLocal* R_AllocLightDef()
{
	return new idRenderLightLocal;
}

void R_FreeLightDef( idRenderLightLocal* light )
{
	delete light;
}

int Material_CinematicLength( const idMaterial* material )
{
	return material->CinematicLength();
}

void Material_ResetCinematicTime( const idMaterial* material, int time )
{
	material->ResetCinematicTime( time );
}

int Material_GetCinematicStartTime( const idMaterial* material )
{
	return material->GetCinematicStartTime();
}

bool Material_CinematicIsPlaying( const idMaterial* material )
{
	return material->CinematicIsPlaying();
}

idImage* Material_GetEditorImage( const idMaterial* material )
{
	return material->GetEditorImage();
}

bool Material_Save( idMaterial* material, const char* fileName )
{
	return material->Save( fileName );
}

bool Image_IsLoaded( const idImage* image )
{
	return image->IsLoaded();
}

void Image_ActuallyLoadImage( idImage* image, bool fromBackEnd )
{
	image->ActuallyLoadImage( fromBackEnd );
}

bool Render_EditorTargetBegin( int width, int height, const float* clearColor )
{
	return Framebuffer::EditorTargetBegin( width, height, clearColor );
}

bool Render_EditorTargetEnd( unsigned char* rgba )
{
	return Framebuffer::EditorTargetEnd( rgba );
}

void CmdlineProgress_Start( CommandlineProgressBar* bar )
{
	bar->Start();
}

void CmdlineProgress_Increment( CommandlineProgressBar* bar, bool updateScreen )
{
	bar->Increment( updateScreen );
}

float Render_GetFrameShaderTime()
{
	return tr.frameShaderTime;
}

void Render_SetFrameShaderTime( float time )
{
	tr.frameShaderTime = time;
}

void Render_SetEditorRenderCrop( int x1, int y1, int x2, int y2 )
{
	tr.renderCrops[tr.currentRenderCrop].x1 = ( short )x1;
	tr.renderCrops[tr.currentRenderCrop].y1 = ( short )y1;
	tr.renderCrops[tr.currentRenderCrop].x2 = ( short )x2;
	tr.renderCrops[tr.currentRenderCrop].y2 = ( short )y2;
}

/*
================================================
GetRenderAPI
================================================
*/
extern "C" ID_RENDER_API renderExport_t* GetRenderAPI( renderImport_t* import )
{
	if( import == NULL || import->version != RENDER_API_VERSION )
	{
		return NULL;
	}

	sys							= import->sys;
	common						= import->common;
	cmdSystem					= import->cmdSystem;
	cvarSystem					= import->cvarSystem;
	fileSystem					= import->fileSystem;
	soundSystem					= import->soundSystem;
	uiManager					= import->uiManager;
	declManager					= import->declManager;
	imguiSystem					= import->imguiSystem;
	parallelJobManager			= import->parallelJobManager;	// idLib global, the job threads belong to the exe
	renderHost					= import->host;
	renderHostCVars				= import->cvars;

	// set interface pointers used by idLib
	idLib::sys					= sys;
	idLib::common				= common;
	idLib::cvarSystem			= cvarSystem;
	idLib::fileSystem			= fileSystem;

	// initialize idLib
	idLib::Init();

	idLocalization::SetDictionaryPtr( common->GetLanguageDictionary() );

	// register static cvars declared in the renderer
	idCVar::RegisterStaticVars();

	// initialize processor specific SIMD
	idSIMD::InitProcessor( "renderer", renderHostCVars.com_forceGenericSIMD->GetBool() );

	declManager->RegisterDeclType( "material", DECL_MATERIAL, idDeclAllocator<idMaterial> );

#define RENDER_INTERNAL_FILL( ret, name, params, args )	renderInternalTable.name = &name;
	RENDER_INTERNAL_FUNCS( RENDER_INTERNAL_FILL )
#undef RENDER_INTERNAL_FILL

	// setup export interface
	renderExport.version			= RENDER_API_VERSION;
	renderExport.renderSystem		= renderSystem;
	renderExport.renderModelManager	= renderModelManager;
	renderExport.deviceManager		= deviceManager;
	renderExport.internal			= &renderInternalTable;
	renderExport.apiName			= R_API_Name();

	return &renderExport;
}
