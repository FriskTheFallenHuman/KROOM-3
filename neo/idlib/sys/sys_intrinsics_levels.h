/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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

#if defined(USE_INTRINSICS_SSE)

#undef _mm_madd_ps
#undef _mm_nmsub_ps
#undef _mm_selm_ps
#undef _mm_msum3_ps
#undef _mm_msum4_ps

// a * b + c
#if ID_SIMD_LEVEL >= ID_SIMD_LEVEL_AVX2
	#define _mm_madd_ps( a, b, c )				_mm_fmadd_ps( (a), (b), (c) )
	#define _mm_nmsub_ps( a, b, c )				_mm_fnmadd_ps( (a), (b), (c) )
#else
	#define _mm_madd_ps( a, b, c )				_mm_add_ps( _mm_mul_ps( (a), (b) ), (c) )
	#define _mm_nmsub_ps( a, b, c )				_mm_sub_ps( (c), _mm_mul_ps( (a), (b) ) )
#endif

// select b where the mask is set and a elsewhere, the mask has to be a comparison result
#if ID_SIMD_LEVEL >= ID_SIMD_LEVEL_SSE41
	#define _mm_selm_ps( a, b, c )				_mm_blendv_ps( (a), (b), __m128c( c ) )
#else
	#define _mm_selm_ps( a, b, c )				_mm_sel_ps( a, b, c )
#endif

// dot product of the first 3 / all 4 components, the result is replicated into all 4 components
#if ID_SIMD_LEVEL >= ID_SIMD_LEVEL_SSE41
	#define _mm_msum3_ps( a, b )				_mm_dp_ps( (a), (b), 0x7F )
	#define _mm_msum4_ps( a, b )				_mm_dp_ps( (a), (b), 0xFF )
#else
	#define _mm_msum3_ps( a, b )				_mm_msum3_ps_sse2( a, b )
	#define _mm_msum4_ps( a, b )				_mm_msum4_ps_sse2( a, b )
#endif

#endif // #if defined(USE_INTRINSICS_SSE)
