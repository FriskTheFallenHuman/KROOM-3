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

#ifndef __RENDERAPI_H__
#define __RENDERAPI_H__

struct renderInternal_t;
class CinematicAudio;

/*
================================================

idRenderHost

================================================
*/
class idRenderHost
{
public:
	virtual					~idRenderHost() {}

	virtual unsigned int	Milliseconds() = 0;
	virtual uint64			Microseconds() = 0;
	virtual int				EventLoopMilliseconds() = 0;
	virtual void			OnWindowCreated() = 0;
	virtual void			OnWindowDestroyed() = 0;
	virtual CinematicAudio*	CreateCinematicAudio( bool bink ) = 0;
	virtual void			WaitGameThread() = 0;
	virtual void			DrawGameFrame() = 0;
	virtual void			CloseConsole() = 0;
	virtual bool			SessionIsLoading() = 0;
	virtual bool			GetActiveTonemapState( int& preset, float& exposure, float& saturation, float& contrast, float& hdrKey ) = 0;
	virtual void*			GetImGuiContext() = 0;
	virtual void			WriteDeclCache( idDemoFile* f, int demoCategory, int demoCode, declType_t declType ) = 0;
};

/*
================================================

cvars defined by the executable.

================================================
*/
struct renderHostCVars_t
{
	idCVar*		com_smp;
	idCVar*		com_engineHz;
	idCVar*		com_version;
	idCVar*		com_developer;
	idCVar*		com_productionMode;
	idCVar*		com_forceGenericSIMD;
	idCVar*		s_noSound;
	idCVar*		s_playCinematicAudio;
};

/*
================================================================================================

Renderer module API.

================================================================================================
*/

const int RENDER_API_VERSION = 2;

#ifdef _WIN32
	#define ID_RENDER_API	__declspec( dllexport )
#else
	#define ID_RENDER_API	__attribute__( ( visibility( "default" ) ) )
#endif

/*
================================================

renderImport_t

================================================
*/
struct renderImport_t
{
	int							version;

	idSys*						sys;
	idCommon*					common;
	idCmdSystem*				cmdSystem;
	idCVarSystem*				cvarSystem;
	idFileSystem*				fileSystem;
	idSoundSystem*				soundSystem;
	idUserInterfaceManager*		uiManager;
	idDeclManager*				declManager;
	idImGuiSystem*				imguiSystem;
	idParallelJobManager*		parallelJobManager;

	idRenderHost*				host;
	renderHostCVars_t			cvars;
};

/*
================================================

renderExport_t

================================================
*/
struct renderExport_t
{
	int							version;
	const char*					apiName;

	idRenderSystem*				renderSystem;
	idRenderModelManager*		renderModelManager;
	idDeviceManager*			deviceManager;
	renderInternal_t*			internal;
};

typedef renderExport_t* ( *GetRenderAPI_t )( renderImport_t* import );

#endif /* !__RENDERAPI_H__ */
