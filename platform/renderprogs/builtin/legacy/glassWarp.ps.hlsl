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

#include "renderprogs/global.inc.hlsl"

// *INDENT-OFF*
uniform sampler2D samp0 : register(s0); // texture 0 is the base glass texture
uniform sampler2D samp1 : register(s1); // texture 1 is the scratch warp texture
uniform sampler2D samp2 : register(s2); // texture 2 is the detail crack/face texture

struct PS_IN {
	float4 position : VPOS;
	float2 texcoord0 : TEXCOORD0_centroid;
	float2 texcoord1 : TEXCOORD1_centroid;
	float4 color : COLOR0;
};

struct PS_OUT {
	float4 color : COLOR;
};
// *INDENT-ON*

void main( PS_IN fragment, out PS_OUT result )
{
	float2 uv = fragment.texcoord0;
	float2 centered = uv - 0.5f;
	float2 warped = centered * 2.0f;

	float4 noise = tex2D( samp2, uv );
	float2 distortion = noise.xy * 0.2f;

	float2 shifted = ( fragment.texcoord1 - 0.5f ) * ( 1.0f - distortion * 2.0f ) + 0.5f;
	float4 base = tex2D( samp0, shifted );
	float4 warpTex = tex2D( samp1, shifted );
	float4 detail = tex2D( samp2, shifted ) * 0.1f;

	float grey = dot( base.rgb, float3( 0.299f, 0.587f, 0.114f ) );
	float4 tint = float4( grey, grey, grey, 1.0f ) * float4( 0.6f, 0.6f, 1.0f, 1.0f );
	float4 warpedColor = base * ( 1.0f - noise.x ) + tint * noise.x;
	warpedColor += detail;
	warpedColor += warpTex * 0.1f;

	result.color = float4( sRGBToLinearRGB( warpedColor.xyz ), 1.0f ) * fragment.color;
}
