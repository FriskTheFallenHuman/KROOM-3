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

#ifndef __SURFACE_H__
#define __SURFACE_H__

/*
===============================================================================

	Surface base class.

	A surface is tesselated to a triangle mesh with each edge shared by
	at most two triangles.

===============================================================================
*/

typedef struct surfaceEdge_s
{
	int						verts[2];	// edge vertices always with ( verts[0] < verts[1] )
	int						tris[2];	// edge triangles
} surfaceEdge_t;


class idSurface
{
public:
	idSurface();
	explicit idSurface( const idSurface& surf );
	explicit idSurface( const idDrawVert* verts, const int numVerts, const int* indexes, const int numIndexes );
	~idSurface();

	const idDrawVert& 		operator[]( const int index ) const;
	idDrawVert& 			operator[]( const int index );
	idSurface& 				operator+=( const idSurface& surf );

	int						GetNumIndexes() const
	{
		return indexes.Num();
	}
	const int* 				GetIndexes() const
	{
		return indexes.Ptr();
	}
	int						GetNumVertices() const
	{
		return verts.Num();
	}
	const idDrawVert* 		GetVertices() const
	{
		return verts.Ptr();
	}
	const int* 				GetEdgeIndexes() const
	{
		return edgeIndexes.Ptr();
	}
	const surfaceEdge_t* 	GetEdges() const
	{
		return edges.Ptr();
	}

	void					Clear();
	void					TranslateSelf( const idVec3& translation );
	void					RotateSelf( const idMat3& rotation );

	// splits the surface into a front and back surface, the surface itself stays unchanged
	// frontOnPlaneEdges and backOnPlaneEdges optionally store the indexes to the edges that lay on the split plane
	// returns a SIDE_?
	int						Split( const idPlane& plane, const float epsilon, idSurface** front, idSurface** back, int* frontOnPlaneEdges = NULL, int* backOnPlaneEdges = NULL ) const;
	// cuts off the part at the back side of the plane, returns true if some part was at the front
	// if there is nothing at the front the number of points is set to zero
	bool					ClipInPlace( const idPlane& plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

	// returns true if each triangle can be reached from any other triangle by a traversal
	bool					IsConnected() const;
	// returns true if the surface is closed
	bool					IsClosed() const;
	// returns true if the surface is a convex hull
	bool					IsPolytope( const float epsilon = 0.1f ) const;

	float					PlaneDistance( const idPlane& plane ) const;
	int						PlaneSide( const idPlane& plane, const float epsilon = ON_EPSILON ) const;

	// returns true if the line intersects one of the surface triangles
	bool					LineIntersection( const idVec3& start, const idVec3& end, bool backFaceCull = false ) const;
	// intersection point is start + dir * scale
	bool					RayIntersection( const idVec3& start, const idVec3& dir, float& scale, bool backFaceCull = false ) const;

protected:
	idList<idDrawVert>		verts;			// vertices
	idList<int>				indexes;		// 3 references to vertices for each triangle
	idList<surfaceEdge_t>	edges;			// edges
	idList<int>				edgeIndexes;	// 3 references to edges for each triangle, may be negative for reversed edge

protected:
	void					GenerateEdgeIndexes();
	int						FindEdge( int v1, int v2 ) const;
};

/*
====================
idSurface::idSurface
====================
*/
ID_INLINE idSurface::idSurface()
{
}

/*
=================
idSurface::idSurface
=================
*/
ID_INLINE idSurface::idSurface( const idDrawVert* verts, const int numVerts, const int* indexes, const int numIndexes )
{
	assert( verts != NULL && indexes != NULL && numVerts > 0 && numIndexes > 0 );
	this->verts.SetNum( numVerts );
	for( int i = 0; i < numVerts; i++ )
	{
		this->verts[i] = verts[i];
	}
	this->indexes.SetNum( numIndexes );
	memcpy( this->indexes.Ptr(), indexes, numIndexes * sizeof( indexes[0] ) );
	GenerateEdgeIndexes();
}

/*
====================
idSurface::idSurface
====================
*/
ID_INLINE idSurface::idSurface( const idSurface& surf )
{
	this->verts = surf.verts;
	this->indexes = surf.indexes;
	this->edges = surf.edges;
	this->edgeIndexes = surf.edgeIndexes;
}

/*
====================
idSurface::~idSurface
====================
*/
ID_INLINE idSurface::~idSurface()
{
}

/*
=================
idSurface::operator[]
=================
*/
ID_INLINE const idDrawVert& idSurface::operator[]( const int index ) const
{
	return verts[ index ];
};

/*
=================
idSurface::operator[]
=================
*/
ID_INLINE idDrawVert& idSurface::operator[]( const int index )
{
	return verts[ index ];
};

/*
=================
idSurface::operator+=
=================
*/
ID_INLINE idSurface& idSurface::operator+=( const idSurface& surf )
{
	int i, m, n;
	n = verts.Num();
	m = indexes.Num();
	verts.Append( surf.verts );			// merge verts where possible ?
	indexes.Append( surf.indexes );
	for( i = m; i < indexes.Num(); i++ )
	{
		indexes[i] += n;
	}
	GenerateEdgeIndexes();
	return *this;
}

/*
=================
idSurface::Clear
=================
*/
ID_INLINE void idSurface::Clear()
{
	verts.Clear();
	indexes.Clear();
	edges.Clear();
	edgeIndexes.Clear();
}

/*
=================
idSurface::TranslateSelf
=================
*/
ID_INLINE void idSurface::TranslateSelf( const idVec3& translation )
{
	for( int i = 0; i < verts.Num(); i++ )
	{
		verts[i].xyz += translation;
	}
}

/*
=================
idSurface::RotateSelf
=================
*/
ID_INLINE void idSurface::RotateSelf( const idMat3& rotation )
{
	for( int i = 0; i < verts.Num(); i++ )
	{
		verts[i].xyz *= rotation;
		verts[i].SetNormal( verts[i].GetNormal() * rotation );
		verts[i].SetTangent( verts[i].GetTangent() * rotation );
	}
}

#endif /* !__SURFACE_H__ */
