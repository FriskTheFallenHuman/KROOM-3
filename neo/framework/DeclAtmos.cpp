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

/*
================
atmosParms_t::SetDefaults
================
*/
void atmosParms_t::SetDefaults()
{
	skyMaterial = NULL;
	skyTint.Set( 1.0f, 1.0f, 1.0f );
	zenithColor.Set( 0.16f, 0.28f, 0.48f );
	horizonColor.Set( 0.55f, 0.62f, 0.72f );
	intensity = 1.0f;
	grade = 1.0f;
	portalSky = ATMOS_PORTALSKY_BEHIND;

	clouds.enabled = true;
	clouds.heightMin = 2600.0f;
	clouds.heightMax = 7600.0f;
	clouds.scale = 0.00021f;
	clouds.coverage = 0.30f;
	clouds.density = 0.5f;
	clouds.opacity = 0.78f;
	clouds.seed = 12000.0f;
	clouds.autoColor = true;
	clouds.color.Set( 0.85f, 0.62f, 0.48f );
	clouds.wind.Set( 55.0f, -18.0f );
	clouds.volume = 0.65f;

	fog.enabled = true;
	fog.autoColor = true;
	fog.color.Set( 0.55f, 0.62f, 0.72f );
	fog.opacity = 0.36f;
	fog.height = 0.30f;
	fog.power = 1.15f;
}

/*
=================
idDeclAtmos::Size
=================
*/
size_t idDeclAtmos::Size() const
{
	return sizeof( idDeclAtmos );
}

/*
================
idDeclAtmos::FreeData
================
*/
void idDeclAtmos::FreeData()
{
	parms.SetDefaults();
}

/*
================
idDeclAtmos::SetDefaultText
================
*/
bool idDeclAtmos::SetDefaultText()
{
	return false;
}

/*
================
idDeclAtmos::DefaultDefinition
================
*/
const char* idDeclAtmos::DefaultDefinition() const
{
	return "{\n}";
}

/*
================
ParseColor3

Accepts 0..1 or 0..255.
================
*/
static void ParseColor3( idLexer& src, idVec3& out )
{
	float c[3];
	c[0] = src.ParseFloat();
	c[1] = src.ParseFloat();
	c[2] = src.ParseFloat();

	const float scale = ( c[0] > 1.0f || c[1] > 1.0f || c[2] > 1.0f ) ? ( 1.0f / 255.0f ) : 1.0f;
	out.Set( idMath::ClampFloat( 0.0f, 1.0f, c[0] * scale ),
			 idMath::ClampFloat( 0.0f, 1.0f, c[1] * scale ),
			 idMath::ClampFloat( 0.0f, 1.0f, c[2] * scale ) );
}

/*
================
ParseAutoColor3
================
*/
static bool ParseAutoColor3( idLexer& src, idVec3& out )
{
	idToken token;
	if( src.ReadToken( &token ) && !token.Icmp( "auto" ) )
	{
		return true;
	}
	src.UnreadToken( &token );
	ParseColor3( src, out );
	return false;
}

/*
================
idDeclAtmos::Parse
================
*/
bool idDeclAtmos::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	idLexer src;
	idToken	token;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	parms.SetDefaults();

	while( 1 )
	{
		if( !src.ReadToken( &token ) )
		{
			break;
		}

		if( !token.Icmp( "}" ) )
		{
			break;
		}

		if( !token.Icmp( "skyMaterial" ) )
		{
			idToken name;
			if( !src.ReadToken( &name ) )
			{
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			parms.skyMaterial = declManager->FindMaterial( name, false );
			if( parms.skyMaterial == NULL )
			{
				src.Warning( "atmos '%s': sky material '%s' not found, using gradient", GetName(), name.c_str() );
			}
		}
		else if( !token.Icmp( "skyTint" ) )
		{
			ParseColor3( src, parms.skyTint );
		}
		else if( !token.Icmp( "zenithColor" ) )
		{
			ParseColor3( src, parms.zenithColor );
		}
		else if( !token.Icmp( "horizonColor" ) )
		{
			ParseColor3( src, parms.horizonColor );
		}
		else if( !token.Icmp( "intensity" ) )
		{
			parms.intensity = Max( 0.0f, src.ParseFloat() );
		}
		else if( !token.Icmp( "grade" ) )
		{
			parms.grade = idMath::ClampFloat( 0.0f, 1.0f, src.ParseFloat() );
		}
		else if( !token.Icmp( "portalSky" ) )
		{
			idToken mode;
			src.ReadToken( &mode );
			if( !mode.Icmp( "behind" ) )
			{
				parms.portalSky = ATMOS_PORTALSKY_BEHIND;
			}
			else if( !mode.Icmp( "replace" ) )
			{
				parms.portalSky = ATMOS_PORTALSKY_REPLACE;
			}
			else if( !mode.Icmp( "off" ) )
			{
				parms.portalSky = ATMOS_PORTALSKY_OFF;
			}
			else
			{
				src.Warning( "atmos '%s': bad portalSky mode '%s' (behind | replace | off)", GetName(), mode.c_str() );
			}
		}
		else if( !token.Icmp( "clouds" ) )
		{
			src.ExpectTokenString( "{" );
			while( 1 )
			{
				idToken key;
				if( !src.ReadToken( &key ) )
				{
					src.Warning( "Unexpected end of file" );
					MakeDefault();
					return false;
				}
				if( !key.Icmp( "}" ) )
				{
					break;
				}

				if( !key.Icmp( "enabled" ) )
				{
					parms.clouds.enabled = src.ParseBool();
				}
				else if( !key.Icmp( "heightMin" ) )
				{
					parms.clouds.heightMin = src.ParseFloat();
				}
				else if( !key.Icmp( "heightMax" ) )
				{
					parms.clouds.heightMax = src.ParseFloat();
				}
				else if( !key.Icmp( "scale" ) )
				{
					parms.clouds.scale = Max( 0.00001f, src.ParseFloat() );
				}
				else if( !key.Icmp( "coverage" ) )
				{
					parms.clouds.coverage = idMath::ClampFloat( 0.0f, 1.0f, src.ParseFloat() );
				}
				else if( !key.Icmp( "density" ) )
				{
					parms.clouds.density = Max( 0.0f, src.ParseFloat() );
				}
				else if( !key.Icmp( "opacity" ) )
				{
					parms.clouds.opacity = idMath::ClampFloat( 0.0f, 1.0f, src.ParseFloat() );
				}
				else if( !key.Icmp( "seed" ) )
				{
					parms.clouds.seed = src.ParseFloat();
				}
				else if( !key.Icmp( "color" ) )
				{
					parms.clouds.autoColor = ParseAutoColor3( src, parms.clouds.color );
				}
				else if( !key.Icmp( "wind" ) )
				{
					parms.clouds.wind.x = src.ParseFloat();
					parms.clouds.wind.y = src.ParseFloat();
				}
				else if( !key.Icmp( "volume" ) )
				{
					parms.clouds.volume = idMath::ClampFloat( 0.0f, 1.0f, src.ParseFloat() );
				}
				else
				{
					src.Warning( "atmos '%s': unknown clouds key '%s'", GetName(), key.c_str() );
					src.SkipRestOfLine();
				}
			}
		}
		else if( !token.Icmp( "fog" ) )
		{
			src.ExpectTokenString( "{" );
			while( 1 )
			{
				idToken key;
				if( !src.ReadToken( &key ) )
				{
					src.Warning( "Unexpected end of file" );
					MakeDefault();
					return false;
				}
				if( !key.Icmp( "}" ) )
				{
					break;
				}

				if( !key.Icmp( "enabled" ) )
				{
					parms.fog.enabled = src.ParseBool();
				}
				else if( !key.Icmp( "color" ) )
				{
					parms.fog.autoColor = ParseAutoColor3( src, parms.fog.color );
				}
				else if( !key.Icmp( "opacity" ) )
				{
					parms.fog.opacity = idMath::ClampFloat( 0.0f, 1.0f, src.ParseFloat() );
				}
				else if( !key.Icmp( "height" ) )
				{
					parms.fog.height = Max( 0.01f, src.ParseFloat() );
				}
				else if( !key.Icmp( "power" ) )
				{
					parms.fog.power = Max( 0.25f, src.ParseFloat() );
				}
				else
				{
					src.Warning( "atmos '%s': unknown fog key '%s'", GetName(), key.c_str() );
					src.SkipRestOfLine();
				}
			}
		}
		else
		{
			src.Warning( "atmos '%s': unknown key '%s'", GetName(), token.c_str() );
			src.SkipRestOfLine();
		}
	}

	if( parms.clouds.heightMax < parms.clouds.heightMin + 64.0f )
	{
		parms.clouds.heightMax = parms.clouds.heightMin + 64.0f;
	}

	return true;
}
