/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2016 Johannes Ohlemacher (http://github.com/eXistence/fhDOOM)

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
uniform sampler2D samp0 : register(s0);
#if defined( USE_SOFT_PARTICLE )
uniform sampler2D samp1 : register(s1);
#endif

struct PS_IN {
	float4 position : VPOS;
	float2 texcoord0 : TEXCOORD0_centroid;
	float4 color : COLOR0;
};

struct PS_OUT {
	float4 color : COLOR;
};
// *INDENT-ON*

#if defined( USE_SOFT_PARTICLE )
// window depth to eye distance, the inverse of R_SetupProjectionMatrix: ndc.z = 0.999 - zNear / dist
float SoftParticleEyeDistance( float windowZ )
{
	float denom = -rpProjectionMatrixZ.z - ( windowZ * 2.0 - 1.0 );
	return ( denom > 0.00001 ) ? ( -rpProjectionMatrixZ.w / denom ) : 1.0e9;
}

// c is linear, fragPos is VPOS (window pixels + window depth)
float4 SoftParticleFade( float4 c, float4 fragPos )
{
	float2 uv = ( fragPos.xy - rpSoftParticleViewport.xy ) * rpSoftParticleViewport.zw;
	float sceneDist = SoftParticleEyeDistance( tex2D( samp1, uv ).r );
	float fragDist = SoftParticleEyeDistance( fragPos.z );
	float f = saturate( ( sceneDist - fragDist ) / rpSoftParticleParms.x );

	int mode = int( rpSoftParticleParms.y + 0.5 );
	if( mode == 3 )
	{
		// DBM_COLORALPHA_ZERO
		c *= f;
	}
	else if( mode == 4 )
	{
		// DBM_COLORALPHA_ONE
		c = lerp( float4( 1.0, 1.0, 1.0, 1.0 ), c, f );
	}
	else if( mode == 5 )
	{
		// DBM_ALPHA_ONE
		c.a = lerp( 1.0, c.a, f );
	}
	else if( mode == 6 )
	{
		// DBM_ALPHA_ZERO
		c.a *= f;
	}

	return c;
}
#endif

void main( PS_IN fragment, out PS_OUT result )
{
	float4 color = tex2D( samp0, fragment.texcoord0 ) * fragment.color;
	clip( color.a - rpAlphaTest.x );
	color = sRGBAToLinearRGBA( color );
#if defined( USE_SOFT_PARTICLE )
	color = SoftParticleFade( color, fragment.position );
#endif
	result.color = color;
}