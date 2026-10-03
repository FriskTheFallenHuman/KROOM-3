/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Stephen Pridham
Copyright (C) 2026 Justin Marshall

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

idCVar r_atmos( "r_atmos", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = off\n1 = map .atmos decl only\n2 = decl, built in defaults on maps without one\n3 = force defaults + cvars everywhere", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_atmosDebug( "r_atmosDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = debug information\n2 = debug information + ignore depth", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_atmosCloudLayers( "r_atmosCloudLayers", "5", CVAR_RENDERER | CVAR_INTEGER, "cloud raymarch layers (quality / cost), 1..7", 1, 7, idCmdSystem::ArgCompletion_Integer<1, 7> );

idCVar r_atmosSkyMaterial( "r_atmosSkyMaterial", "", CVAR_RENDERER, "override: sky material (2D equirect or cube), 'none' = gradient" );
idCVar r_atmosSkyTint( "r_atmosSkyTint", "", CVAR_RENDERER, "override: sky material tint 'r g b'" );
idCVar r_atmosZenithColor( "r_atmosZenithColor", "", CVAR_RENDERER, "override: gradient zenith color 'r g b'" );
idCVar r_atmosHorizonColor( "r_atmosHorizonColor", "", CVAR_RENDERER, "override: gradient horizon color 'r g b'" );
idCVar r_atmosIntensity( "r_atmosIntensity", "", CVAR_RENDERER, "override: linear HDR multiplier" );
idCVar r_atmosGrade( "r_atmosGrade", "", CVAR_RENDERER, "override: S-curve grade strength 0..1" );
idCVar r_atmosPortalSky( "r_atmosPortalSky", "", CVAR_RENDERER, "override: behind | replace | off" );

idCVar r_atmosClouds( "r_atmosClouds", "", CVAR_RENDERER, "override: 0 / 1" );
idCVar r_atmosCloudHeight( "r_atmosCloudHeight", "", CVAR_RENDERER, "override: 'min max' units above the eye" );
idCVar r_atmosCloudScale( "r_atmosCloudScale", "", CVAR_RENDERER, "override: cloud noise scale" );
idCVar r_atmosCloudCoverage( "r_atmosCloudCoverage", "", CVAR_RENDERER, "override: cloud coverage 0..1, lower = denser" );
idCVar r_atmosCloudDensity( "r_atmosCloudDensity", "", CVAR_RENDERER, "override: cloud density" );
idCVar r_atmosCloudOpacity( "r_atmosCloudOpacity", "", CVAR_RENDERER, "override: cloud opacity 0..1" );
idCVar r_atmosCloudSeed( "r_atmosCloudSeed", "", CVAR_RENDERER, "override: cloud noise seed" );
idCVar r_atmosCloudColor( "r_atmosCloudColor", "", CVAR_RENDERER, "override: cloud color 'r g b' or 'auto'" );
idCVar r_atmosCloudWind( "r_atmosCloudWind", "", CVAR_RENDERER, "override: wind 'x y' in units per second" );
idCVar r_atmosCloudVolume( "r_atmosCloudVolume", "", CVAR_RENDERER, "override: cloud self shadowing 0..1" );

idCVar r_atmosFog( "r_atmosFog", "", CVAR_RENDERER, "override: horizon fog 0 / 1" );
idCVar r_atmosFogColor( "r_atmosFogColor", "", CVAR_RENDERER, "override: fog color 'r g b' or 'auto'" );
idCVar r_atmosFogOpacity( "r_atmosFogOpacity", "", CVAR_RENDERER, "override: fog opacity 0..1" );
idCVar r_atmosFogHeight( "r_atmosFogHeight", "", CVAR_RENDERER, "override: view ray z where the fog fades out" );
idCVar r_atmosFogPower( "r_atmosFogPower", "", CVAR_RENDERER, "override: fog falloff power" );

static bool CvarIsSet( const idCVar& cvar )
{
	return cvar.GetString()[0] != '\0';
}

static bool CvarFloat( const idCVar& cvar, float& out )
{
	if( !CvarIsSet( cvar ) )
	{
		return false;
	}
	out = cvar.GetFloat();
	return true;
}

static bool CvarBool( const idCVar& cvar, bool& out )
{
	if( !CvarIsSet( cvar ) )
	{
		return false;
	}
	out = cvar.GetBool();
	return true;
}

static bool CvarFloats( const idCVar& cvar, float* out, int count )
{
	if( !CvarIsSet( cvar ) )
	{
		return false;
	}

	float tmp[3] = { 0.0f, 0.0f, 0.0f };
	idLexer src( cvar.GetString(), idStr::Length( cvar.GetString() ), "atmos cvar", LEXFL_NOERRORS );
	for( int i = 0; i < count; i++ )
	{
		tmp[i] = src.ParseFloat();
	}
	for( int i = 0; i < count; i++ )
	{
		out[i] = tmp[i];
	}
	return true;
}

static bool CvarColor( const idCVar& cvar, idVec3& out )
{
	float c[3];
	if( !CvarFloats( cvar, c, 3 ) )
	{
		return false;
	}
	const float scale = ( c[0] > 1.0f || c[1] > 1.0f || c[2] > 1.0f ) ? ( 1.0f / 255.0f ) : 1.0f;
	out.Set( idMath::ClampFloat( 0.0f, 1.0f, c[0] * scale ),
			 idMath::ClampFloat( 0.0f, 1.0f, c[1] * scale ),
			 idMath::ClampFloat( 0.0f, 1.0f, c[2] * scale ) );
	return true;
}

static bool CvarAutoColor( const idCVar& cvar, bool& autoColor, idVec3& out )
{
	if( !CvarIsSet( cvar ) )
	{
		return false;
	}
	if( idStr::Icmp( cvar.GetString(), "auto" ) == 0 )
	{
		autoColor = true;
		return true;
	}
	if( CvarColor( cvar, out ) )
	{
		autoColor = false;
		return true;
	}
	return false;
}

/*
================
GetSkyAverageColor
================
*/
struct skyAverage_t
{
	idStr		name;
	idVec3		color;
	bool		valid;
};
static idList<skyAverage_t> skyAverageCache;

static bool GetSkyAverageColor( const idImage* image, idVec3& out )
{
	if( image == NULL )
	{
		return false;
	}

	const char* name = image->GetName();
	for( int i = 0; i < skyAverageCache.Num(); i++ )
	{
		if( skyAverageCache[i].name.Icmp( name ) == 0 )
		{
			out = skyAverageCache[i].color;
			return skyAverageCache[i].valid;
		}
	}

	skyAverage_t entry;
	entry.name = name;
	entry.color.Set( 0.55f, 0.62f, 0.72f );
	entry.valid = false;

	byte* pic = NULL;
	int width = 0, height = 0;
	R_LoadImageProgram( name, &pic, &width, &height, NULL, NULL );

	if( pic != NULL && width > 0 && height > 1 )
	{
		double sum[3] = { 0.0, 0.0, 0.0 };
		const int rows = height / 2;
		for( int y = 0; y < rows; y++ )
		{
			const byte* row = pic + y * width * 4;
			for( int x = 0; x < width; x++ )
			{
				sum[0] += row[x * 4 + 0];
				sum[1] += row[x * 4 + 1];
				sum[2] += row[x * 4 + 2];
			}
		}
		const double inv = 1.0 / ( 255.0 * double( rows ) * double( width ) );
		entry.color.Set( Max( 0.03f, float( sum[0] * inv ) ), Max( 0.03f, float( sum[1] * inv ) ), Max( 0.03f, float( sum[2] * inv ) ) );
		entry.valid = true;
	}

	if( pic != NULL )
	{
		R_StaticFree( pic );
	}

	skyAverageCache.Append( entry );
	out = entry.color;
	return entry.valid;
}

/*
================
R_AtmosFlushCache
================
*/
void R_AtmosFlushCache()
{
	skyAverageCache.Clear();
}

/*
================
ResolveAtmosParms
================
*/
static bool ResolveAtmosParms( const idRenderWorldLocal* world, atmosParms_t& p, bool& cloudAuto, bool& fogAuto )
{
	const int mode = r_atmos.GetInteger();
	if( mode <= 0 )
	{
		return false;
	}

	p.SetDefaults();

	if( mode < 3 )
	{
		if( world->atmosOptOut )
		{
			return false;
		}
		if( world->atmosDecl != NULL )
		{
			p = world->atmosDecl->GetParms();
		}
		else if( mode < 2 )
		{
			return false;
		}
	}

	cloudAuto = p.clouds.autoColor;
	fogAuto = p.fog.autoColor;

	// ---- overrides
	if( CvarIsSet( r_atmosSkyMaterial ) )
	{
		if( idStr::Icmp( r_atmosSkyMaterial.GetString(), "none" ) == 0 )
		{
			p.skyMaterial = NULL;
		}
		else
		{
			const idMaterial* mat = declManager->FindMaterial( r_atmosSkyMaterial.GetString(), false );
			if( mat != NULL )
			{
				p.skyMaterial = mat;
			}
		}
	}
	CvarColor( r_atmosSkyTint, p.skyTint );
	CvarColor( r_atmosZenithColor, p.zenithColor );
	CvarColor( r_atmosHorizonColor, p.horizonColor );

	float f;
	if( CvarFloat( r_atmosIntensity, f ) )
	{
		p.intensity = Max( 0.0f, f );
	}
	if( CvarFloat( r_atmosGrade, f ) )
	{
		p.grade = idMath::ClampFloat( 0.0f, 1.0f, f );
	}
	if( CvarIsSet( r_atmosPortalSky ) )
	{
		const char* s = r_atmosPortalSky.GetString();
		if( !idStr::Icmp( s, "behind" ) )
		{
			p.portalSky = ATMOS_PORTALSKY_BEHIND;
		}
		else if( !idStr::Icmp( s, "replace" ) )
		{
			p.portalSky = ATMOS_PORTALSKY_REPLACE;
		}
		else if( !idStr::Icmp( s, "off" ) )
		{
			p.portalSky = ATMOS_PORTALSKY_OFF;
		}
	}

	CvarBool( r_atmosClouds, p.clouds.enabled );
	float h[2];
	if( CvarFloats( r_atmosCloudHeight, h, 2 ) )
	{
		p.clouds.heightMin = h[0];
		p.clouds.heightMax = h[1];
	}
	if( CvarFloat( r_atmosCloudScale, f ) )
	{
		p.clouds.scale = Max( 0.00001f, f );
	}
	if( CvarFloat( r_atmosCloudCoverage, f ) )
	{
		p.clouds.coverage = idMath::ClampFloat( 0.0f, 1.0f, f );
	}
	if( CvarFloat( r_atmosCloudDensity, f ) )
	{
		p.clouds.density = Max( 0.0f, f );
	}
	if( CvarFloat( r_atmosCloudOpacity, f ) )
	{
		p.clouds.opacity = idMath::ClampFloat( 0.0f, 1.0f, f );
	}
	CvarFloat( r_atmosCloudSeed, p.clouds.seed );
	CvarAutoColor( r_atmosCloudColor, cloudAuto, p.clouds.color );
	float w[2];
	if( CvarFloats( r_atmosCloudWind, w, 2 ) )
	{
		p.clouds.wind.Set( w[0], w[1] );
	}
	if( CvarFloat( r_atmosCloudVolume, f ) )
	{
		p.clouds.volume = idMath::ClampFloat( 0.0f, 1.0f, f );
	}

	CvarBool( r_atmosFog, p.fog.enabled );
	CvarAutoColor( r_atmosFogColor, fogAuto, p.fog.color );
	if( CvarFloat( r_atmosFogOpacity, f ) )
	{
		p.fog.opacity = idMath::ClampFloat( 0.0f, 1.0f, f );
	}
	if( CvarFloat( r_atmosFogHeight, f ) )
	{
		p.fog.height = Max( 0.01f, f );
	}
	if( CvarFloat( r_atmosFogPower, f ) )
	{
		p.fog.power = Max( 0.25f, f );
	}

	if( p.clouds.heightMax < p.clouds.heightMin + 64.0f )
	{
		p.clouds.heightMax = p.clouds.heightMin + 64.0f;
	}

	return true;
}

/*
================
R_AtmosPortalSkyMode
================
*/
int R_AtmosPortalSkyMode( const idRenderWorldLocal* world )
{
	atmosParms_t p;
	bool cloudAuto, fogAuto;
	if( world == NULL || !ResolveAtmosParms( world, p, cloudAuto, fogAuto ) )
	{
		return -1;
	}
	return p.portalSky;
}

/*
================
R_SetupAtmosForView

Returns NULL when this view must not draw the atmosphere.
================
*/
static const atmosFrame_t* SetupAtmosForView( const idRenderWorldLocal* world, const viewDef_t* viewDef, const char*& reason );

const atmosFrame_t* R_SetupAtmosForView( const idRenderWorldLocal* world, const viewDef_t* viewDef )
{
	const char* reason = "drawn";
	const atmosFrame_t* a = SetupAtmosForView( world, viewDef, reason );

	if( r_atmosDebug.GetInteger() > 0 )
	{
		static const char* lastReason = NULL;
		static int lastFlags = -1;
		const int flags = viewDef->renderView.rdflags;
		if( reason != lastReason || flags != lastFlags )
		{
			lastReason = reason;
			lastFlags = flags;
			common->Printf( "atmos: %s (r_atmos %d, decl %s, disabled %d, rdflags 0x%x)\n", reason, r_atmos.GetInteger(),
							world->atmosDecl ? world->atmosDecl->GetName() : "none", ( int )world->atmosOptOut, flags );
		}
	}
	return a;
}

static const atmosFrame_t* SetupAtmosForView( const idRenderWorldLocal* world, const viewDef_t* viewDef, const char*& reason )
{
	const int rdflags = viewDef->renderView.rdflags;

	if( ( rdflags & RDF_NOATMOS ) != 0 )
	{
		reason = "RDF_NOATMOS";
		return NULL;
	}

	atmosParms_t p;
	bool cloudAuto, fogAuto;
	if( !ResolveAtmosParms( world, p, cloudAuto, fogAuto ) )
	{
		reason = "off: r_atmos 0, map disabled, or no decl on this map (r_atmos 1)";
		return NULL;
	}

	if( ( rdflags & RDF_PORTALSKY_VIEW ) != 0 )
	{
		if( p.portalSky == ATMOS_PORTALSKY_OFF )
		{
			reason = "sky box view, portalSky off";
			return NULL;
		}
	}
	else if( ( rdflags & RDF_PORTALSKY_ACTIVE ) != 0 )
	{
		reason = "main view, portalsky box rendered (atmos is in the portalsky box view)";
		return NULL;
	}

	atmosFrame_t* a = ( atmosFrame_t* )R_FrameAlloc( sizeof( atmosFrame_t ), FRAME_ALLOC_VIEW_DEF );

	// sky dome
	idVec3 average = p.horizonColor;
	a->skyImage = NULL;
	if( p.skyMaterial != NULL && p.skyMaterial->GetNumStages() > 0 )
	{
		const idImage* image = p.skyMaterial->GetStage( 0 )->texture.image;
		if( image != NULL )
		{
			a->skyImage = image;
			GetSkyAverageColor( image, average );
		}
	}

	a->skyTint = p.skyTint;
	a->zenithColor = p.zenithColor;
	a->horizonColor = p.horizonColor;
	a->intensity = p.intensity;
	a->grade = p.grade;

	if( a->skyImage == NULL )
	{
		// gradient
		average = p.horizonColor;
	}

	// clouds
	a->cloudOpacity = p.clouds.enabled ? p.clouds.opacity : 0.0f;
	a->cloudHeightMin = p.clouds.heightMin;
	a->cloudHeightMax = p.clouds.heightMax;
	a->cloudScale = p.clouds.scale;
	a->cloudCoverage = p.clouds.coverage;
	a->cloudDensity = p.clouds.density;
	a->cloudSeed = p.clouds.seed;
	a->cloudVolume = p.clouds.volume;
	a->cloudLayers = idMath::ClampInt( 1, 7, r_atmosCloudLayers.GetInteger() );
	a->cloudWind = p.clouds.wind;
	a->cloudColor = cloudAuto ? average : p.clouds.color;

	// fog
	a->fogOpacity = p.fog.enabled ? p.fog.opacity : 0.0f;
	a->fogHeight = p.fog.height;
	a->fogPower = p.fog.power;
	a->fogColor = fogAuto ? average : p.fog.color;

	return a;
}

/*
================
idRenderWorldLocal::SetAtmos
================
*/
void idRenderWorldLocal::SetAtmos( const idDeclAtmos* decl, bool optOut )
{
	atmosDecl = decl;
	atmosOptOut = optOut;
}

/*
================
idRenderWorldLocal::GetAtmosPortalSkyMode
================
*/
int idRenderWorldLocal::GetAtmosPortalSkyMode() const
{
	return R_AtmosPortalSkyMode( this );
}
