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

#include "precompiled.h"
#pragma hdrstop


/*
===============
FloatCRC
===============
*/
ID_INLINE unsigned int FloatCRC( float f )
{
	return *( unsigned int* )&f;
}

/*
===============
StringCRC
===============
*/
ID_INLINE unsigned int StringCRC( const char* str )
{
	unsigned int i, crc;
	const unsigned char* ptr;

	crc = 0;
	ptr = reinterpret_cast<const unsigned char*>( str );
	for( i = 0; str[i]; i++ )
	{
		crc ^= str[i] << ( i & 3 );
	}
	return crc;
}

/*
=================
ComputeAxisBase

WARNING : special case behaviour of atan2(y,x) <-> atan(y/x) might not be the same everywhere when x == 0
rotation by (0,RotY,RotZ) assigns X to normal
=================
*/
static void ComputeAxisBase( const idVec3& normal, idVec3& texS, idVec3& texT )
{
	float RotY, RotZ;
	idVec3 n;

	// do some cleaning
	n[0] = ( idMath::Fabs( normal[0] ) < 1e-6f ) ? 0.0f : normal[0];
	n[1] = ( idMath::Fabs( normal[1] ) < 1e-6f ) ? 0.0f : normal[1];
	n[2] = ( idMath::Fabs( normal[2] ) < 1e-6f ) ? 0.0f : normal[2];

	RotY = -atan2( n[2], idMath::Sqrt( n[1] * n[1] + n[0] * n[0] ) );
	RotZ = atan2( n[1], n[0] );
	// rotate (0,1,0) and (0,0,1) to compute texS and texT
	texS[0] = -sin( RotZ );
	texS[1] = cos( RotZ );
	texS[2] = 0;
	// the texT vector is along -Z ( T texture coorinates axis )
	texT[0] = -sin( RotY ) * cos( RotZ );
	texT[1] = -sin( RotY ) * sin( RotZ );
	texT[2] = -cos( RotY );
}

/*
=================
idMapBrushSide::GetTextureVectors
=================
*/
void idMapBrushSide::GetTextureVectors( idVec4 v[2] ) const
{
	idVec3 texX, texY;

	ComputeAxisBase( plane.Normal(), texX, texY );
	for( int i = 0; i < 2; i++ )
	{
		v[i][0] = texX[0] * texMat[i][0] + texY[0] * texMat[i][1];
		v[i][1] = texX[1] * texMat[i][0] + texY[1] * texMat[i][1];
		v[i][2] = texX[2] * texMat[i][0] + texY[2] * texMat[i][1];
		v[i][3] = texMat[i][2] + ( origin * v[i].ToVec3() );
	}
}

/*
=================
idMapPatch::Parse
=================
*/
idMapPatch* idMapPatch::Parse( idLexer& src, const idVec3& origin, bool patchDef3, float version )
{
	float		info[7];
	idDrawVert* vert;
	idToken		token;
	int			i, j;

	if( !src.ExpectTokenString( "{" ) )
	{
		return NULL;
	}

	// read the material (we had an implicit 'textures/' in the old format...)
	if( !src.ReadToken( &token ) )
	{
		src.Error( "idMapPatch::Parse: unexpected EOF" );
		return NULL;
	}

	// Parse it
	if( patchDef3 )
	{
		if( !src.Parse1DMatrix( 7, info ) )
		{
			src.Error( "idMapPatch::Parse: unable to Parse patchDef3 info" );
			return NULL;
		}
	}
	else
	{
		if( !src.Parse1DMatrix( 5, info ) )
		{
			src.Error( "idMapPatch::Parse: unable to parse patchDef2 info" );
			return NULL;
		}
	}

	idMapPatch* patch = new( TAG_IDLIB ) idMapPatch( info[0], info[1] );

	patch->SetSize( info[0], info[1] );
	if( version < 2.0f )
	{
		patch->SetMaterial( "textures/" + token );
	}
	else
	{
		patch->SetMaterial( token );
	}

	if( patchDef3 )
	{
		patch->SetHorzSubdivisions( info[2] );
		patch->SetVertSubdivisions( info[3] );
		patch->SetExplicitlySubdivided( true );
	}

	if( patch->GetWidth() < 0 || patch->GetHeight() < 0 )
	{
		src.Error( "idMapPatch::Parse: bad size" );
		delete patch;
		return NULL;
	}

	// these were written out in the wrong order, IMHO
	if( !src.ExpectTokenString( "(" ) )
	{
		src.Error( "idMapPatch::Parse: bad patch vertex data" );
		delete patch;
		return NULL;
	}


	for( j = 0; j < patch->GetWidth(); j++ )
	{
		if( !src.ExpectTokenString( "(" ) )
		{
			src.Error( "idMapPatch::Parse: bad vertex row data" );
			delete patch;
			return NULL;
		}
		for( i = 0; i < patch->GetHeight(); i++ )
		{
			float v[5];

			if( !src.Parse1DMatrix( 5, v ) )
			{
				src.Error( "idMapPatch::Parse: bad vertex column data" );
				delete patch;
				return NULL;
			}

			vert = &( ( *patch )[i * patch->GetWidth() + j] );
			vert->xyz[0] = v[0] - origin[0];
			vert->xyz[1] = v[1] - origin[1];
			vert->xyz[2] = v[2] - origin[2];
			vert->SetTexCoord( v[3], v[4] );
		}
		if( !src.ExpectTokenString( ")" ) )
		{
			delete patch;
			src.Error( "idMapPatch::Parse: unable to parse patch control points" );
			return NULL;
		}
	}

	if( !src.ExpectTokenString( ")" ) )
	{
		src.Error( "idMapPatch::Parse: unable to parse patch control points, no closure" );
		delete patch;
		return NULL;
	}

	// read any key/value pairs
	while( src.ReadToken( &token ) )
	{
		if( token == "}" )
		{
			src.ExpectTokenString( "}" );
			break;
		}
		if( token.type == TT_STRING )
		{
			idStr key = token;
			src.ExpectTokenType( TT_STRING, 0, &token );
			patch->epairs.Set( key, token );
		}
	}

	return patch;
}

/*
============
idMapPatch::Write
============
*/
bool idMapPatch::Write( idFile* fp, int primitiveNum, const idVec3& origin ) const
{
	int i, j;
	const idDrawVert* v;

	if( GetExplicitlySubdivided() )
	{
		fp->WriteFloatString( "// primitive %d\n{\n patchDef3\n {\n", primitiveNum );
		fp->WriteFloatString( "  \"%s\"\n  ( %d %d %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight(), GetHorzSubdivisions(), GetVertSubdivisions() );
	}
	else
	{
		fp->WriteFloatString( "// primitive %d\n{\n patchDef2\n {\n", primitiveNum );
		fp->WriteFloatString( "  \"%s\"\n  ( %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight() );
	}

	fp->WriteFloatString( "  (\n" );
	idVec2 st;
	for( i = 0; i < GetWidth(); i++ )
	{
		fp->WriteFloatString( "   ( " );
		for( j = 0; j < GetHeight(); j++ )
		{
			v = &verts[ j * GetWidth() + i ];
			st = v->GetTexCoord();
			fp->WriteFloatString( " ( %f %f %f %f %f )", v->xyz[0] + origin[0],
								  v->xyz[1] + origin[1], v->xyz[2] + origin[2], st[0], st[1] );
		}
		fp->WriteFloatString( " )\n" );
	}
	fp->WriteFloatString( "  )\n }\n}\n" );

	return true;
}

/*
===============
idMapPatch::GetGeometryCRC
===============
*/
unsigned int idMapPatch::GetGeometryCRC() const
{
	int i, j;
	unsigned int crc;

	crc = GetHorzSubdivisions() ^ GetVertSubdivisions();
	for( i = 0; i < GetWidth(); i++ )
	{
		for( j = 0; j < GetHeight(); j++ )
		{
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.x );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.y );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.z );
		}
	}

	crc ^= StringCRC( GetMaterial() );

	return crc;
}

/*
=================
idMapBrush::Parse
=================
*/
idMapBrush* idMapBrush::Parse( idLexer& src, const idVec3& origin, bool newFormat, float version )
{
	int i;
	idVec3 planepts[3];
	idToken token;
	idList<idMapBrushSide*> sides;
	idMapBrushSide*	side;
	idDict epairs;

	if( !src.ExpectTokenString( "{" ) )
	{
		return NULL;
	}

	do
	{
		if( !src.ReadToken( &token ) )
		{
			src.Error( "idMapBrush::Parse: unexpected EOF" );
			sides.DeleteContents( true );
			return NULL;
		}
		if( token == "}" )
		{
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do
		{
			// if token is a brace
			if( token == "(" )
			{
				break;
			}
			// the token should be a key string for a key/value pair
			if( token.type != TT_STRING )
			{
				src.Error( "idMapBrush::Parse: unexpected %s, expected ( or epair key string", token.c_str() );
				sides.DeleteContents( true );
				return NULL;
			}

			idStr key = token;

			if( !src.ReadTokenOnLine( &token ) || token.type != TT_STRING )
			{
				src.Error( "idMapBrush::Parse: expected epair value string not found" );
				sides.DeleteContents( true );
				return NULL;
			}

			epairs.Set( key, token );

			// try to read the next key
			if( !src.ReadToken( &token ) )
			{
				src.Error( "idMapBrush::Parse: unexpected EOF" );
				sides.DeleteContents( true );
				return NULL;
			}
		}
		while( 1 );

		src.UnreadToken( &token );

		side = new( TAG_IDLIB ) idMapBrushSide();
		sides.Append( side );

		if( newFormat )
		{
			if( !src.Parse1DMatrix( 4, side->plane.ToFloatPtr() ) )
			{
				src.Error( "idMapBrush::Parse: unable to read brush side plane definition" );
				sides.DeleteContents( true );
				return NULL;
			}
		}
		else
		{
			// read the three point plane definition
			if( !src.Parse1DMatrix( 3, planepts[0].ToFloatPtr() ) ||
					!src.Parse1DMatrix( 3, planepts[1].ToFloatPtr() ) ||
					!src.Parse1DMatrix( 3, planepts[2].ToFloatPtr() ) )
			{
				src.Error( "idMapBrush::Parse: unable to read brush side plane definition" );
				sides.DeleteContents( true );
				return NULL;
			}

			planepts[0] -= origin;
			planepts[1] -= origin;
			planepts[2] -= origin;

			side->plane.FromPoints( planepts[0], planepts[1], planepts[2] );
		}

		// read the texture matrix
		// this is odd, because the texmat is 2D relative to default planar texture axis
		if( !src.Parse2DMatrix( 2, 3, side->texMat[0].ToFloatPtr() ) )
		{
			src.Error( "idMapBrush::Parse: unable to read brush side texture matrix" );
			sides.DeleteContents( true );
			return NULL;
		}
		side->origin = origin;

		// read the material
		if( !src.ReadTokenOnLine( &token ) )
		{
			src.Error( "idMapBrush::Parse: unable to read brush side material" );
			sides.DeleteContents( true );
			return NULL;
		}

		// we had an implicit 'textures/' in the old format...
		if( version < 2.0f )
		{
			side->material = "textures/" + token;
		}
		else
		{
			side->material = token;
		}

		// Q2 allowed override of default flags and values, but we don't any more
		if( src.ReadTokenOnLine( &token ) )
		{
			if( src.ReadTokenOnLine( &token ) )
			{
				if( src.ReadTokenOnLine( &token ) )
				{
				}
			}
		}
	}
	while( 1 );

	if( !src.ExpectTokenString( "}" ) )
	{
		sides.DeleteContents( true );
		return NULL;
	}

	idMapBrush* brush = new( TAG_IDLIB ) idMapBrush();
	for( i = 0; i < sides.Num(); i++ )
	{
		brush->AddSide( sides[i] );
	}

	brush->epairs = epairs;

	return brush;
}

/*
=================
idMapBrush::ParseQ3
=================
*/
idMapBrush* idMapBrush::ParseQ3( idLexer& src, const idVec3& origin )
{
	int i, shift[2], rotate;
	float scale[2];
	idVec3 planepts[3];
	idToken token;
	idList<idMapBrushSide*> sides;
	idMapBrushSide*	side;
	idDict epairs;

	do
	{
		if( src.CheckTokenString( "}" ) )
		{
			break;
		}

		side = new( TAG_IDLIB ) idMapBrushSide();
		sides.Append( side );

		// read the three point plane definition
		if( !src.Parse1DMatrix( 3, planepts[0].ToFloatPtr() ) ||
				!src.Parse1DMatrix( 3, planepts[1].ToFloatPtr() ) ||
				!src.Parse1DMatrix( 3, planepts[2].ToFloatPtr() ) )
		{
			src.Error( "idMapBrush::ParseQ3: unable to read brush side plane definition" );
			sides.DeleteContents( true );
			return NULL;
		}

		planepts[0] -= origin;
		planepts[1] -= origin;
		planepts[2] -= origin;

		side->plane.FromPoints( planepts[0], planepts[1], planepts[2] );

		// read the material
		if( !src.ReadTokenOnLine( &token ) )
		{
			src.Error( "idMapBrush::ParseQ3: unable to read brush side material" );
			sides.DeleteContents( true );
			return NULL;
		}

		// we have an implicit 'textures/' in the old format
		side->material = "textures/" + token;

		// read the texture shift, rotate and scale
		shift[0] = src.ParseInt();
		shift[1] = src.ParseInt();
		rotate = src.ParseInt();
		scale[0] = src.ParseFloat();
		scale[1] = src.ParseFloat();
		side->texMat[0] = idVec3( 0.03125f, 0.0f, 0.0f );
		side->texMat[1] = idVec3( 0.0f, 0.03125f, 0.0f );
		side->origin = origin;

		// Q2 allowed override of default flags and values, but we don't any more
		if( src.ReadTokenOnLine( &token ) )
		{
			if( src.ReadTokenOnLine( &token ) )
			{
				if( src.ReadTokenOnLine( &token ) )
				{
				}
			}
		}
	}
	while( 1 );

	idMapBrush* brush = new( TAG_IDLIB ) idMapBrush();
	for( i = 0; i < sides.Num(); i++ )
	{
		brush->AddSide( sides[i] );
	}

	brush->epairs = epairs;

	return brush;
}

/*
============
idMapBrush::Write
============
*/
bool idMapBrush::Write( idFile* fp, int primitiveNum, const idVec3& origin ) const
{
	int i;
	idMapBrushSide* side;

	fp->WriteFloatString( "// primitive %d\n{\n brushDef3\n {\n", primitiveNum );

	// write brush epairs
	for( i = 0; i < epairs.GetNumKeyVals(); i++ )
	{
		fp->WriteFloatString( "  \"%s\" \"%s\"\n", epairs.GetKeyVal( i )->GetKey().c_str(), epairs.GetKeyVal( i )->GetValue().c_str() );
	}

	// write brush sides
	for( i = 0; i < GetNumSides(); i++ )
	{
		side = GetSide( i );
		fp->WriteFloatString( "  ( %f %f %f %f ) ", side->plane[0], side->plane[1], side->plane[2], side->plane[3] );
		fp->WriteFloatString( "( ( %f %f %f ) ( %f %f %f ) ) \"%s\" 0 0 0\n",
							  side->texMat[0][0], side->texMat[0][1], side->texMat[0][2],
							  side->texMat[1][0], side->texMat[1][1], side->texMat[1][2],
							  side->material.c_str() );
	}

	fp->WriteFloatString( " }\n}\n" );

	return true;
}

/*
===============
idMapBrush::GetGeometryCRC
===============
*/
unsigned int idMapBrush::GetGeometryCRC() const
{
	int i, j;
	idMapBrushSide* mapSide;
	unsigned int crc;

	crc = 0;
	for( i = 0; i < GetNumSides(); i++ )
	{
		mapSide = GetSide( i );
		for( j = 0; j < 4; j++ )
		{
			crc ^= FloatCRC( mapSide->GetPlane()[j] );
		}
		crc ^= StringCRC( mapSide->GetMaterial() );
	}

	return crc;
}

/*
================
idMapPolygonMesh::ConvertFromBrush
================
*/
void idMapPolygonMesh::ConvertFromBrush( const idMapBrush* mapBrush, int entityNum, int primitiveNum )
{
	originalType = TYPE_BRUSH;

	// fix degenerate planes
	idPlane* planes = ( idPlane* ) _alloca16( mapBrush->GetNumSides() * sizeof( planes[0] ) );
	for( int i = 0; i < mapBrush->GetNumSides(); i++ )
	{
		planes[i] = mapBrush->GetSide( i )->GetPlane();
		planes[i].FixDegeneracies( DEGENERATE_DIST_EPSILON );
	}

	idList<idFixedWinding> planeWindings;
	idBounds bounds;
	bounds.Clear();

	int numVerts = 0;
	int numIndexes = 0;

	bool badBrush = false;

	for( int i = 0; i < mapBrush->GetNumSides(); i++ )
	{
		idMapBrushSide* mapSide = mapBrush->GetSide( i );

		const idMaterial* material = declManager->FindMaterial( mapSide->GetMaterial() );
		//contents |= ( material->GetContentFlags() & CONTENTS_REMOVE_UTIL );
		//materials.AddUnique( material );

		// chop base plane by other brush sides
		idFixedWinding& w = planeWindings.Alloc();
		w.BaseForPlane( -planes[i] );

		if( !w.GetNumPoints() )
		{
			common->Printf( "Entity %i, Brush %i: base winding has no points\n", entityNum, primitiveNum );
			badBrush = true;
			break;
		}

		for( int j = 0; j < mapBrush->GetNumSides() && w.GetNumPoints(); j++ )
		{
			if( i == j )
			{
				continue;
			}

			if( !w.ClipInPlace( -planes[j], 0 ) )
			{
				// no intersection
				//badBrush = true;
				common->Printf( "Entity %i, Brush %i: no intersection with other brush plane\n", entityNum, primitiveNum );
				//break;
			}
		}

		if( w.GetNumPoints() <= 2 )
		{
			continue;
		}

		// only used for debugging
		for( int j = 0; j < w.GetNumPoints(); j++ )
		{
			const idVec3& v = w[j].ToVec3();
			bounds.AddPoint( v );
		}
	}

	if( badBrush )
	{
		//common->Error( "" )
		return;
	}

	// copy the data from the windings and build polygons
	for( int i = 0; i < mapBrush->GetNumSides(); i++ )
	{
		idMapBrushSide* mapSide = mapBrush->GetSide( i );

		idFixedWinding& w = planeWindings[i];
		if( !w.GetNumPoints() )
		{
			continue;
		}

		idMapPolygon& polygon = polygons.Alloc();
		polygon.SetMaterial( mapSide->GetMaterial() );


		//for( int j = 0; j < w.GetNumPoints(); j++ )

		// reverse order, so normal does not point inwards
		for( int j = w.GetNumPoints() - 1; j >= 0; j-- )
		{
			polygon.AddIndex( verts.Num() + j );
		}

		for( int j = 0; j < w.GetNumPoints(); j++ )
		{
			idDrawVert& dv = verts.Alloc();

			const idVec3& xyz = w[j].ToVec3();

			dv.xyz = xyz;

			// calculate texture s/t from brush primitive texture matrix
			idVec4 texVec[2];
			mapSide->GetTextureVectors( texVec );

			idVec2 st;
			st.x = ( xyz * texVec[0].ToVec3() ) + texVec[0][3];
			st.y = ( xyz * texVec[1].ToVec3() ) + texVec[1][3];

			// flip y
			//st.y = 1.0f - st.y;

			dv.SetTexCoord( st );

			// copy normal
			dv.SetNormal( mapSide->GetPlane().Normal() );

			//if( dv->GetNormal().Length() < 0.9 || dv->GetNormal().Length() > 1.1 )
			//{
			//	common->Error( "Bad normal in TriListForSide" );
			//}
		}
	}

	SetContents();
}

/*
================
idMapPolygonMesh::ConvertFromPatch
================
*/
void idMapPolygonMesh::ConvertFromPatch( const idMapPatch* patch, int entityNum, int primitiveNum )
{
	originalType = TYPE_PATCH;

	idSurface_Patch* cp = new idSurface_Patch( *patch );

	if( patch->GetExplicitlySubdivided() )
	{
		cp->SubdivideExplicit( patch->GetHorzSubdivisions(), patch->GetVertSubdivisions(), true );
	}
	else
	{
		cp->Subdivide( DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_LENGTH, true );
	}

	for( int i = 0; i < cp->GetNumIndexes(); i += 3 )
	{
		verts.Append( ( *cp )[cp->GetIndexes()[i + 1]] );
		verts.Append( ( *cp )[cp->GetIndexes()[i + 2]] );
		verts.Append( ( *cp )[cp->GetIndexes()[i + 0]] );
	}

	for( int i = 0; i < cp->GetNumIndexes(); i += 3 )
	{
		idMapPolygon& polygon = polygons.Alloc();
		polygon.SetMaterial( patch->GetMaterial() );

		polygon.AddIndex( i + 0 );
		polygon.AddIndex( i + 1 );
		polygon.AddIndex( i + 2 );
	}

	delete cp;

	SetContents();

	// RB: patch meshes are not opaque
	// avoid BSP splits caused by patches
	opaque = false;
}

/*
================
idMapPolygonMesh::Write
================
*/
bool idMapPolygonMesh::Write( idFile* fp, int primitiveNum, const idVec3& origin ) const
{
	fp->WriteFloatString( "// primitive %d\n{\n meshDef\n {\n", primitiveNum );
	//fp->WriteFloatString( "  \"%s\"\n  ( %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight() );

	fp->WriteFloatString( "  ( %d %d 0 0 0 )\n", verts.Num(), polygons.Num() );

	fp->WriteFloatString( "  (\n" );
	idVec2 st;
	idVec3 n;
	for( int i = 0; i < verts.Num(); i++ )
	{
		const idDrawVert* v = &verts[ i ];
		st = v->GetTexCoord();
		n = v->GetNormalRaw();

		//fp->WriteFloatString( "   ( %f %f %f %f %f %f %f %f )\n", v->xyz[0] + origin[0], v->xyz[1] + origin[1], v->xyz[2] + origin[2], st[0], st[1], n[0], n[1], n[2] );
		fp->WriteFloatString( "   ( %f %f %f %f %f %f %f %f )\n", v->xyz[0], v->xyz[1], v->xyz[2], st[0], st[1], n[0], n[1], n[2] );
	}
	fp->WriteFloatString( "  )\n" );

	fp->WriteFloatString( "  (\n" );
	for( int i = 0; i < polygons.Num(); i++ )
	{
		const idMapPolygon& poly = polygons[ i ];

		fp->WriteFloatString( "   \"%s\" %d = ", poly.GetMaterial(), poly.indexes.Num() );

		for( int j = 0; j < poly.indexes.Num(); j++ )
		{
			fp->WriteFloatString( "%d ", poly.indexes[j] );
		}
		fp->WriteFloatString( "\n" );
	}
	fp->WriteFloatString( "  )\n" );

	fp->WriteFloatString( " }\n}\n" );

	return true;
}

/*
================
idMapPolygonMesh::Parse
================
*/
idMapPolygonMesh* idMapPolygonMesh::Parse( idLexer& src, const idVec3& origin, float version )
{
	float		info[7];
	idToken		token;
	int			i;

	if( !src.ExpectTokenString( "{" ) )
	{
		return NULL;
	}

	// Parse it
	if( !src.Parse1DMatrix( 5, info ) )
	{
		src.Error( "idMapPolygonMesh::Parse: unable to parse meshDef info" );
		return NULL;
	}

	const int numVertices = ( int ) info[0];
	const int numPolygons = ( int ) info[1];

	idMapPolygonMesh* mesh = new idMapPolygonMesh();

	// parse vertices
	if( !src.ExpectTokenString( "(" ) )
	{
		src.Error( "idMapPolygonMesh::Parse: bad mesh vertex data" );
		delete mesh;
		return NULL;
	}

	for( i = 0; i < numVertices; i++ )
	{
		float v[8];

		if( !src.Parse1DMatrix( 8, v ) )
		{
			src.Error( "idMapPolygonMesh::Parse: bad vertex column data" );
			delete mesh;
			return NULL;
		}

		// TODO optimize: preallocate vertices
		//vert = &( ( *patch )[i * patch->GetWidth() + j] );

		idDrawVert vert;

		vert.xyz[0] = v[0];// - origin[0];
		vert.xyz[1] = v[1];// - origin[1];
		vert.xyz[2] = v[2];// - origin[2];
		vert.SetTexCoord( v[3], v[4] );

		idVec3 n( v[5], v[6], v[7] );
		vert.SetNormal( n );

		mesh->AddVertex( vert );
	}

	if( !src.ExpectTokenString( ")" ) )
	{
		delete mesh;
		src.Error( "idMapPolygonMesh::Parse: unable to parse vertices" );
		return NULL;
	}

	// parse polygons
	if( !src.ExpectTokenString( "(" ) )
	{
		src.Error( "idMapPolygonMesh::Parse: bad mesh polygon data" );
		delete mesh;
		return NULL;
	}

	for( i = 0; i < numPolygons; i++ )
	{
		// get material name
		idMapPolygon& polygon = mesh->polygons.Alloc();

		src.ReadToken( &token );
		if( token.type == TT_STRING )
		{
			polygon.SetMaterial( token );
		}
		else
		{
			src.Error( "idMapPolygonMesh::Parse: bad mesh polygon data" );
			delete mesh;
			return NULL;
		}

		int numIndexes = src.ParseInt();

		if( !src.ExpectTokenString( "=" ) )
		{
			src.Error( "idMapPolygonMesh::Parse: bad mesh polygon data" );
			delete mesh;
			return NULL;
		}

		//idTempArray<int> indexes( numIndexes );
		for( int j = 0; j < numIndexes; j++ )
		{
			//indexes[j] = src.ParseInt();

			int index = src.ParseInt();
			polygon.AddIndex( index );
		}

		//polygon->SetIndexes( indexes );
	}

	if( !src.ExpectTokenString( ")" ) )
	{
		delete mesh;
		src.Error( "idMapPolygonMesh::Parse: unable to parse polygons" );
		return NULL;
	}

	if( !src.ExpectTokenString( "}" ) )
	{
		delete mesh;
		src.Error( "idMapPolygonMesh::Parse: unable to parse mesh primitive end" );
		return NULL;
	}

	if( !src.ExpectTokenString( "}" ) )
	{
		delete mesh;
		src.Error( "idMapPolygonMesh::Parse: unable to parse mesh primitive end" );
		return NULL;
	}

	mesh->SetContents();

	return mesh;
}

/*
================
idMapPolygonMesh::SetContents
================
*/
void idMapPolygonMesh::SetContents()
{
	if( polygons.Num() < 1 )
	{
		contents = CONTENTS_SOLID;
		opaque = true;

		return;
	}

	int			c2;

	idMapPolygon* poly = &polygons[0];

	const idMaterial* mat = declManager->FindMaterial( poly->GetMaterial() );
	contents = mat->GetContentFlags();

	//b->contentShader = s->material;
	bool mixed = false;

	// a brush is only opaque if all sides are opaque
	opaque = true;

	for( int i = 1 ; i < polygons.Num() ; i++ )
	{
		poly = &polygons[i];

		const idMaterial* mat2 = declManager->FindMaterial( poly->GetMaterial() );

		c2 = mat2->GetContentFlags();
		if( c2 != contents )
		{
			mixed = true;
			contents |= c2;
		}

		if( mat2->Coverage() != MC_OPAQUE )
		{
			opaque = false;
		}
	}
}

/*
================
idMapPolygonMesh::GetGeometryCRC
================
*/
unsigned int idMapPolygonMesh::GetGeometryCRC() const
{
	int i;
	unsigned int crc;

	crc = 0;
	for( i = 0; i < verts.Num(); i++ )
	{
		crc ^= FloatCRC( verts[i].xyz.x );
		crc ^= FloatCRC( verts[i].xyz.y );
		crc ^= FloatCRC( verts[i].xyz.z );
	}

	for( i = 0; i < polygons.Num(); i++ )
	{
		const idMapPolygon& poly = polygons[i];

		crc ^= StringCRC( poly.GetMaterial() );
	}

	return crc;
}

/*
================
idMapPolygonMesh::IsAreaportal
================
*/
bool idMapPolygonMesh::IsAreaportal() const
{
	return ( ( contents & CONTENTS_AREAPORTAL ) != 0 );
}

/*
================
idMapPolygonMesh::GetBounds
================
*/
void idMapPolygonMesh::GetBounds( idBounds& bounds ) const
{
	if( !verts.Num() )
	{
		bounds.Clear();
		return;
	}


	bounds[0] = bounds[1] = verts[0].xyz;
	for( int i = 1; i < verts.Num(); i++ )
	{
		const idVec3& p = verts[i].xyz;

		if( p.x < bounds[0].x )
		{
			bounds[0].x = p.x;
		}
		else if( p.x > bounds[1].x )
		{
			bounds[1].x = p.x;
		}
		if( p.y < bounds[0].y )
		{
			bounds[0].y = p.y;
		}
		else if( p.y > bounds[1].y )
		{
			bounds[1].y = p.y;
		}
		if( p.z < bounds[0].z )
		{
			bounds[0].z = p.z;
		}
		else if( p.z > bounds[1].z )
		{
			bounds[1].z = p.z;
		}
	}
}

/*
================
idMapEntity::Parse
================
*/
idMapEntity* idMapEntity::Parse( idLexer& src, bool worldSpawn, float version )
{
	idToken	token;
	idMapEntity* mapEnt;
	idMapPatch* mapPatch;
	idMapBrush* mapBrush;
	// RB begin
	idMapPolygonMesh* mapMesh;
	// RB end
	bool worldent;
	idVec3 origin;
	double v1, v2, v3;

	if( !src.ReadToken( &token ) )
	{
		return NULL;
	}

	if( token != "{" )
	{
		src.Error( "idMapEntity::Parse: { not found, found %s", token.c_str() );
		return NULL;
	}

	mapEnt = new( TAG_IDLIB ) idMapEntity();

	if( worldSpawn )
	{
		mapEnt->primitives.Resize( 1024, 256 );
	}

	origin.Zero();
	worldent = false;
	do
	{
		if( !src.ReadToken( &token ) )
		{
			src.Error( "idMapEntity::Parse: EOF without closing brace" );
			return NULL;
		}
		if( token == "}" )
		{
			break;
		}

		if( token == "{" )
		{
			// parse a brush or patch
			if( !src.ReadToken( &token ) )
			{
				src.Error( "idMapEntity::Parse: unexpected EOF" );
				return NULL;
			}

			if( worldent )
			{
				origin.Zero();
			}

			// if is it a brush: brush, brushDef, brushDef2, brushDef3
			if( token.Icmpn( "brush", 5 ) == 0 )
			{
				mapBrush = idMapBrush::Parse( src, origin, ( !token.Icmp( "brushDef2" ) || !token.Icmp( "brushDef3" ) ), version );
				if( !mapBrush )
				{
					return NULL;
				}
				mapEnt->AddPrimitive( mapBrush );
			}
			// if is it a patch: patchDef2, patchDef3
			else if( token.Icmpn( "patch", 5 ) == 0 )
			{
				mapPatch = idMapPatch::Parse( src, origin, !token.Icmp( "patchDef3" ), version );
				if( !mapPatch )
				{
					return NULL;
				}
				mapEnt->AddPrimitive( mapPatch );
			}
			// RB: new mesh primitive with ngons
			else if( token.Icmpn( "mesh", 4 ) == 0 )
			{
				mapMesh = idMapPolygonMesh::Parse( src, origin, version );
				if( !mapMesh )
				{
					return NULL;
				}
				mapEnt->AddPrimitive( mapMesh );
			}
			// RB end
			// assume it's a brush in Q3 or older style
			else
			{
				src.UnreadToken( &token );
				mapBrush = idMapBrush::ParseQ3( src, origin );
				if( !mapBrush )
				{
					return NULL;
				}
				mapEnt->AddPrimitive( mapBrush );
			}
		}
		else
		{
			idStr key, value;

			// parse a key / value pair
			key = token;
			src.ReadTokenOnLine( &token );
			value = token;

			// strip trailing spaces that sometimes get accidentally
			// added in the editor
			value.StripTrailingWhitespace();
			key.StripTrailingWhitespace();

			mapEnt->epairs.Set( key, value );

			if( !idStr::Icmp( key, "origin" ) )
			{
				// scanf into doubles, then assign, so it is idVec size independent
				v1 = v2 = v3 = 0;
				sscanf( value, "%lf %lf %lf", &v1, &v2, &v3 );
				origin.x = v1;
				origin.y = v2;
				origin.z = v3;
			}
			else if( !idStr::Icmp( key, "classname" ) && !idStr::Icmp( value, "worldspawn" ) )
			{
				worldent = true;
			}
		}
	}
	while( 1 );

	return mapEnt;
}

/*
============
idMapEntity::Write
============
*/
bool idMapEntity::Write( idFile* fp, int entityNum ) const
{
	int i;
	idMapPrimitive* mapPrim;
	idVec3 origin;

	fp->WriteFloatString( "// entity %d\n{\n", entityNum );

	// write entity epairs
	for( i = 0; i < epairs.GetNumKeyVals(); i++ )
	{
		fp->WriteFloatString( "\"%s\" \"%s\"\n", epairs.GetKeyVal( i )->GetKey().c_str(), epairs.GetKeyVal( i )->GetValue().c_str() );
	}

	epairs.GetVector( "origin", "0 0 0", origin );

	// write pritimives
	for( i = 0; i < GetNumPrimitives(); i++ )
	{
		mapPrim = GetPrimitive( i );

		switch( mapPrim->GetType() )
		{
			case idMapPrimitive::TYPE_BRUSH:
				static_cast<idMapBrush*>( mapPrim )->Write( fp, i, origin );
				break;
			case idMapPrimitive::TYPE_PATCH:
				static_cast<idMapPatch*>( mapPrim )->Write( fp, i, origin );
				break;
			case idMapPrimitive::TYPE_MESH:
				static_cast<idMapPolygonMesh*>( mapPrim )->Write( fp, i, origin );
				break;
		}
	}

	fp->WriteFloatString( "}\n" );

	return true;
}

/*
===============
idMapEntity::RemovePrimitiveData
===============
*/
void idMapEntity::RemovePrimitiveData()
{
	primitives.DeleteContents( true );
}

/*
===============
idMapEntity::GetGeometryCRC
===============
*/
unsigned int idMapEntity::GetGeometryCRC() const
{
	int i;
	unsigned int crc;
	idMapPrimitive*	mapPrim;

	crc = 0;
	for( i = 0; i < GetNumPrimitives(); i++ )
	{
		mapPrim = GetPrimitive( i );

		switch( mapPrim->GetType() )
		{
			case idMapPrimitive::TYPE_BRUSH:
				crc ^= static_cast<idMapBrush*>( mapPrim )->GetGeometryCRC();
				break;
			case idMapPrimitive::TYPE_PATCH:
				crc ^= static_cast<idMapPatch*>( mapPrim )->GetGeometryCRC();
				break;
			case idMapPrimitive::TYPE_MESH:
				crc ^= static_cast<idMapPolygonMesh*>( mapPrim )->GetGeometryCRC();
				break;
		}
	}

	return crc;
}

/*
===============
idMapFile::Parse
===============
*/
bool idMapFile::Parse( const char* filename, bool ignoreRegion, bool osPath )
{
	// no string concatenation for epairs and allow path names for materials
	idLexer src( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
	idToken token;
	idStr fullName;
	idMapEntity* mapEnt;
	int i, j, k;

	name = filename;
	name.StripFileExtension();
	name.StripFileExtension(); // RB: there might be .map.map
	fullName = name;
	hasPrimitiveData = false;

	if( !ignoreRegion )
	{
		// try loading a .reg file first
		fullName.SetFileExtension( "reg" );
		src.LoadFile( fullName, osPath );
	}

	if( !src.IsLoaded() )
	{
		// now try a .map file
		fullName.SetFileExtension( "map" );
		src.LoadFile( fullName, osPath );
		if( !src.IsLoaded() )
		{
			// didn't get anything at all
			return false;
		}
	}

	version = OLD_MAP_VERSION;
	fileTime = src.GetFileTime();
	entities.DeleteContents( true );

	if( src.CheckTokenString( "Version" ) )
	{
		src.ReadTokenOnLine( &token );
		version = token.GetFloatValue();
	}

	while( 1 )
	{
		mapEnt = idMapEntity::Parse( src, ( entities.Num() == 0 ), version );
		if( !mapEnt )
		{
			break;
		}
		entities.Append( mapEnt );
	}

	SetGeometryCRC();

	// if the map has a worldspawn
	if( entities.Num() )
	{

		// "removeEntities" "classname" can be set in the worldspawn to remove all entities with the given classname
		const idKeyValue* removeEntities = entities[0]->epairs.MatchPrefix( "removeEntities", NULL );
		while( removeEntities )
		{
			RemoveEntities( removeEntities->GetValue() );
			removeEntities = entities[0]->epairs.MatchPrefix( "removeEntities", removeEntities );
		}

		// "overrideMaterial" "material" can be set in the worldspawn to reset all materials
		idStr material;
		if( entities[0]->epairs.GetString( "overrideMaterial", "", material ) )
		{
			for( i = 0; i < entities.Num(); i++ )
			{
				mapEnt = entities[i];
				for( j = 0; j < mapEnt->GetNumPrimitives(); j++ )
				{
					idMapPrimitive* mapPrimitive = mapEnt->GetPrimitive( j );
					switch( mapPrimitive->GetType() )
					{
						case idMapPrimitive::TYPE_BRUSH:
						{
							idMapBrush* mapBrush = static_cast<idMapBrush*>( mapPrimitive );
							for( k = 0; k < mapBrush->GetNumSides(); k++ )
							{
								mapBrush->GetSide( k )->SetMaterial( material );
							}
							break;
						}
						case idMapPrimitive::TYPE_PATCH:
						{
							static_cast<idMapPatch*>( mapPrimitive )->SetMaterial( material );
							break;
						}
					}
				}
			}
		}

		// force all entities to have a name key/value pair
		if( entities[0]->epairs.GetBool( "forceEntityNames" ) )
		{
			for( i = 1; i < entities.Num(); i++ )
			{
				mapEnt = entities[i];
				if( !mapEnt->epairs.FindKey( "name" ) )
				{
					mapEnt->epairs.Set( "name", va( "%s%d", mapEnt->epairs.GetString( "classname", "forcedName" ), i ) );
				}
			}
		}

		// move the primitives of any func_group entities to the worldspawn
		if( entities[0]->epairs.GetBool( "moveFuncGroups" ) )
		{
			for( i = 1; i < entities.Num(); i++ )
			{
				mapEnt = entities[i];
				if( idStr::Icmp( mapEnt->epairs.GetString( "classname" ), "func_group" ) == 0 )
				{
					entities[0]->primitives.Append( mapEnt->primitives );
					mapEnt->primitives.Clear();
				}
			}
		}
	}

	// Record how many entities came from the base .map, so the .edit writer
	// can distinguish new ("add") entities from existing ("modify") ones.
	numBaseEntities = entities.Num();

	// Krispy: <name>.edit allows adding, modifying and removing entities
	fullName = name;
	fullName.SetFileExtension( "edit" );

	idFile* editFile = fileSystem->OpenFileRead( fullName );
	if( editFile != NULL )
	{
		fileSystem->CloseFile( editFile );
		ApplyEditFile( fullName, osPath );
	}

	hasPrimitiveData = true;
	return true;
}

/*
============
idMapFile::Write
============
*/
bool idMapFile::Write( const char* fileName, const char* ext, bool fromBasePath )
{
	int i;
	idStr qpath;
	idFile* fp;

	qpath = fileName;
	qpath.SetFileExtension( ext );

	idLib::common->Printf( "writing %s...\n", qpath.c_str() );

	if( fromBasePath )
	{
		fp = idLib::fileSystem->OpenFileWrite( qpath, "fs_basepath" );
	}
	else
	{
		fp = idLib::fileSystem->OpenExplicitFileWrite( qpath );
	}

	if( !fp )
	{
		idLib::common->Warning( "Couldn't open %s\n", qpath.c_str() );
		return false;
	}

	fp->WriteFloatString( "Version %f\n", ( float ) CURRENT_MAP_VERSION );

	for( i = 0; i < entities.Num(); i++ )
	{
		entities[i]->Write( fp, i );
	}

	idLib::fileSystem->CloseFile( fp );

	return true;
}

/*
===============
idMapFile::ApplyEditFile

Reads a <map>.edit file and applies "add", "modify" and "remove" operations
to this map's entity list.

Rules:
  - modify accepts "worldspawn" (=> base index 0) or "entity N".
  - add refuses "worldspawn" — worldspawn can't be added.
  - remove refuses both "worldspawn" and "entity 0" — worldspawn can't be
	removed.
===============
*/
bool idMapFile::ApplyEditFile( const char* filename, bool osPath )
{
	idLexer src( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
	idStr fullName = filename;
	if( !fullName.CheckExtension( "edit" ) )
	{
		fullName.SetFileExtension( "edit" );
	}

	if( !src.LoadFile( fullName, osPath ) )
	{
		// it's okay not to be found.
		return false;
	}

	idToken token;

	// map name
	if( !src.ReadToken( &token ) || token.type != TT_STRING )
	{
		src.Error( "%s: expected map name\n", __FUNCTION__ );
		return false;
	}

	if( !src.ExpectTokenString( "{" ) )
	{
		return false;
	}

	while( 1 )
	{
		if( !src.ReadToken( &token ) )
		{
			break;
		}

		if( token == "}" )
		{
			break;
		}

		if( token.type != TT_STRING )
		{
			src.Error( "%s: unexpected token '%s'\n", __FUNCTION__, token.c_str() );
			return false;
		}

		// version
		if( token == "version" )
		{
			idToken v;
			src.ReadToken( &v );
			continue;
		}

		// add entities
		if( token == "add" )
		{
			if( !src.ExpectTokenString( "{" ) )
			{
				return false;
			}

			while( 1 )
			{
				idToken entTok;
				if( !src.ReadToken( &entTok ) )
				{
					return false;
				}
				if( entTok == "}" )
				{
					break;
				}

				// worldspawn can't be added.
				if( idStr::Icmp( entTok.c_str(), "worldspawn" ) == 0 )
				{
					src.Warning( "%s: can't add worldspawn, skipping\n", __FUNCTION__ );

					if( src.PeekTokenString( "{" ) )
					{
						src.ExpectTokenString( "{" );
						src.SkipBracedSection( false );
					}
					continue;
				}

				if( !src.ExpectTokenString( "{" ) )
				{
					return false;
				}

				idMapEntity* mapEnt = new( TAG_SYSTEM ) idMapEntity();

				while( 1 )
				{
					idToken k, v;
					if( !src.ReadToken( &k ) )
					{
						delete mapEnt;
						return false;
					}

					if( k == "}" )
					{
						break;
					}

					if( k.type != TT_STRING )
					{
						src.Error( "%s: expected quoted key\n", __FUNCTION__ );
						delete mapEnt;
						return false;
					}

					if( !src.ReadToken( &v ) || v.type != TT_STRING )
					{
						src.Error( "%s: expected quoted value\n", __FUNCTION__ );
						delete mapEnt;
						return false;
					}
					mapEnt->epairs.Set( k.c_str(), v.c_str() );
				}

				// refuse to append anything that declares itself as worldspawn.
				if( idStr::Icmp( mapEnt->epairs.GetString( "classname" ), "worldspawn" ) == 0 )
				{
					src.Warning( "%s: entity with classname \"worldspawn\" can't be added, skipping\n", __FUNCTION__ );
					delete mapEnt;
					continue;
				}

				entities.Append( mapEnt );
			}
			continue;
		}

		// modify an existing entity
		if( token == "modify" )
		{
			if( !src.ExpectTokenString( "{" ) )
			{
				return false;
			}
			while( 1 )
			{
				idToken entTok;
				if( !src.ReadToken( &entTok ) )
				{
					return false;
				}
				if( entTok == "}" )
				{
					break;
				}

				int entIndex = -1;
				const bool isWorldspawn = ( idStr::Icmp( entTok.c_str(), "worldspawn" ) == 0 );

				if( isWorldspawn )
				{
					entIndex = 0;   // worldspawn is always entity 0
				}
				else if( sscanf( entTok.c_str(), "entity %d", &entIndex ) != 1 )
				{
					src.Warning( "%s: bad reference '%s', skipping block\n", __FUNCTION__, entTok.c_str() );

					if( src.PeekTokenString( "{" ) )
					{
						src.ExpectTokenString( "{" );
						src.SkipBracedSection( false );
					}
					continue;
				}

				if( !src.ExpectTokenString( "{" ) )
				{
					return false;
				}

				idMapEntity* mapEnt = ( entIndex >= 0 && entIndex < entities.Num() ) ? entities[entIndex] : NULL;
				if( mapEnt == NULL )
				{
					src.Warning( "%s: modify #%d out of range\n", __FUNCTION__, entIndex );
				}

				while( 1 )
				{
					idToken k, v;
					if( !src.ReadToken( &k ) )
					{
						return false;
					}

					if( k == "}" )
					{
						break;
					}

					if( k.type != TT_STRING )
					{
						return false;
					}

					if( !src.ReadToken( &v ) || v.type != TT_STRING )
					{
						return false;
					}

					// refuse to change the classname of worldspawn.
					if( mapEnt == entities[0] && idStr::Icmp( k.c_str(), "classname" ) == 0 )
					{
						src.Warning( "%s: can't change worldspawn's classname\n", __FUNCTION__ );
						continue;
					}

					if( mapEnt != NULL )
					{
						// empty value means "delete this key"
						if( v.Length() > 0 )
						{
							mapEnt->epairs.Set( k.c_str(), v.c_str() );
						}
						else
						{
							mapEnt->epairs.Delete( k.c_str() );
						}
					}
				}
			}
			continue;
		}

		// remove an entity
		if( token == "remove" )
		{
			if( !src.ExpectTokenString( "{" ) )
			{
				return false;
			}

			idList<int> toRemove;
			while( 1 )
			{
				idToken entTok;
				if( !src.ReadToken( &entTok ) )
				{
					return false;
				}
				if( entTok == "}" )
				{
					break;
				}

				// worldspawn can't be removed by name.
				if( idStr::Icmp( entTok.c_str(), "worldspawn" ) == 0 )
				{
					src.Warning( "%s: can't remove worldspawn, skipping\n", __FUNCTION__ );
					continue;
				}

				int entIndex = -1;
				if( sscanf( entTok.c_str(), "entity %d", &entIndex ) != 1 )
				{
					continue;
				}
				// entity 0 is worldspawn refuse it.
				if( entIndex == 0 )
				{
					src.Warning( "%s: entity 0 is worldspawn, can't be removed\n", __FUNCTION__ );
					continue;
				}
				if( entIndex > 0 )
				{
					toRemove.Append( entIndex );
				}
			}

			// delete from highest index to lowest so earlier removes don't
			// shift the indices of later removes
			for( int a = 0; a < toRemove.Num(); ++a )
			{
				for( int b = a + 1; b < toRemove.Num(); ++b )
				{
					if( toRemove[b] > toRemove[a] )
					{
						int tmp = toRemove[a];
						toRemove[a] = toRemove[b];
						toRemove[b] = tmp;
					}
				}
			}

			for( int a = 0; a < toRemove.Num(); ++a )
			{
				int idx = toRemove[a];
				if( idx >= 0 && idx < entities.Num() )
				{
					// don't delete worldspawn.
					if( idStr::Icmp( entities[idx]->epairs.GetString( "classname" ), "worldspawn" ) == 0 )
					{
						src.Warning( "%s: refusing to remove worldspawn (index %d)\n", __FUNCTION__, idx );
						continue;
					}
					delete entities[idx];
					entities.RemoveIndex( idx );
				}
			}
			continue;
		}

		// unknown section — skip it gracefully
		src.Warning( "%s: unknown section '%s'\n", __FUNCTION__, token.c_str() );
		if( src.PeekTokenString( "{" ) )
		{
			src.ExpectTokenString( "{" );
			src.SkipBracedSection( false );
		}
	}

	return true;
}

/*
===============
idMapFile::SetGeometryCRC
===============
*/
void idMapFile::SetGeometryCRC()
{
	int i;

	geometryCRC = 0;
	for( i = 0; i < entities.Num(); i++ )
	{
		geometryCRC ^= entities[i]->GetGeometryCRC();
	}
}

/*
===============
idMapFile::AddEntity
===============
*/
int idMapFile::AddEntity( idMapEntity* mapEnt )
{
	int ret = entities.Append( mapEnt );
	return ret;
}

/*
===============
idMapFile::FindEntity
===============
*/
idMapEntity* idMapFile::FindEntity( const char* name )
{
	for( int i = 0; i < entities.Num(); i++ )
	{
		idMapEntity* ent = entities[i];
		if( idStr::Icmp( ent->epairs.GetString( "name" ), name ) == 0 )
		{
			return ent;
		}
	}
	return NULL;
}

/*
===============
idMapFile::FindEntityAtOrigin
===============
*/
idMapEntity* idMapFile::FindEntityAtOrigin( const idVec3& org ) const
{
	idBounds bo( org );
	bo.ExpandSelf( 0.125f );

	for( int i = 0; i < entities.Num(); i++ )
	{
		idMapEntity* ent = entities[i];

		idVec3 entPos;
		ent->epairs.GetVector( "origin", "", entPos );

		if( bo.ContainsPoint( entPos ) )
		{
			return ent;
		}
	}
	return NULL;
}


/*
===============
idMapFile::RemoveEntity
===============
*/
void idMapFile::RemoveEntity( idMapEntity* mapEnt )
{
	entities.Remove( mapEnt );
	delete mapEnt;
}

/*
===============
idMapFile::RemoveEntity
===============
*/
void idMapFile::RemoveEntities( const char* classname )
{
	for( int i = 0; i < entities.Num(); i++ )
	{
		idMapEntity* ent = entities[i];
		if( idStr::Icmp( ent->epairs.GetString( "classname" ), classname ) == 0 )
		{
			delete entities[i];
			entities.RemoveIndex( i );
			i--;
		}
	}
}

/*
===============
idMapFile::RemoveAllEntities
===============
*/
void idMapFile::RemoveAllEntities()
{
	entities.DeleteContents( true );
	hasPrimitiveData = false;
}

/*
===============
idMapFile::RemovePrimitiveData
===============
*/
void idMapFile::RemovePrimitiveData()
{
	for( int i = 0; i < entities.Num(); i++ )
	{
		idMapEntity* ent = entities[i];
		ent->RemovePrimitiveData();
	}
	hasPrimitiveData = false;
}

/*
===============
idMapFile::NeedsReload
===============
*/
bool idMapFile::NeedsReload()
{
	if( name.Length() )
	{
		ID_TIME_T time = FILE_NOT_FOUND_TIMESTAMP;
		if( idLib::fileSystem->ReadFile( name, NULL, &time ) > 0 )
		{
			return ( time > fileTime );
		}
	}
	return true;
}

/*
===============
idMapFile::ConvertToPolygonMeshFormat
===============
*/
bool idMapFile::ConvertToPolygonMeshFormat()
{
	int count = GetNumEntities();
	for( int j = 0; j < count; j++ )
	{
		idMapEntity* ent = GetEntity( j );
		if( ent )
		{
			idStr classname = ent->epairs.GetString( "classname" );

			//if( classname == "worldspawn" )
			{
				for( int i = 0; i < ent->GetNumPrimitives(); i++ )
				{
					idMapPrimitive*	mapPrim;

					mapPrim = ent->GetPrimitive( i );
					if( mapPrim->GetType() == idMapPrimitive::TYPE_BRUSH )
					{
						idMapPolygonMesh* meshPrim = new idMapPolygonMesh();
						meshPrim->epairs.Copy( mapPrim->epairs );

						meshPrim->ConvertFromBrush( static_cast<idMapBrush*>( mapPrim ), j, i );
						ent->primitives[ i ] = meshPrim;

						delete mapPrim;

						continue;
					}
					else if( mapPrim->GetType() == idMapPrimitive::TYPE_PATCH )
					{
						idMapPolygonMesh* meshPrim = new idMapPolygonMesh();
						meshPrim->epairs.Copy( mapPrim->epairs );

						meshPrim->ConvertFromPatch( static_cast<idMapPatch*>( mapPrim ), j, i );
						ent->primitives[ i ] = meshPrim;

						delete mapPrim;

						continue;
					}
				}
			}
		}
	}

	return true;
}