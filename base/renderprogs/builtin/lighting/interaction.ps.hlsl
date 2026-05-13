/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2021 Robert Beckebans

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

#include "renderprogs/BRDF.inc.hlsl"


// *INDENT-OFF*
uniform sampler2D				samp0 : register(s0); // texture 1 is the per-surface normal map
uniform sampler2D				samp1 : register(s1); // texture 3 is the per-surface specular or roughness/metallic/AO mixer map
uniform sampler2D				samp2 : register(s2); // texture 2 is the per-surface baseColor map
uniform sampler2D				samp3 : register(s3); // texture 4 is the light falloff texture
uniform sampler2D				samp4 : register(s4); // texture 5 is the light projection texture
uniform sampler2D				samp5 : register(s5); // texture 6 is the coat/sheen texture

struct PS_IN
{
	half4 position		: VPOS;
	half4 texcoord0		: TEXCOORD0_centroid;
	half4 texcoord1		: TEXCOORD1_centroid;
	half4 texcoord2		: TEXCOORD2_centroid;
	half4 texcoord3		: TEXCOORD3_centroid;
	half4 texcoord4		: TEXCOORD4_centroid;
	half4 texcoord5		: TEXCOORD5_centroid;
	half4 texcoord6		: TEXCOORD6_centroid;
	half4 color			: COLOR0;
};

struct PS_OUT
{
	half4 color : COLOR;
};
// *INDENT-ON*

// Sheen (Zeltner et al. 2022 Charlie NDF)
float SheenLookup( float x, float alphaG )
{
	float oneMinusAlphaSq = ( 1.0 - alphaG ) * ( 1.0 - alphaG );
	float a = lerp( 25.3245, 21.5473, oneMinusAlphaSq );
	float b = lerp(  3.3244,  3.8299, oneMinusAlphaSq );
	float c = lerp(  0.1680,  0.1982, oneMinusAlphaSq );
	float d = lerp( -1.2739, -1.9736, oneMinusAlphaSq );
	float e = lerp( -4.8597, -4.3205, oneMinusAlphaSq );
	return a / ( 1.0 + b * pow( abs( x ), c ) ) + d * x + e;
}

float D_Charlie( float roughness, float NdotH )
{
	float alpha  = roughness * roughness;
	float invAlpha = 1.0 / max( alpha, 0.001 );
	float sin2h  = max( 1.0 - NdotH * NdotH, 0.0078125 );
	return ( 2.0 + invAlpha ) * pow( sin2h, invAlpha * 0.5 ) / ( 2.0 * PI );
}

float V_Sheen( float NdotL, float NdotV, float roughness )
{
	float alphaG  = roughness * roughness;
	float lambdaV = exp( SheenLookup( NdotV, alphaG ) );
	float lambdaL = exp( SheenLookup( NdotL, alphaG ) );
	return 1.0 / ( ( 1.0 + lambdaV + lambdaL ) * ( 4.0 * max( NdotV * NdotL, 0.0001 ) ) );
}

half3 EvaluateSheen( half3 sheenColor, float sheenRoughness, float NdotL, float NdotV, float NdotH )
{
	if( dot( sheenColor, sheenColor ) < 0.0001 )
		return half3( 0.0, 0.0, 0.0 );
	float D = D_Charlie( sheenRoughness, NdotH );
	float V = V_Sheen( NdotL, NdotV, sheenRoughness );
	return sheenColor * D * V;
}

//
// Coat (GGX clearcoat, IOR 1.5, F0 = 0.04)
//
half3 EvaluateCoat( float coatWeight, float coatRoughness, float NdotL, float NdotV, float NdotH, float VdotH )
{
	if( coatWeight < 0.001 )
		return half3( 0.0, 0.0, 0.0 );

	float alpha = coatRoughness * coatRoughness;
	float alpha2 = alpha * alpha;
	float denom = NdotH * NdotH * ( alpha2 - 1.0 ) + 1.0;
	float D = alpha2 / ( PI * denom * denom );

	float k = ( coatRoughness + 1.0 );
	float k2 = ( k * k ) / 8.0;
	float GV = NdotV / ( NdotV * ( 1.0 - k2 ) + k2 );
	float GL = NdotL / ( NdotL * ( 1.0 - k2 ) + k2 );
	float G = GV * GL;

	// Schlick F, F0 = 0.04
	float Fc = pow( 1.0 - VdotH, 5.0 );
	float3 Fcoat = float3( 0.04 + ( 1.0 - 0.04 ) * Fc );

	return coatWeight * Fcoat * D * G / max( 4.0 * NdotL * NdotV, 0.0001 );
}

//
// Energy conservation: coat attenuates the base layer
//
float CoatAttenuation( float coatWeight, float NdotV )
{
	float Fc = pow( 1.0 - NdotV, 5.0 );
	float F = 0.04 + ( 1.0 - 0.04 ) * Fc;
	return 1.0 - coatWeight * F;
}

void main( PS_IN fragment, out PS_OUT result )
{
	half4 bumpMap      = tex2D( samp0, fragment.texcoord1.xy );
	half4 lightFalloff = idtex2Dproj( samp3, fragment.texcoord2 );
	half4 lightProj    = idtex2Dproj( samp4, fragment.texcoord3 );
	half4 YCoCG        = tex2D( samp2, fragment.texcoord4.xy );
	half4 specMapSRGB  = tex2D( samp1, fragment.texcoord5.xy );
	half4 specMap      = sRGBAToLinearRGBA( specMapSRGB );

	// OpenPBR layer map (zero when not bound — safe default)
	// R = coat weight, G = coat roughness, B = sheen weight, A = sheen roughness
	half4 layerMap     = tex2D( samp5, fragment.texcoord5.xy );

	half3 lightVector  = normalize( fragment.texcoord0.xyz );
	half3 viewVector   = normalize( fragment.texcoord6.xyz );
	half3 diffuseMap   = sRGBToLinearRGB( ConvertYCoCgToRGB( YCoCG ) );

	// ---- Normal ----
	half3 localNormal;
	// RB begin
#if defined(USE_NORMAL_FMT_RGB8)
	localNormal.xy = bumpMap.rg - 0.5;
#else
	localNormal.xy = bumpMap.wy - 0.5;
#endif
	// RB end
	localNormal.z = sqrt( abs( dot( localNormal.xy, localNormal.xy ) - 0.25 ) );
	localNormal = normalize( localNormal );

	// dot products
	half ldotN = saturate( dot3( localNormal, lightVector ) );
	half NdotV = clamp( dot3( localNormal, viewVector ),  0.001, 1.0 );

	half3 halfAngle = normalize( lightVector + viewVector );
	half NdotH = clamp( dot3( localNormal, halfAngle ),  0.0, 1.0 );
	half VdotH = clamp( dot3( viewVector,  halfAngle ),  0.0, 1.0 );
	half ldotH = clamp( dot3( lightVector, halfAngle ),  0.0, 1.0 );

#if defined( USE_HALF_LAMBERT )
	// RB: http://developer.valvesoftware.com/wiki/Half_Lambert
	half halfLdotN = dot3( localNormal, lightVector ) * 0.5 + 0.5;
	halfLdotN *= halfLdotN;

	// tweak to not loose so many details
	half lambert = lerp( ldotN, halfLdotN, 0.5 );
#else
	half lambert = ldotN;
#endif

	// Light color
	half3 lightColor = sRGBToLinearRGB( lightProj.xyz * lightFalloff.xyz );

	// Material model selection
	half3 diffuseLight;
	half3 specularLight;

#if defined( USE_PBR )
	const half  metallic = specMapSRGB.g;
	const half  roughness = max( specMapSRGB.r, 0.045 );
	const half3 dielectricF0 = half3( 0.04, 0.04, 0.04 );
	const half3 baseColor = diffuseMap;

	half3 diffuseColor  = baseColor * ( 1.0 - metallic );
	half3 specularColor = lerp( dielectricF0, baseColor, metallic );

	// GGX specular (ARM approximation — cheap single-division form)
	float rr = roughness * roughness;
	float rrrr = rr * rr;
	float D = ( NdotH * NdotH ) * ( rrrr - 1.0 ) + 1.0;
	float VF = ( ldotH * ldotH ) * ( roughness + 0.5 );
	half3 reflectColor = specularColor * rpSpecularModifier.rgb;

	specularLight = ( rrrr / ( 4.0 * PI * D * D * VF ) ) * ldotN * reflectColor;
	diffuseLight = diffuseColor * lambert * rpDiffuseModifier.xyz;

	// coat layer
	float coatWeight = layerMap.r;
	float coatRoughness = max( layerMap.g, 0.045 );

	half3 coatBRDF = EvaluateCoat( coatWeight, coatRoughness, ldotN, NdotV, NdotH, VdotH );
	float baseAtten = CoatAttenuation( coatWeight, NdotV );

	// sheen layer
	// Sheen color is base color tinted by sheen weight
	half3 sheenColor = baseColor * layerMap.b;
	float sheenRoughness = max( layerMap.a, 0.07 );

	half3 sheenBRDF = EvaluateSheen( sheenColor, sheenRoughness, ldotN, NdotV, NdotH );

	// Layer order (outside in): coat -> sheen -> base
	// Coat attenuates base. Sheen sits between coat and base.
	half3 baseLayer = ( diffuseLight + specularLight ) * baseAtten;
	half3 combined = ( baseLayer + sheenBRDF * lambert + coatBRDF ) * lightColor;

	result.color.rgb = combined * fragment.color.rgb;
	result.color.a = 1.0;

#else
	const float roughness = EstimateLegacyRoughness( specMapSRGB.rgb );
	half3 diffuseColor = diffuseMap;
	half3 specularColor = specMapSRGB.rgb; // not linear, intentionally flat look

	float rr = roughness * roughness;
	float rrrr = rr * rr;
	float D = ( NdotH * NdotH ) * ( rrrr - 1.0 ) + 1.0;
	float VF = ( ldotH * ldotH ) * ( roughness + 0.5 );
	half3 reflectColor = specularColor * rpSpecularModifier.rgb;

	specularLight = ( rrrr / ( 4.0 * PI * D * D * VF ) ) * ldotN * reflectColor;
	diffuseLight = diffuseColor * lambert * rpDiffuseModifier.xyz;

	result.color.rgb = ( diffuseLight + specularLight ) * lightColor * fragment.color.rgb;
	result.color.a = 1.0;
#endif
}
