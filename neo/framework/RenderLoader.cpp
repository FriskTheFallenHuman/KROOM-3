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

#include "../renderer/RenderCommon.h"
#include "../renderer/RenderContext.h"
#include "../renderer/RenderAPI.h"
#include "../renderer/RenderInternal.h"

#include "Common_local.h"
#include "RenderLoader.h"
#include "../renderer/CmdlineProgressbar.h"
#include "../sound/OpenAL/AL_CinematicAudio.h"

#include <SDL2/SDL.h>

idRenderSystem*			renderSystem = NULL;
idRenderModelManager*	renderModelManager = NULL;
idDeviceManager*		deviceManager = NULL;
renderInternal_t*		renderInternal = NULL;

idCVar r_renderer( "r_renderer", "opengl", CVAR_SYSTEM | CVAR_ARCHIVE, "renderer module loaded at startup: opengl, vulkan. Needs a restart." );

static uintptr_t		rendererDLL = 0;
static const char*		rendererAPIName = "none";

/*
================================================================================================

idRenderHostLocal

================================================================================================
*/
extern void WriteDeclCache( idDemoFile* f, int demoCategory, int demoCode, declType_t declType );

class idRenderHostLocal : public idRenderHost
{
public:
	virtual unsigned int	Milliseconds()
	{
		return Sys_Milliseconds();
	}
	virtual uint64			Microseconds()
	{
		return Sys_Microseconds();
	}
	virtual int				EventLoopMilliseconds()
	{
		return eventLoop->Milliseconds();
	}
	virtual void			OnWindowCreated()
	{
		Sys_InitInput();
	}
	virtual void			OnWindowDestroyed()
	{
		Sys_ShutdownInput();
	}
	virtual CinematicAudio*	CreateCinematicAudio( bool bink )
	{
		return new( TAG_AUDIO ) CinematicAudio_OpenAL( bink );
	}
	virtual void			WaitGameThread()
	{
		commonLocal.WaitGameThread();
	}
	virtual void			DrawGameFrame()
	{
		commonLocal.Draw();
	}
	virtual void			CloseConsole()
	{
		console->Close();
	}
	virtual bool			SessionIsLoading()
	{
		return session->GetState() == idSession::LOADING;
	}
	virtual bool			GetActiveTonemapState( int& preset, float& exposure, float& saturation, float& contrast, float& hdrKey )
	{
		return game != NULL && game->GetActiveTonemapState( preset, exposure, saturation, contrast, hdrKey );
	}
	virtual void*			GetImGuiContext()
	{
		return ImGui::GetCurrentContext();
	}
	virtual void			WriteDeclCache( idDemoFile* f, int demoCategory, int demoCode, declType_t declType )
	{
		::WriteDeclCache( f, demoCategory, demoCode, declType );
	}
};

static idRenderHostLocal renderHostLocal;

/*
========================
PeekConfigRenderer
========================
*/
static idStr PeekConfigRenderer()
{
	idStr result;
#ifdef CONFIG_FILE
	idFile* file = fileSystem->OpenFileRead( CONFIG_FILE );
	if( file == NULL )
	{
		return result;
	}

	const int len = file->Length();
	char* buffer = ( char* )Mem_Alloc( len + 1, TAG_TEMP );
	file->Read( buffer, len );
	buffer[ len ] = '\0';
	fileSystem->CloseFile( file );

	const char* key = "r_renderer";
	const int keyLen = idStr::Length( key );
	for( const char* p = strstr( buffer, key ); p != NULL; p = strstr( p + keyLen, key ) )
	{
		// whole word, at the start of a "seta r_renderer ..." / "set r_renderer ..." line
		if( p != buffer && p[-1] != ' ' && p[-1] != '\t' )
		{
			continue;
		}

		const char* q = p + keyLen;
		if( *q != ' ' && *q != '\t' )
		{
			continue;
		}
		while( *q == ' ' || *q == '\t' )
		{
			q++;
		}
		const bool quoted = ( *q == '"' );
		if( quoted )
		{
			q++;
		}
		const char* e = q;
		while( *e != '\0' && *e != '"' && *e != '\n' && *e != '\r' && ( quoted || ( *e != ' ' && *e != '\t' ) ) )
		{
			e++;
		}

		result = q;
		result.CapLength( ( int )( e - q ) );
		break;
	}

	Mem_Free( buffer );
#endif
	return result;
}

/*
========================
TryLoadRendererModule
========================
*/
static bool TryLoadRendererModule( const char* api )
{
	idStr baseName = "ref_neo_";
	baseName += api;

	char dllPath[ MAX_OSPATH ];
	fileSystem->FindDLL( baseName.c_str(), dllPath );
	if( !dllPath[ 0 ] )
	{
		// not in <fs_basepath>/<gamedir>: try the directory of the executable
		char* basePath = SDL_GetBasePath();
		if( basePath != NULL )
		{
			char dllName[ MAX_OSPATH ];
			sys->DLL_GetFileName( baseName.c_str(), dllName, MAX_OSPATH );
			idStr exePath = basePath;
			exePath.AppendPath( dllName );
			SDL_free( basePath );
			idStr::Copynz( dllPath, exePath.c_str(), MAX_OSPATH );
		}
	}
	if( !dllPath[ 0 ] )
	{
		common->Warning( "couldn't find renderer module '%s'", baseName.c_str() );
		return false;
	}

	common->Printf( "Loading renderer module: '%s'\n", dllPath );
	uintptr_t dll = sys->DLL_Load( dllPath );
	if( !dll )
	{
		common->Warning( "couldn't load renderer module '%s'", dllPath );
		return false;
	}

	GetRenderAPI_t GetRenderAPI = ( GetRenderAPI_t )Sys_DLL_GetProcAddress( dll, "GetRenderAPI" );
	if( !GetRenderAPI )
	{
		common->Warning( "couldn't find GetRenderAPI in '%s'", dllPath );
		Sys_DLL_Unload( dll );
		return false;
	}

	renderImport_t import;
	memset( &import, 0, sizeof( import ) );
	import.version				= RENDER_API_VERSION;
	import.sys					= ::sys;
	import.common				= ::common;
	import.cmdSystem			= ::cmdSystem;
	import.cvarSystem			= ::cvarSystem;
	import.fileSystem			= ::fileSystem;
	import.soundSystem			= ::soundSystem;
	import.uiManager			= ::uiManager;
	import.declManager			= ::declManager;
	import.imguiSystem			= ::imguiSystem;
	import.parallelJobManager	= ::parallelJobManager;
	import.host					= &renderHostLocal;

	extern idCVar com_smp;
	extern idCVar com_forceGenericSIMD;
	extern idCVar s_noSound;
	extern idCVar s_playCinematicAudio;
	import.cvars.s_noSound				= &s_noSound;
	import.cvars.s_playCinematicAudio	= &s_playCinematicAudio;
	import.cvars.com_smp				= &com_smp;
	import.cvars.com_engineHz			= &com_engineHz;
	import.cvars.com_version			= &com_version;
	import.cvars.com_developer			= &com_developer;
	import.cvars.com_productionMode		= &com_productionMode;
	import.cvars.com_forceGenericSIMD	= &com_forceGenericSIMD;

	renderExport_t* export_ = GetRenderAPI( &import );
	if( export_ == NULL || export_->version != RENDER_API_VERSION )
	{
		common->Warning( "renderer module '%s' has a wrong API version", dllPath );
		Sys_DLL_Unload( dll );
		return false;
	}

	rendererDLL				= dll;
	rendererAPIName			= export_->apiName;
	renderSystem			= export_->renderSystem;
	renderModelManager		= export_->renderModelManager;
	deviceManager			= export_->deviceManager;
	renderInternal			= export_->internal;
	return true;
}

/*
========================
R_LoadRendererModule
========================
*/
void R_LoadRendererModule()
{
	idStr api = r_renderer.GetString();

	// a value from the command line is already in the cvar, otherwise look at the config file
	if( idStr::Icmp( api.c_str(), r_renderer.GetDefaultString() ) == 0 )
	{
		idStr fromConfig = PeekConfigRenderer();
		if( fromConfig.Length() > 0 )
		{
			api = fromConfig;
		}
	}
	api.ToLower();

	if( TryLoadRendererModule( api.c_str() ) )
	{
		return;
	}

	if( api.Icmp( "opengl" ) != 0 )
	{
		common->Warning( "falling back to the OpenGL renderer" );
		if( TryLoadRendererModule( "opengl" ) )
		{
			return;
		}
	}

	common->FatalError( "couldn't load a renderer module" );
}

/*
========================
R_GetRendererAPIName
========================
*/
const char* R_GetRendererAPIName()
{
	return rendererAPIName;
}

#define RENDER_INTERNAL_FORWARD( ret, name, params, args ) \
	ret name params { return renderInternal->name args; }
RENDER_INTERNAL_FUNCS( RENDER_INTERNAL_FORWARD )
#undef RENDER_INTERNAL_FORWARD

int idMaterial::CinematicLength() const
{
	return Material_CinematicLength( this );
}

void idMaterial::ResetCinematicTime( int time ) const
{
	Material_ResetCinematicTime( this, time );
}

int idMaterial::GetCinematicStartTime() const
{
	return Material_GetCinematicStartTime( this );
}

bool idMaterial::CinematicIsPlaying() const
{
	return Material_CinematicIsPlaying( this );
}

idImage* idMaterial::GetEditorImage() const
{
	return Material_GetEditorImage( this );
}

bool idMaterial::Save( const char* fileName )
{
	return Material_Save( this, fileName );
}

bool idImage::IsLoaded() const
{
	return Image_IsLoaded( this );
}

void idImage::ActuallyLoadImage( bool fromBackEnd )
{
	Image_ActuallyLoadImage( this, fromBackEnd );
}

void CommandlineProgressBar::Start()
{
	CmdlineProgress_Start( this );
}

void CommandlineProgressBar::Increment( bool updateScreen )
{
	CmdlineProgress_Increment( this, updateScreen );
}

idRenderContext rRenderContext;

void idRenderContext::MakeCurrent()
{
	RenderContext_MakeCurrent();
}

void idRenderContext::Disable()
{
	RenderContext_Disable();
}
