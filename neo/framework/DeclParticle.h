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

#ifndef __DECLPARTICLE_H__
#define __DECLPARTICLE_H__

/*
===============================================================================

	idDeclParticle

===============================================================================
*/

static const int MAX_PARTICLE_STAGES	= 32;

class idParticleParm
{
public:
	idParticleParm()
	{
		table = NULL;
		from = to = 0.0f;
	}

	const idDeclTable* 		table;
	float					from;
	float					to;

	float					Eval( float frac, idRandom& rand ) const;
	float					Integrate( float frac, idRandom& rand ) const;
};


typedef enum
{
	PDIST_RECT,				// ( sizeX sizeY sizeZ )
	PDIST_CYLINDER,			// ( sizeX sizeY sizeZ )
	PDIST_SPHERE			// ( sizeX sizeY sizeZ ringFraction )
	// a ringFraction of zero allows the entire sphere, 0.9 would only
	// allow the outer 10% of the sphere
} prtDistribution_t;

typedef enum
{
	PDIR_CONE,				// parm0 is the solid cone angle
	PDIR_OUTWARD			// direction is relative to offset from origin, parm0 is an upward bias
} prtDirection_t;

typedef enum
{
	PPATH_STANDARD,
	PPATH_HELIX,			// ( sizeX sizeY sizeZ radialSpeed climbSpeed )
	PPATH_FLIES,
	PPATH_ORBIT,
	PPATH_DRIP
} prtCustomPth_t;

typedef enum
{
	POR_VIEW,
	POR_AIMED,				// angle and aspect are disregarded
	POR_X,
	POR_Y,
	POR_Z
} prtOrientation_t;

typedef struct renderEntity_s renderEntity_t;
typedef struct renderView_s renderView_t;

typedef struct
{
	const renderEntity_t* 	renderEnt;			// for shaderParms, etc
	const renderView_t* 	renderView;
	int						index;				// particle number in the system
	float					frac;				// 0.0 to 1.0
	idRandom				random;
	idVec3					origin;				// dynamic smoke particles can have individual origins and axis
	idMat3					axis;


	float					age;				// in seconds, calculated as fraction * stage->particleLife
	idRandom				originalRandom;		// needed so aimed particles can reset the random for another origin calculation
	float					animationFrameFrac;	// set by ParticleTexCoords, used to make the cross faded version
} particleGen_t;


//
// single particle stage
//
class idParticleStage
{
public:
	idParticleStage();
	~idParticleStage() {}

	void					Default();
	virtual int				NumQuadsPerParticle() const;	// includes trails and cross faded animations
	// returns the number of verts created, which will range from 0 to 4*NumQuadsPerParticle()
	virtual int				CreateParticle( particleGen_t* g, idDrawVert* verts ) const;

	void					ParticleOrigin( particleGen_t* g, idVec3& origin ) const;
	int						ParticleVerts( particleGen_t* g, const idVec3 origin, idDrawVert* verts ) const;
	void					ParticleTexCoords( particleGen_t* g, idDrawVert* verts ) const;
	void					ParticleColors( particleGen_t* g, idDrawVert* verts ) const;

	const char* 			GetCustomPathName();
	const char* 			GetCustomPathDesc();
	int						NumCustomPathParms();
	void					SetCustomPathType( const char* p );
	void					operator=( const idParticleStage& src );


	//------------------------------

	const idMaterial* 		material;

	int						totalParticles;		// total number of particles, although some may be invisible at a given time
	float					cycles;				// allows things to oneShot ( 1 cycle ) or run for a set number of cycles
	// on a per stage basis

	int						cycleMsec;			// ( particleLife + deadTime ) in msec

	float					spawnBunching;		// 0.0 = all come out at first instant, 1.0 = evenly spaced over cycle time
	float					particleLife;		// total seconds of life for each particle
	float					timeOffset;			// time offset from system start for the first particle to spawn
	float					deadTime;			// time after particleLife before respawning

	//-------------------------------	// standard path parms

	prtDistribution_t		distributionType;
	float					distributionParms[4];

	prtDirection_t			directionType;
	float					directionParms[4];

	idParticleParm			speed;
	float					gravity;				// can be negative to float up
	bool					worldGravity;			// apply gravity in world space
	bool					randomDistribution;		// randomly orient the quad on emission ( defaults to true )
	bool					entityColor;			// force color from render entity ( fadeColor is still valid )

	//------------------------------	// custom path will completely replace the standard path calculations

	prtCustomPth_t			customPathType;		// use custom C code routines for determining the origin
	float					customPathParms[8];

	//--------------------------------

	idVec3					offset;				// offset from origin to spawn all particles, also applies to customPath

	int						animationFrames;	// if > 1, subdivide the texture S axis into frames and crossfade
	float					animationRate;		// frames per second

	float					initialAngle;		// in degrees, random angle is used if zero ( default )
	idParticleParm			rotationSpeed;		// half the particles will have negative rotation speeds

	prtOrientation_t		orientation;	// view, aimed, or axis fixed
	float					orientationParms[4];

	idParticleParm			size;
	idParticleParm			aspect;				// greater than 1 makes the T axis longer

	idVec4					color;
	idVec4					fadeColor;			// either 0 0 0 0 for additive, or 1 1 1 0 for blended materials
	float					fadeInFraction;		// in 0.0 to 1.0 range
	float					fadeOutFraction;	// in 0.0 to 1.0 range
	float					fadeIndexFraction;	// in 0.0 to 1.0 range, causes later index smokes to be more faded

	bool					hidden;				// for editor use
	//-----------------------------------

	float					boundsExpansion;	// user tweak to fix poorly calculated bounds

	idBounds				bounds;				// derived
};

/*
================
idParticleStage::NumQuadsPerParticle

includes trails and cross faded animations
================
*/
ID_INLINE int idParticleStage::NumQuadsPerParticle() const
{
	int	count = 1;

	if( orientation == POR_AIMED )
	{
		int	trails = idMath::Ftoi( orientationParms[0] );
		// each trail stage will add an extra quad
		count *= ( 1 + trails );
	}

	// if we are doing strip-animation, we need to double the number and cross fade them
	if( animationFrames > 1 )
	{
		count *= 2;
	}

	return count;
}

/*
================
idParticleStage::CreateParticle

Returns 0 if no particle is created because it is completely faded out
Returns 4 if a normal quad is created
Returns 8 if two cross faded quads are created

Vertex order is:

0 1
2 3
================
*/
ID_INLINE int idParticleStage::CreateParticle( particleGen_t* g, idDrawVert* verts ) const
{
	idVec3	origin;

	verts[0].Clear();
	verts[1].Clear();
	verts[2].Clear();
	verts[3].Clear();

	ParticleColors( g, verts );

	// if we are completely faded out, kill the particle
	if( verts[0].color[0] == 0 && verts[0].color[1] == 0 && verts[0].color[2] == 0 && verts[0].color[3] == 0 )
	{
		return 0;
	}

	ParticleOrigin( g, origin );

	ParticleTexCoords( g, verts );

	int	numVerts = ParticleVerts( g, origin, verts );

	if( animationFrames <= 1 )
	{
		return numVerts;
	}

	// if we are doing strip-animation, we need to double the quad and cross fade it
	float	width = 1.0f / animationFrames;
	float	frac = g->animationFrameFrac;
	float	iFrac = 1.0f - frac;

	idVec2 tempST;
	for( int i = 0 ; i < numVerts ; i++ )
	{
		verts[numVerts + i] = verts[i];

		tempST = verts[numVerts + i].GetTexCoord();
		verts[numVerts + i].SetTexCoord( tempST.x + width, tempST.y );

		verts[numVerts + i].color[0] *= frac;
		verts[numVerts + i].color[1] *= frac;
		verts[numVerts + i].color[2] *= frac;
		verts[numVerts + i].color[3] *= frac;

		verts[i].color[0] *= iFrac;
		verts[i].color[1] *= iFrac;
		verts[i].color[2] *= iFrac;
		verts[i].color[3] *= iFrac;
	}

	return numVerts * 2;
}

//
// group of particle stages
//
class idDeclParticle : public idDecl
{
public:

	virtual size_t			Size() const;
	virtual const char* 	DefaultDefinition() const;
	virtual bool			Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	bool					Save( const char* fileName = NULL );

	// Loaded instead of re-parsing, written if MD5 hash different
	bool					LoadBinary( idFile* file, unsigned int checksum );
	void					WriteBinary( idFile* file, unsigned int checksum );

	idList<idParticleStage*>stages;
	idBounds				bounds;
	float					depthHack;

private:
	bool					RebuildTextSource();
	void					GetStageBounds( idParticleStage* stage );
	idParticleStage* 		ParseParticleStage( idLexer& src );
	void					ParseParms( idLexer& src, float* parms, int maxParms );
	void					ParseParametric( idLexer& src, idParticleParm* parm );
	void					WriteStage( idFile* f, idParticleStage* stage );
	void					WriteParticleParm( idFile* f, idParticleParm* parm, const char* name );
};

#endif /* !__DECLPARTICLE_H__ */
