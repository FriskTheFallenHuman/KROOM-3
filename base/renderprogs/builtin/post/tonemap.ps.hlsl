/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2009-2015 Robert Beckebans

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

#include "renderprogs/global.inc.hlsl"


// *INDENT-OFF*
uniform sampler2D	samp0 : register(s0); // texture 0 is _currentRender
uniform sampler2D	samp1 : register(s1); // texture 1 is heatmap
uniform float4  rpUser0 : register(c128); // x=preset, y=saturation, z=contrast, w=exposure_bias

struct PS_IN
{
	float4 position : VPOS;
	float2 texcoord0 : TEXCOORD0_centroid;
};

struct PS_OUT
{
	float4 color : COLOR;
};
// *INDENT-ON*

//
// https://github.com/colour-science/aces-dev
//
float3 ACESFilm( float3 x )
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return saturate( ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e ) );
}

//
// https://gamedev.stackexchange.com/questions/62917/uncharted-2-tone-mapping-and-an-eye-adaptation
//
float3 Uncharted2Tonemap( float3 x )
{
	float A = 0.22;
	float B = 0.3;
	float C = 0.10;
	float D = 0.20;
	float E = 0.01;
	float F = 0.30;
	return ( ( x * ( A * x + C * B ) + D * E ) / ( x * ( A * x + B ) + D * F ) ) - E / F;
}

//
// Filmic Blender (Blender 2.x - 3.x default)
//
float3 FilmicBlender( float3 x )
{
	float3 c = max( float3( 0.0, 0.0, 0.0 ), x - float3( 0.004, 0.004, 0.004 ) );
	return ( c * ( 6.2 * c + 0.5 ) ) / ( c * ( 6.2 * c + 1.7 ) + 0.06 );
}

//
// AgX by Troy Sobotka (Blender 4.0+ default)
//
float3 AgXDefaultContrastApprox( float3 x )
{
	float3 x2 = x * x;
	float3 x4 = x2 * x2;
	float3 x6 = x4 * x2;

	return + 15.5     * x6 * x
		   - 40.14    * x6
		   + 31.96    * x4 * x
		   - 6.868    * x4
		   + 0.4298   * x2 * x
		   + 0.1191   * x2
		   - 0.00232  * x
		   + 0.0;
}

float3 AgX( float3 color )
{
	// Input transform: linear sRGB -> AgX Log (Rec.2020 primaries)
	const float3x3 AgXInputMatrix = float3x3(
										float3( 0.842479062253094,  0.0423282422610123, 0.0423756549057051 ),
										float3( 0.0784335999999992, 0.878468636469772,  0.0784336 ),
										float3( 0.0792237451477643, 0.0791661274605434, 0.879520573507657 )
									);

	color = AgXInputMatrix * color;

	// Log encoding
	const float AgXMinEV = -12.47393;
	const float AgXMaxEV = 4.026069;

	color = clamp( color, float3( 0.0, 0.0, 0.0 ), float3( 1.0, 1.0, 1.0 ) );
	color = max( color, 1e-10 );
	color = log2( color );
	color = ( color - AgXMinEV ) / ( AgXMaxEV - AgXMinEV );
	color = clamp( color, float3( 0.0, 0.0, 0.0 ), float3( 1.0, 1.0, 1.0 ) );

	// S-curve look
	color = AgXDefaultContrastApprox( color );

	// Output transform: AgX -> linear sRGB
	const float3x3 AgXOutputMatrix = float3x3(
										 float3( 1.19687900512017,   -0.0528968517574562, -0.0529716355144438 ),
										 float3( -0.0980208811401368,  1.15190312990417,   -0.0980434501171241 ),
										 float3( -0.0990297440797205, -0.0989611768448433,  1.15107028411220 )
									 );

	color = AgXOutputMatrix * color;
	return saturate( color );
}

//
// https://www-old.cs.utah.edu/docs/techreports/2002/pdf/UUCS-02-001.pdf
//
float3 ReinhardExtended( float3 x, float maxWhite )
{
	return ( x * ( 1.0 + x / ( maxWhite * maxWhite ) ) ) / ( 1.0 + x );
}

//
// Color grading
//
float3 ApplySaturation( float3 color, float saturation )
{
	float lum = dot( color, float3( 0.2126, 0.7152, 0.0722 ) );
	return lerp( float3( lum, lum, lum ), color, saturation );
}

float3 ApplyContrast( float3 color, float contrast )
{
	// Pivot around linear mid-gray (0.18)
	const float pivot = 0.18;
	return ( color - pivot ) * contrast + pivot;
}

void main( PS_IN fragment, out PS_OUT result )
{
	float2 tCoords = fragment.texcoord0;

#if defined( BRIGHTPASS_FILTER )
	tCoords *= float2( 4.0, 4.0 );
#endif

	float4 color = tex2D( samp0, tCoords );
	float  Y = dot( LUMINANCE_SRGB, color );

	// Read tone mapping parameters
	// rpScreenCorrectionFactor carries hdr key/lum data (set by CalculateAutomaticExposure)
	float hdrKey = rpScreenCorrectionFactor.x;
	float hdrAverageLum = rpScreenCorrectionFactor.y;
	float hdrMaxLum = rpScreenCorrectionFactor.z;

	// rpUser0 carries our new controls (set by idRenderBackend::Tonemap)
	int   tonemapPreset = int( rpUser0.x );
	float saturation = rpUser0.y;
	float contrast = rpUser0.z;
	float exposureBias = rpUser0.w;  // extra exposure offset in stops

	// Exposure
	float avgLum = max( hdrAverageLum, 0.001 );
	float linExp = ( hdrKey / avgLum );
	float exposure = log2( max( linExp, 0.0001 ) ) + exposureBias;

#if defined( BRIGHTPASS )
	// Bright pass: used for bloom extraction
	if( Y < 0.1 )
	{
		result.color = float4( 0.0, 0.0, 0.0, 1.0 );
		return;
	}
#endif

	float3 exposedColor = exp2( exposure ) * color.rgb;

	// Tone mapping presset
	float3 mapped;

	if( tonemapPreset == 1 )
	{
		mapped = FilmicBlender( exposedColor );
	}
	else if( tonemapPreset == 2 )
	{
		mapped = AgX( exposedColor );
	}
	else if( tonemapPreset == 3 )
	{
		mapped = ReinhardExtended( exposedColor, hdrMaxLum );
	}
	else if( tonemapPreset == 4 )
	{
		float3 curr = Uncharted2Tonemap( exposedColor );
		float3 whiteScale = 1.0 / Uncharted2Tonemap( float3( hdrMaxLum, hdrMaxLum, hdrMaxLum ) );
		mapped = curr * whiteScale;
	}
	else if( tonemapPreset == 5 )
	{
		mapped = saturate( exposedColor );
	}
	else
	{
		mapped = ACESFilm( exposedColor );
	}

	mapped = max( float3( 0.0, 0.0, 0.0 ), mapped );

	// Color grading (applied in tonemapped space, before gamma)
	if( saturation != 1.0 )
	{
		mapped = ApplySaturation( mapped, saturation );
		mapped = max( float3( 0.0, 0.0, 0.0 ), mapped );
	}

	if( contrast != 1.0 )
	{
		mapped = ApplyContrast( mapped, contrast );
		mapped = max( float3( 0.0, 0.0, 0.0 ), mapped );
	}

#if defined( BRIGHTPASS )
	const half hdrContrastThreshold = rpOverbright.x;
	const half hdrContrastOffset = rpOverbright.y;

	float Yr = ( hdrKey * Y ) / hdrAverageLum;
	float T = max( Yr - hdrContrastThreshold, 0.0 );
	float B = T > 0.0 ? T / ( hdrContrastOffset + T ) : T;
	mapped *= clamp( B, 0.0, 1.0 );
#endif

	// AgX and Filmic Blender already encode an approximate gamma.
	// ACES, Reinhard, Uncharted need explicit sRGB gamma.
	if( tonemapPreset == 1 || tonemapPreset == 2 )
	{
		// Filmic/AgX: already perceptually encoded, just clamp
		result.color = float4( saturate( mapped ), 1.0 );
	}
	else
	{
		float gamma = 1.0 / 2.2;
		mapped.r = pow( mapped.r, gamma );
		mapped.g = pow( mapped.g, gamma );
		mapped.b = pow( mapped.b, gamma );
		result.color = float4( mapped, 1.0 );
	}

#if defined( HDR_DEBUG )
	// Luminance heat map visualization
	const float3 debugColors[16] =
	{
		float3( 0.0,   0.0,    0.0 ),
		float3( 0.0,   0.0,    0.1647 ),
		float3( 0.0,   0.0,    0.3647 ),
		float3( 0.0,   0.0,    0.6647 ),
		float3( 0.0,   0.0,    0.9647 ),
		float3( 0.0,   0.9255, 0.9255 ),
		float3( 0.0,   0.5647, 0.0 ),
		float3( 0.0,   0.7843, 0.0 ),
		float3( 1.0,   1.0,    0.0 ),
		float3( 0.906, 0.753,  0.0 ),
		float3( 1.0,   0.5647, 0.0 ),
		float3( 1.0,   0.0,    0.0 ),
		float3( 0.839, 0.0,    0.0 ),
		float3( 1.0,   0.0,    1.0 ),
		float3( 0.6,   0.333,  0.788 ),
		float3( 1.0,   1.0,    1.0 )
	};
	float v = log2( Y / 0.18 );
	v = clamp( v + 5.0, 0.0, 15.0 );
	int idx = int( floor( v ) );
	result.color = float4( lerp( debugColors[idx], debugColors[min( 15, idx + 1 )], fract( v ) ), 1.0 );
#endif
}
