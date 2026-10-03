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

// *INDENT-OFF*
uniform float4 rpUser0 : register( c128 );	// right.xyz, 1/proj[0]
uniform float4 rpUser1 : register( c129 );	// up.xyz, 1/proj[5] (signed, follows the projection)
uniform float4 rpUser2 : register( c130 );	// forward.xyz, time (s)
uniform float4 rpUser3 : register( c131 );	// sky tint rgb (zenith rgb in gradient mode), sky mode (0 gradient, 1 material)
uniform float4 rpUser4 : register( c132 );	// horizon rgb, cloud layer count
uniform float4 rpUser5 : register( c133 );	// cloud color rgb, cloud opacity
uniform float4 rpUser6 : register( c134 );	// cloud heightMin, heightMax, scale, coverage
uniform float4 rpUser7 : register( c135 );	// cloud density, seed, wind x, wind y
uniform float4 rpUser8 : register( c136 );	// fog color rgb, fog opacity
uniform float4 rpUser9 : register( c137 );	// fog height, fog power, cloud volume, intensity
uniform float4 rpUser10 : register( c138 );	// projection offset xy, grade strength

struct PS_IN {
	float4 position : VPOS;
	float2 texcoord0 : TEXCOORD0_centroid;
};

struct PS_OUT {
	float4 color : COLOR;
};
// *INDENT-ON*

float AtmosHash13( float3 p )
{
	p = frac( p * 0.1031 );
	p += dot( p, p.yzx + 33.33 );
	return frac( ( p.x + p.y ) * p.z );
}

float AtmosNoise( float3 p )
{
	float3 i = floor( p );
	float3 f = frac( p );
	f = f * f * ( 3.0 - 2.0 * f );

	float n000 = AtmosHash13( i + float3( 0.0, 0.0, 0.0 ) );
	float n100 = AtmosHash13( i + float3( 1.0, 0.0, 0.0 ) );
	float n010 = AtmosHash13( i + float3( 0.0, 1.0, 0.0 ) );
	float n110 = AtmosHash13( i + float3( 1.0, 1.0, 0.0 ) );
	float n001 = AtmosHash13( i + float3( 0.0, 0.0, 1.0 ) );
	float n101 = AtmosHash13( i + float3( 1.0, 0.0, 1.0 ) );
	float n011 = AtmosHash13( i + float3( 0.0, 1.0, 1.0 ) );
	float n111 = AtmosHash13( i + float3( 1.0, 1.0, 1.0 ) );

	float nx00 = lerp( n000, n100, f.x );
	float nx10 = lerp( n010, n110, f.x );
	float nx01 = lerp( n001, n101, f.x );
	float nx11 = lerp( n011, n111, f.x );

	return lerp( lerp( nx00, nx10, f.y ), lerp( nx01, nx11, f.y ), f.z );
}

float AtmosFBm( float3 p )
{
	float sum = 0.0;
	float amp = 0.56;
	float norm = 0.0;

	for( int octave = 0; octave < 5; octave++ )
	{
		sum += AtmosNoise( p ) * amp;
		norm += amp;
		p *= 2.06;
		amp *= 0.52;
	}

	return sum / max( norm, 0.0001 );
}

float AtmosBillow( float3 p )
{
	return 1.0 - abs( AtmosFBm( p ) * 2.0 - 1.0 );
}

float3 AtmosGrade( float3 rgb, float strength )
{
	rgb = saturate( rgb );

	float3 sCurve = rgb * rgb * ( 3.0 - 2.0 * rgb );
	float luma = dot( rgb, float3( 0.299, 0.587, 0.114 ) );
	float highlight = smoothstep( 0.54, 0.92, luma );
	float shadow = 1.0 - smoothstep( 0.06, 0.42, luma );

	float3 graded = lerp( rgb, sCurve, 0.72 );
	graded = saturate( ( graded - 0.42 ) * 1.34 + 0.42 );
	graded *= 1.0 - shadow * 0.18;
	graded += rgb * rgb * ( 0.28 * highlight );

	return lerp( rgb, saturate( graded ), strength );
}

void main( PS_IN fragment, out PS_OUT result )
{
	float2 ndc = fragment.texcoord0;

	float3 axisRight = rpUser0.xyz;
	float3 axisUp = rpUser1.xyz;
	float3 axisForward = rpUser2.xyz;

	float viewX = ( ndc.x + rpUser10.x ) * rpUser0.w;
	float viewY = ( ndc.y + rpUser10.y ) * rpUser1.w;
	float3 ray = normalize( axisForward + axisRight * viewX + axisUp * viewY );

	// sky dome
	float3 skyColor;
	if( rpUser3.w > 0.5 )
	{
		// below the horizon the dome had no geometry, stretch the horizon row
		float3 skyDir = float3( ray.xy, max( ray.z, 0.0 ) );
		float skyLen = length( skyDir );
		if( skyLen > 0.0001 )
		{
			skyDir /= skyLen;
		}
		else
		{
			skyDir = float3( 0.0, 0.0, 1.0 );
		}

		skyColor = AtmosGrade( AtmosSampleSky( skyDir ).rgb * rpUser3.rgb, rpUser10.z );
	}
	else
	{
		skyColor = lerp( rpUser4.rgb, rpUser3.rgb, sqrt( saturate( ray.z ) ) );
	}

	// clouds, cloud heights are relative to the eye
	float time = rpUser2.w;
	float cloudAlpha = 0.0;
	float3 brightCloud = saturate( rpUser5.rgb );
	float cloudOpacity = rpUser5.w;
	float cloudDensity = rpUser7.x;
	float cloudVolume = saturate( rpUser9.z );
	float horizonMask = smoothstep( -0.015, 0.18, ray.z );

	if( cloudOpacity > 0.0001 && cloudDensity > 0.0001 && horizonMask > 0.0001 )
	{
		float zMin = rpUser6.x;
		float zMax = max( rpUser6.y, zMin + 64.0 );
		float slabDepth = zMax - zMin;
		float cloudZ = zMin + slabDepth * 0.55;
		float rayZ = max( ray.z, 0.10 );
		float baseT = cloudZ / rayZ;

		float2 windScroll = rpUser7.zw * time;
		float2 baseWorld = ( rpGlobalEyePos.xy + ray.xy * baseT ) + windScroll;
		float shade = 0.0;
		float shadow = 0.0;
		float scale = max( rpUser6.z, 0.00001 );
		float seed = rpUser7.y;
		float3 seedVec = float3( 11.7 + seed * 0.00011, 3.4 - seed * 0.00007, 9.1 );

		int layerCount = int( clamp( rpUser4.w, 1.0, 7.0 ) );
		float layerFrac = 1.0 / float( layerCount );
		float layerWeight = 0.36 * 7.0 * layerFrac;

		for( int layerIdx = 0; layerIdx < layerCount; layerIdx++ )
		{
			float l = ( float( layerIdx ) + 0.5 ) * layerFrac;
			float heightFade = smoothstep( 0.02, 0.34, l ) * ( 1.0 - smoothstep( 0.70, 1.0, l ) );
			float windShear = ( l - 0.5 ) * time * 5.5;
			float2 layerDrift = float2( 0.62, -0.28 ) * ( seed * 0.00003 + l * 310.0 + windShear );
			float2 parallax = ray.xy * ( ( l - 0.5 ) * slabDepth * lerp( 0.12, 0.22, cloudVolume ) );
			float2 cloudXY = ( baseWorld + parallax + layerDrift ) * scale;
			float3 p = float3( cloudXY, seed * 0.00009 + l * 2.7 );

			float broad = AtmosFBm( p * 0.48 + seedVec );
			float warpA = AtmosFBm( p * 0.82 + broad * 0.85 + seedVec.yzx );
			float billowA = smoothstep( 0.18, 0.96, AtmosBillow( p * 1.35 + warpA * 1.20 + time * 0.012 ) );
			float billowB = smoothstep( 0.42, 0.96, AtmosBillow( p * 2.05 + seedVec.zxy - time * 0.018 ) );

			float anvil = smoothstep( 0.20, 0.76, broad );
			float shape = anvil * ( billowA * 0.88 + billowB * 0.12 );
			shape = smoothstep( saturate( rpUser6.w ) * 0.66, 0.94, shape );

			float coreLift = lerp( 1.0, smoothstep( 0.04, 0.72, l ), cloudVolume * 0.42 );
			float selfShadow = lerp( 1.0, exp( -shadow * 1.65 ), cloudVolume );
			float density = shape * heightFade * coreLift * max( cloudDensity, 0.0 );
			cloudAlpha += ( 1.0 - cloudAlpha ) * density * layerWeight;
			shade += density * selfShadow * ( 0.70 + l * 0.26 + broad * 0.10 );
			shadow += density * ( 0.52 + ( 1.0 - l ) * 0.35 );
		}

		cloudAlpha = saturate( cloudAlpha * saturate( cloudOpacity ) * horizonMask );

		float skyLum = dot( saturate( rpUser5.rgb ), float3( 0.299, 0.587, 0.114 ) );
		float3 cloudTint = saturate( rpUser5.rgb * ( 1.22 + skyLum * 0.18 ) );
		float volumeShade = lerp( 1.0, 0.82 + exp( -shadow * 0.38 ) * 0.26, cloudVolume );
		brightCloud = lerp( cloudTint, float3( 1.0, 0.985, 0.94 ), 0.48 );
		brightCloud *= ( 0.88 + saturate( shade ) * 0.24 + saturate( skyLum ) * 0.12 ) * volumeShade;
		brightCloud = saturate( brightCloud );
	}

	// horizon fog
	float fogHeight = max( rpUser9.x, 0.01 );
	float fogShape = 1.0 - smoothstep( -0.02, fogHeight, ray.z );
	fogShape = pow( saturate( fogShape ), max( rpUser9.y, 0.25 ) );
	float fogAlpha = saturate( rpUser8.w ) * fogShape;

	// composite clouds + fog over the dome
	float alpha = cloudAlpha + fogAlpha * ( 1.0 - cloudAlpha );
	float3 overlay = ( brightCloud * cloudAlpha + saturate( rpUser8.rgb ) * fogAlpha * ( 1.0 - cloudAlpha ) ) / max( alpha, 0.001 );
	float3 finalColor = lerp( skyColor, saturate( overlay ), alpha );

	// the scene is linear: convert once at the end, same as builtin/legacy/skybox
	result.color = float4( sRGBToLinearRGB( saturate( finalColor ) ) * rpUser9.w, 1.0 );
}
