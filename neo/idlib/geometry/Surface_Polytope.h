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

#ifndef __SURFACE_POLYTOPE_H__
#define __SURFACE_POLYTOPE_H__

/*
===============================================================================

	Polytope surface.

	NOTE: vertexes are not duplicated for texture coordinates.

===============================================================================
*/

class idSurface_Polytope : public idSurface
{
public:
	idSurface_Polytope();
	explicit idSurface_Polytope( const idSurface& surface ) : idSurface( surface ) {}

	void				FromPlanes( const idPlane* planes, const int numPlanes );

	void				SetupTetrahedron( const idBounds& bounds );
	void				SetupHexahedron( const idBounds& bounds );
	void				SetupOctahedron( const idBounds& bounds );
	void				SetupDodecahedron( const idBounds& bounds );
	void				SetupIcosahedron( const idBounds& bounds );
	void				SetupCylinder( const idBounds& bounds, const int numSides );
	void				SetupCone( const idBounds& bounds, const int numSides );

	int					SplitPolytope( const idPlane& plane, const float epsilon, idSurface_Polytope** front, idSurface_Polytope** back ) const;

protected:

};

/*
====================
idSurface_Polytope::idSurface_Polytope
====================
*/
ID_INLINE idSurface_Polytope::idSurface_Polytope()
{
}

#endif /* !__SURFACE_POLYTOPE_H__ */
