/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/

#include "renderprogs/global.inc.hlsl"


// *INDENT-OFF*
uniform sampler2D	samp0 : register(s0); // texture 0 is _current Render
uniform sampler2D	samp1 : register(s1); // texture 1 is the per-surface bump map

struct PS_IN {
	float4 position		: VPOS;
	float4 texcoord0	: TEXCOORD0_centroid;
	float4 texcoord1	: TEXCOORD1_centroid;
};

struct PS_OUT {
	float4 color : COLOR;
};
// *INDENT-ON*

void main( PS_IN fragment, out PS_OUT result )
{

	// load the filtered normal map and convert to -1 to 1 range
	float4 bumpMap = ( tex2D( samp1, fragment.texcoord0.xy ) * 2.0f ) - 1.0f;
	float2 localNormal = bumpMap.wy;

	// calculate the screen texcoord in the 0.0 to 1.0 range
	float2 screenTexCoord = vposToScreenPosTexCoord( fragment.position.xy );
	screenTexCoord += ( localNormal * fragment.texcoord1.xy );
	screenTexCoord = saturate( screenTexCoord );

	// load the screen render
	result.color = ( tex2D( samp0, screenTexCoord.xy ) );
}