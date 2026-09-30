/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans
Copyright (C) 2016 Johannes Ohlemacher (http://github.com/eXistence/fhDOOM)

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "dmap.h"
#include "../../framework/OBJExporter.h"

/*
=============================================================================

Occlusion geometry for shadow map rendering.

For every prelit light that casts shadows, dmap collects the triangles
inside the light frustum that cast shadows, optimizes them and writes them
into a separate <map>.ocl file.  The .map and .proc files are unchanged.
The .ocl file is optional, maps without one render as before.

=============================================================================
*/

// max number of material groups (occluder surfaces) per light
#define MAX_LIGHT_OCCLUDERS 128

void Write1DMatrix( idFile* f, int x, float* m );		// output.cpp

/*
====================
BuildLightOccluders
====================
*/
void BuildLightOccluders( uEntity_t* e, mapLight_t* light )
{
	assert( light && "light must not be NULL" );
	assert( e && "e must not be NULL" );

	memset( light->occluders, 0, sizeof( light->occluders ) );

	if( !light->def.LightCastsShadows() )
	{
		return;
	}

	// the name is the only thing that links the occluders to the light entity at runtime
	if( light->name[0] == '\0' )
	{
		idLib::Warning( "BuildLightOccluders: light at (%f,%f,%f) has no name, skipping occluders",
						light->def.parms.origin.x, light->def.parms.origin.y, light->def.parms.origin.z );
		return;
	}

	// shadowers will contain all the triangles that will cast a shadow
	optimizeGroup_t* shadowerGroups[MAX_LIGHT_OCCLUDERS] = { NULL };

	for( int i = 0; i < e->numAreas; i++ )
	{
		for( optimizeGroup_t* group = e->areas[i].groups; group; group = group->nextGroup )
		{
			// if the surface doesn't cast shadows, skip it
			if( !group->material->SurfaceCastsShadow() )
			{
				continue;
			}

			// if the group bounds doesn't intersect the light bounds, skip it
			if( !group->bounds.IntersectsBounds( light->def.globalLightBounds ) )
			{
				continue;
			}

			// build up a list of the triangle fragments inside the light frustum
			mapTri_t* shadowers = NULL;
			for( mapTri_t* tri = group->triList; tri; tri = tri->next )
			{
				mapTri_t* in, *out;

				// clip it to the light frustum
				ClipTriByLight( light, tri, &in, &out );
				FreeTriList( out );
				shadowers = MergeTriLists( shadowers, in );
			}

			// if we didn't get any out of this group, we don't
			// need to create a new group in the shadower list
			if( !shadowers )
			{
				continue;
			}

			// all opaque groups share one slot, everything else is grouped by material
			int trisIndex = 0;
			for( ; trisIndex < MAX_LIGHT_OCCLUDERS; trisIndex++ )
			{
				if( !shadowerGroups[trisIndex] )
				{
					break;
				}

				if( shadowerGroups[trisIndex]->material->Coverage() == MC_OPAQUE && group->material->Coverage() == MC_OPAQUE )
				{
					break;
				}

				if( shadowerGroups[trisIndex]->material == group->material )
				{
					break;
				}
			}

			if( trisIndex >= MAX_LIGHT_OCCLUDERS )
			{
				idLib::Warning( "BuildLightOccluders: light '%s' exceeds %i occluder materials, dropping surfaces", light->name, MAX_LIGHT_OCCLUDERS );
				FreeTriList( shadowers );
				continue;
			}

			// find a group in shadowerGroups to add these to
			// we will ignore everything but planenum, and we
			// can merge across areas
			optimizeGroup_t* check;

			for( check = shadowerGroups[trisIndex]; check; check = check->nextGroup )
			{
				if( check->planeNum == group->planeNum )
				{
					break;
				}
			}

			if( !check )
			{
				check = ( optimizeGroup_t* )Mem_Alloc( sizeof( *check ), TAG_TOOLS );
				*check = *group;
				if( check->material->Coverage() != MC_PERFORATED )
				{
					check->material = declManager->FindMaterial( "_default" );
				}

				check->triList = NULL;
				check->nextGroup = shadowerGroups[trisIndex];
				shadowerGroups[trisIndex] = check;
			}

			check->triList = MergeTriLists( check->triList, shadowers );
		}
	}

	// optimize all the groups
	for( int i = 0; i < MAX_LIGHT_OCCLUDERS; i++ )
	{
		if( !shadowerGroups[i] )
		{
			continue;
		}

		OptimizeGroupList( shadowerGroups[i] );

		// combine all the triangles into one list
		mapTri_t* combined = NULL;

		for( optimizeGroup_t* group = shadowerGroups[i]; group; group = group->nextGroup )
		{
			combined = MergeTriLists( combined, CopyTriList( group->triList ) );
		}

		if( combined )
		{
			// find uniqued vertexes
			light->occluders[i].tris = ShareMapTriVerts( combined );
			light->occluders[i].material = shadowerGroups[i]->material;

			FreeTriList( combined );
		}

		FreeOptimizeGroupList( shadowerGroups[i] );
	}
}

/*
====================
WriteOccluderTriangles

Writes text verts and indexes to file
====================
*/
static void WriteOccluderTriangles( idFile* f, const srfTriangles_t* tris, bool excludeTextureCoordinates )
{
	f->WriteFloatString( "/* numVerts = */ %i /* numIndexes = */ %i /* no texcoords = */ %i \n",
						 tris->numVerts, tris->numIndexes, ( excludeTextureCoordinates ? 1 : 0 ) );

	// verts
	int col = 0;
	for( int i = 0; i < tris->numVerts; i++ )
	{
		const idDrawVert* dv = &tris->verts[i];
		const idVec2 st = dv->GetTexCoord();

		float vec[5];
		vec[0] = dv->xyz[0];
		vec[1] = dv->xyz[1];
		vec[2] = dv->xyz[2];
		vec[3] = st[0];
		vec[4] = st[1];

		Write1DMatrix( f, excludeTextureCoordinates ? 3 : 5, vec );

		if( ++col == 3 )
		{
			col = 0;
			f->WriteFloatString( "\n" );
		}
	}
	if( col != 0 )
	{
		f->WriteFloatString( "\n" );
	}

	// indexes
	col = 0;
	for( int i = 0; i < tris->numIndexes; i++ )
	{
		f->WriteFloatString( "%i ", tris->indexes[i] );

		if( ++col == 18 )
		{
			col = 0;
			f->WriteFloatString( "\n" );
		}
	}
	if( col != 0 )
	{
		f->WriteFloatString( "\n" );
	}
}

/*
====================
ExportLightOccludersOBJ
====================
*/
static void ExportLightOccludersOBJ( const mapLight_t* light )
{
	OBJExporter exporter;

	OBJExporter::OBJGroup& group = exporter.groups.Alloc();
	group.name = light->name;

	OBJExporter::OBJObject& object = group.objects.Alloc();
	object.name = light->name;

	for( int i = 0; i < MAX_LIGHT_OCCLUDERS; i++ )
	{
		const srfTriangles_t* tris = light->occluders[i].tris;
		const idMaterial* material = light->occluders[i].material;

		if( !tris || tris->numVerts <= 0 || !material )
		{
			continue;
		}

		exporter.materials.AddUnique( material );

		// OBJExporter writes one polygon per face, so emit one face per triangle
		for( int j = 0; j + 2 < tris->numIndexes; j += 3 )
		{
			OBJExporter::OBJFace& face = object.faces.Alloc();
			face.material = material;

			for( int k = 0; k < 3; k++ )
			{
				face.verts.Append( tris->verts[tris->indexes[j + k]] );
				face.indexes.Append( k );
			}
		}
	}

	// SetFileExtension() in OBJExporter::Write would cut the name at a '.'
	idStr safeName = light->name;
	safeName.Replace( ".", "_" );

	idStr objPath;
	objPath.Format( "%s/%s", dmapGlobals.mapFileBase, safeName.c_str() );

	exporter.Write( objPath, "fs_basepath" );
}

/*
====================
WriteOclFile
====================
*/
void WriteOclFile()
{
	idLib::Printf( "----- WriteOclFile -----\n" );

	idStr qpath;
	qpath.Format( "%s." OCL_FILE_EXT, dmapGlobals.mapFileBase );

	idLib::Printf( "writing %s\n", qpath.c_str() );

	idFileLocal oclFile( fileSystem->OpenFileWrite( qpath, "fs_basepath" ) );
	if( !oclFile )
	{
		idLib::Error( "Error opening %s", qpath.c_str() );
	}

	oclFile->WriteFloatString( "%s\n\n", OCL_FILE_ID );

	int lights = 0;
	int surfaces = 0;
	int vertices = 0;

	for( int i = 0; i < dmapGlobals.mapLights.Num(); i++ )
	{
		mapLight_t* light = dmapGlobals.mapLights[i];

		// gather the valid surfaces
		int numSurfaces = 0;
		for( int j = 0; j < MAX_LIGHT_OCCLUDERS; j++ )
		{
			if( light->occluders[j].tris && light->occluders[j].tris->numVerts > 0 && light->occluders[j].material )
			{
				numSurfaces++;
			}
		}

		if( numSurfaces > 0 )
		{
			lights++;
			oclFile->WriteFloatString( "occluder { /* name = */ \"_occluder_%s\" /* numSurfaces = */ %i \n\n", light->name, numSurfaces );

			int surfaceNum = 0;
			for( int j = 0; j < MAX_LIGHT_OCCLUDERS; j++ )
			{
				const srfTriangles_t* tris = light->occluders[j].tris;
				const idMaterial* material = light->occluders[j].material;

				if( !tris || tris->numVerts <= 0 || !material )
				{
					continue;
				}

				surfaces++;
				vertices += tris->numVerts;

				oclFile->WriteFloatString( "/* surface %d */ { \"%s\" ", surfaceNum++, material->GetName() );
				WriteOccluderTriangles( oclFile, tris, material->Coverage() != MC_PERFORATED );
				oclFile->WriteFloatString( "}\n\n" );
			}

			oclFile->WriteFloatString( "}\n\n" );

			if( dmapGlobals.exportOCL )
			{
				ExportLightOccludersOBJ( light );
			}
		}

		for( int j = 0; j < MAX_LIGHT_OCCLUDERS; j++ )
		{
			if( light->occluders[j].tris )
			{
				R_FreeStaticTriSurf( light->occluders[j].tris );
				light->occluders[j].tris = NULL;
			}
		}
	}

	idLib::Printf( "occluder stats: %d lights, %d surfaces, %d vertices\n", lights, surfaces, vertices );
}
