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

#ifndef __GLSTATE_H__
#define __GLSTATE_H__

// one/zero is flipped on src/dest so a gl state of 0 is SRC_ONE,DST_ZERO
static const uint64_t GLS_SRCBLEND_ONE					= 0 << 0;
static const uint64_t GLS_SRCBLEND_ZERO					= 1 << 0;
static const uint64_t GLS_SRCBLEND_DST_COLOR				= 2 << 0;
static const uint64_t GLS_SRCBLEND_ONE_MINUS_DST_COLOR	= 3 << 0;
static const uint64_t GLS_SRCBLEND_SRC_ALPHA				= 4 << 0;
static const uint64_t GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA	= 5 << 0;
static const uint64_t GLS_SRCBLEND_DST_ALPHA				= 6 << 0;
static const uint64_t GLS_SRCBLEND_ONE_MINUS_DST_ALPHA	= 7 << 0;
static const uint64_t GLS_SRCBLEND_BITS					= 7 << 0;

static const uint64_t GLS_DSTBLEND_ZERO					= 0 << 3;
static const uint64_t GLS_DSTBLEND_ONE					= 1 << 3;
static const uint64_t GLS_DSTBLEND_SRC_COLOR				= 2 << 3;
static const uint64_t GLS_DSTBLEND_ONE_MINUS_SRC_COLOR	= 3 << 3;
static const uint64_t GLS_DSTBLEND_SRC_ALPHA				= 4 << 3;
static const uint64_t GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA	= 5 << 3;
static const uint64_t GLS_DSTBLEND_DST_ALPHA				= 6 << 3;
static const uint64_t GLS_DSTBLEND_ONE_MINUS_DST_ALPHA	= 7 << 3;
static const uint64_t GLS_DSTBLEND_BITS					= 7 << 3;

//------------------------
// these masks are the inverse, meaning when set the glColorMask value will be 0,
// preventing that channel from being written
//------------------------
static const uint64_t GLS_DEPTHMASK						= 1 << 6;
static const uint64_t GLS_REDMASK							= 1 << 7;
static const uint64_t GLS_GREENMASK						= 1 << 8;
static const uint64_t GLS_BLUEMASK						= 1 << 9;
static const uint64_t GLS_ALPHAMASK						= 1 << 10;
static const uint64_t GLS_COLORMASK						= ( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK );

static const uint64_t GLS_POLYMODE_LINE					= 1 << 11;
static const uint64_t GLS_POLYGON_OFFSET					= 1 << 12;

static const uint64_t GLS_DEPTHFUNC_LESS					= 0 << 13;
static const uint64_t GLS_DEPTHFUNC_ALWAYS				= 1 << 13;
static const uint64_t GLS_DEPTHFUNC_GREATER				= 2 << 13;
static const uint64_t GLS_DEPTHFUNC_EQUAL					= 3 << 13;
static const uint64_t GLS_DEPTHFUNC_BITS					= 3 << 13;

static const uint64_t GLS_CULL_FRONTSIDED					= 0 << 15;
static const uint64_t GLS_CULL_BACKSIDED					= 1 << 15;
static const uint64_t GLS_CULL_TWOSIDED					= 2 << 15;
static const uint64_t GLS_CULL_BITS						= 2 << 15;
static const uint64_t GLS_CULL_MASK						= GLS_CULL_FRONTSIDED | GLS_CULL_BACKSIDED | GLS_CULL_TWOSIDED;

static const uint64_t GLS_BLENDOP_ADD						= 0 << 18;
static const uint64_t GLS_BLENDOP_SUB						= 1 << 18;
static const uint64_t GLS_BLENDOP_MIN						= 2 << 18;
static const uint64_t GLS_BLENDOP_MAX						= 3 << 18;
static const uint64_t GLS_BLENDOP_BITS					= 3 << 18;

// stencil bits
static const uint64_t GLS_STENCIL_FUNC_REF_SHIFT			= 20;
static const uint64_t GLS_STENCIL_FUNC_REF_BITS			= 0xFFll << GLS_STENCIL_FUNC_REF_SHIFT;

static const uint64_t GLS_STENCIL_FUNC_MASK_SHIFT			= 28;
static const uint64_t GLS_STENCIL_FUNC_MASK_BITS			= 0xFFll << GLS_STENCIL_FUNC_MASK_SHIFT;

#define GLS_STENCIL_MAKE_REF( x ) ( ( (uint64_t)(x) << GLS_STENCIL_FUNC_REF_SHIFT ) & GLS_STENCIL_FUNC_REF_BITS )
#define GLS_STENCIL_MAKE_MASK( x ) ( ( (uint64_t)(x) << GLS_STENCIL_FUNC_MASK_SHIFT ) & GLS_STENCIL_FUNC_MASK_BITS )

// Next 12 bits act as front+back unless GLS_SEPARATE_STENCIL is set, in which case it acts as front.
static const uint64_t GLS_STENCIL_FUNC_ALWAYS				= 0ull << 36;
static const uint64_t GLS_STENCIL_FUNC_LESS				= 1ull << 36;
static const uint64_t GLS_STENCIL_FUNC_LEQUAL				= 2ull << 36;
static const uint64_t GLS_STENCIL_FUNC_GREATER			= 3ull << 36;
static const uint64_t GLS_STENCIL_FUNC_GEQUAL				= 4ull << 36;
static const uint64_t GLS_STENCIL_FUNC_EQUAL				= 5ull << 36;
static const uint64_t GLS_STENCIL_FUNC_NOTEQUAL			= 6ull << 36;
static const uint64_t GLS_STENCIL_FUNC_NEVER				= 7ull << 36;
static const uint64_t GLS_STENCIL_FUNC_BITS				= 7ull << 36;

static const uint64_t GLS_STENCIL_OP_FAIL_KEEP			= 0ull << 39;
static const uint64_t GLS_STENCIL_OP_FAIL_ZERO			= 1ull << 39;
static const uint64_t GLS_STENCIL_OP_FAIL_REPLACE			= 2ull << 39;
static const uint64_t GLS_STENCIL_OP_FAIL_INCR			= 3ull << 39;
static const uint64_t GLS_STENCIL_OP_FAIL_DECR			= 4ull << 39;
static const uint64_t GLS_STENCIL_OP_FAIL_INVERT			= 5ull << 39;
static const uint64_t GLS_STENCIL_OP_FAIL_INCR_WRAP		= 6ull << 39;
static const uint64_t GLS_STENCIL_OP_FAIL_DECR_WRAP		= 7ull << 39;
static const uint64_t GLS_STENCIL_OP_FAIL_BITS			= 7ull << 39;

static const uint64_t GLS_STENCIL_OP_ZFAIL_KEEP			= 0ull << 42;
static const uint64_t GLS_STENCIL_OP_ZFAIL_ZERO			= 1ull << 42;
static const uint64_t GLS_STENCIL_OP_ZFAIL_REPLACE		= 2ull << 42;
static const uint64_t GLS_STENCIL_OP_ZFAIL_INCR			= 3ull << 42;
static const uint64_t GLS_STENCIL_OP_ZFAIL_DECR			= 4ull << 42;
static const uint64_t GLS_STENCIL_OP_ZFAIL_INVERT			= 5ull << 42;
static const uint64_t GLS_STENCIL_OP_ZFAIL_INCR_WRAP		= 6ull << 42;
static const uint64_t GLS_STENCIL_OP_ZFAIL_DECR_WRAP		= 7ull << 42;
static const uint64_t GLS_STENCIL_OP_ZFAIL_BITS			= 7ull << 42;

static const uint64_t GLS_STENCIL_OP_PASS_KEEP			= 0ull << 45;
static const uint64_t GLS_STENCIL_OP_PASS_ZERO			= 1ull << 45;
static const uint64_t GLS_STENCIL_OP_PASS_REPLACE			= 2ull << 45;
static const uint64_t GLS_STENCIL_OP_PASS_INCR			= 3ull << 45;
static const uint64_t GLS_STENCIL_OP_PASS_DECR			= 4ull << 45;
static const uint64_t GLS_STENCIL_OP_PASS_INVERT			= 5ull << 45;
static const uint64_t GLS_STENCIL_OP_PASS_INCR_WRAP		= 6ull << 45;
static const uint64_t GLS_STENCIL_OP_PASS_DECR_WRAP		= 7ull << 45;
static const uint64_t GLS_STENCIL_OP_PASS_BITS			= 7ull << 45;

// Next 12 bits act as back and are only active when GLS_SEPARATE_STENCIL is set.
static const uint64_t GLS_BACK_STENCIL_FUNC_ALWAYS		= 0ull << 48;
static const uint64_t GLS_BACK_STENCIL_FUNC_LESS			= 1ull << 48;
static const uint64_t GLS_BACK_STENCIL_FUNC_LEQUAL		= 2ull << 48;
static const uint64_t GLS_BACK_STENCIL_FUNC_GREATER		= 3ull << 48;
static const uint64_t GLS_BACK_STENCIL_FUNC_GEQUAL		= 4ull << 48;
static const uint64_t GLS_BACK_STENCIL_FUNC_EQUAL			= 5ull << 48;
static const uint64_t GLS_BACK_STENCIL_FUNC_NOTEQUAL		= 6ull << 48;
static const uint64_t GLS_BACK_STENCIL_FUNC_NEVER			= 7ull << 48;
static const uint64_t GLS_BACK_STENCIL_FUNC_BITS			= 7ull << 48;

static const uint64_t GLS_BACK_STENCIL_OP_FAIL_KEEP		= 0ull << 51;
static const uint64_t GLS_BACK_STENCIL_OP_FAIL_ZERO		= 1ull << 51;
static const uint64_t GLS_BACK_STENCIL_OP_FAIL_REPLACE	= 2ull << 51;
static const uint64_t GLS_BACK_STENCIL_OP_FAIL_INCR		= 3ull << 51;
static const uint64_t GLS_BACK_STENCIL_OP_FAIL_DECR		= 4ull << 51;
static const uint64_t GLS_BACK_STENCIL_OP_FAIL_INVERT		= 5ull << 51;
static const uint64_t GLS_BACK_STENCIL_OP_FAIL_INCR_WRAP	= 6ull << 51;
static const uint64_t GLS_BACK_STENCIL_OP_FAIL_DECR_WRAP	= 7ull << 51;
static const uint64_t GLS_BACK_STENCIL_OP_FAIL_BITS		= 7ull << 51;

static const uint64_t GLS_BACK_STENCIL_OP_ZFAIL_KEEP		= 0ull << 54;
static const uint64_t GLS_BACK_STENCIL_OP_ZFAIL_ZERO		= 1ull << 54;
static const uint64_t GLS_BACK_STENCIL_OP_ZFAIL_REPLACE	= 2ull << 54;
static const uint64_t GLS_BACK_STENCIL_OP_ZFAIL_INCR		= 3ull << 54;
static const uint64_t GLS_BACK_STENCIL_OP_ZFAIL_DECR		= 4ull << 54;
static const uint64_t GLS_BACK_STENCIL_OP_ZFAIL_INVERT	= 5ull << 54;
static const uint64_t GLS_BACK_STENCIL_OP_ZFAIL_INCR_WRAP	= 6ull << 54;
static const uint64_t GLS_BACK_STENCIL_OP_ZFAIL_DECR_WRAP	= 7ull << 54;
static const uint64_t GLS_BACK_STENCIL_OP_ZFAIL_BITS		= 7ull << 54;

static const uint64_t GLS_BACK_STENCIL_OP_PASS_KEEP		= 0ull << 57;
static const uint64_t GLS_BACK_STENCIL_OP_PASS_ZERO		= 1ull << 57;
static const uint64_t GLS_BACK_STENCIL_OP_PASS_REPLACE	= 2ull << 57;
static const uint64_t GLS_BACK_STENCIL_OP_PASS_INCR		= 3ull << 57;
static const uint64_t GLS_BACK_STENCIL_OP_PASS_DECR		= 4ull << 57;
static const uint64_t GLS_BACK_STENCIL_OP_PASS_INVERT		= 5ull << 57;
static const uint64_t GLS_BACK_STENCIL_OP_PASS_INCR_WRAP	= 6ull << 57;
static const uint64_t GLS_BACK_STENCIL_OP_PASS_DECR_WRAP	= 7ull << 57;
static const uint64_t GLS_BACK_STENCIL_OP_PASS_BITS		= 7ull << 57;

static const uint64_t GLS_SEPARATE_STENCIL				= GLS_BACK_STENCIL_OP_FAIL_BITS | GLS_BACK_STENCIL_OP_ZFAIL_BITS | GLS_BACK_STENCIL_OP_PASS_BITS;
static const uint64_t GLS_STENCIL_OP_BITS					= GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS | GLS_SEPARATE_STENCIL;
static const uint64_t GLS_STENCIL_FRONT_OPS				= GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS;
static const uint64_t GLS_STENCIL_BACK_OPS				= GLS_SEPARATE_STENCIL;

static const uint64_t GLS_DEPTH_TEST_MASK					= 1ull << 60;
static const uint64_t GLS_CLOCKWISE						= 1ull << 61;
static const uint64_t GLS_MIRROR_VIEW						= 1ull << 62;
static const uint64_t GLS_OVERRIDE						= 1ull << 63;		// override the render prog state

static const uint64_t GLS_KEEP							= GLS_DEPTH_TEST_MASK;
static const uint64_t GLS_DEFAULT							= 0;

#define STENCIL_SHADOW_TEST_VALUE		128
#define STENCIL_SHADOW_MASK_VALUE		255

#endif /* !__GLSTATE_H__ */
