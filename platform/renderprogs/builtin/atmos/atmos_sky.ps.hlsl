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

#include "renderprogs/global.inc.hlsl"

// *INDENT-OFF*
uniform sampler2D samp0 : register(s0);
// *INDENT-ON*

float4 AtmosSampleSky( float3 dir )
{
	float u = atan( dir.y, dir.x ) * ( 0.5 / PI ) + 0.5;
	float v = 1.0 - ( asin( clamp( dir.z, -1.0, 1.0 ) ) / PI + 0.5 );

	float2 uvA = float2( u, v );
	float2 uvB = float2( frac( u + 0.5 ), v );

	float2 dxA = dFdx( uvA );
	float2 dyA = dFdy( uvA );
	float2 dxB = dFdx( uvB );
	float2 dyB = dFdy( uvB );

	float2 gradX = dxA;
	float2 gradY = dyA;
	if( dot( dxB, dxB ) + dot( dyB, dyB ) < dot( dxA, dxA ) + dot( dyA, dyA ) )
	{
		gradX = dxB;
		gradY = dyB;
	}

	return textureGrad( samp0, uvA, gradX, gradY );
}

#include "atmos.inc.hlsl"
