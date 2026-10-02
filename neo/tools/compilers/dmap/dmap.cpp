/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans

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
#include "../compiler_resources.h"

dmapGlobals_t	dmapGlobals;

/*
============
ProcessModel
============
*/
bool ProcessModel( uEntity_t* e, bool floodFill )
{
	bspFace_t*	faces;

	// build a bsp tree using all of the sides
	// of all of the structural brushes
	faces = MakeStructuralBspFaceList( e->primitives );

	// RB: dump BSP for debugging
	if( dmapGlobals.glview )
	{
		WriteGLView( faces, "facelist" );
	}
	// RB end

	e->tree = FaceBSP( faces );

	// create portals at every leaf intersection
	// to allow flood filling
	MakeTreePortals( e->tree );

	// RB: calculate node numbers for split plane analysis
	NumberNodes_r( e->tree->headnode, 0 );

	// classify the leafs as opaque or areaportal
	FilterBrushesIntoTree( e );

	// RB: use mapTri_t by idMapPolygonMesh primitives in case we don't use brushes
	FilterMeshesIntoTree( e );

	// RB: dump BSP for debugging
	//if( dmapGlobals.glview )
	//{
	//WriteGLView( e->tree, "unclipped", dmapGlobals.entityNum );
	//}
	// RB end

	// see if the bsp is completely enclosed
	if( floodFill && !dmapGlobals.noFlood )
	{
		if( FloodEntities( e->tree ) )
		{
			// set the outside leafs to opaque
			FillOutside( e );
		}
		else
		{
			idLib::Printf( "**********************\n" );
			idLib::Warning( "******* leaked *******" );
			idLib::Printf( "**********************\n" );
			LeakFile( e->tree );
			WriteGLView( e->tree, "leaked", 0, true );
			// bail out here.  If someone really wants to
			// process a map that leaks, they should use
			// -noFlood
			return false;
		}
	}

	// get minimum convex hulls for each visible side
	// this must be done before creating area portals,
	// because the visible hull is used as the portal
	ClipSidesByTree( e );

	// determine areas before clipping tris into the
	// tree, so tris will never cross area boundaries
	FloodAreas( e );

	// RB: dump BSP for debugging
	if( dmapGlobals.glview )
	{
		WriteGLView( e->tree, "areas", dmapGlobals.entityNum );
	}
	// RB end

	// we now have a BSP tree with solid and non-solid leafs marked with areas
	// all primitives will now be clipped into this, throwing away
	// fragments in the solid areas
	PutPrimitivesInAreas( e );

	// now build shadow volumes for the lights and split
	// the optimize lists by the light beam trees
	// so there won't be unneeded overdraw in the static
	// case
	Prelight( e );

	// optimizing is a superset of fixing tjunctions
	if( !dmapGlobals.noOptimize )
	{
		OptimizeEntity( e );
	}
	else  if( !dmapGlobals.noTJunc )
	{
		FixEntityTjunctions( e );
	}

	// now fix t junctions across areas
	FixGlobalTjunctions( e );

	return true;
}

/*
============
ProcessModels
============
*/
bool ProcessModels()
{
	bool oldVerbose = dmapGlobals.verbose;

	idStrStatic<128> entityInfo;

	for( dmapGlobals.entityNum = 0; dmapGlobals.entityNum < dmapGlobals.numEntities; dmapGlobals.entityNum++ )
	{
		uEntity_t* entity = &dmapGlobals.uEntities[dmapGlobals.entityNum];
		if( !entity->primitives )
		{
			continue;
		}

		//idLib::Printf( "############### entity %i ###############\n", dmapGlobals.entityNum );

		// if we leaked, stop without any more processing
		if( !ProcessModel( entity, ( bool )( dmapGlobals.entityNum == 0 ) ) )
		{
			return false;
		}

		// we usually don't want to see output for submodels unless
		// something strange is going on
		if( !dmapGlobals.verboseentities )
		{
			dmapGlobals.verbose = false;
		}
	}

	dmapGlobals.verbose = oldVerbose;

	return true;
}

/*
============
DmapHelp
============
*/
void DmapHelp()
{
	idLib::Printf(

		"Usage: dmap [options] mapfile\n"
		"Options:\n"
		"noCurves          = don't process curves\n"
		"noCM              = don't create collision map\n"
		"noAAS             = don't create AAS files\n"
		"exportOCL         = export occlusion geometry (.ocl) as obj files\n"

	);
}

/*
============
ResetDmapGlobals
============
*/
void ResetDmapGlobals()
{
	dmapGlobals.mapFileBase[0] = '\0';
	dmapGlobals.dmapFile = NULL;
	dmapGlobals.prepared = false;
	dmapGlobals.noCM = false;
	dmapGlobals.noAAS = false;
	dmapGlobals.region = false;
	dmapGlobals.startTime = 0;
	dmapGlobals.mapPlanes.Clear();
	dmapGlobals.splitPlanesCounter.Clear();
	dmapGlobals.materialNames.Clear();
	dmapGlobals.materials.Clear();
	dmapGlobals.renderModelNames.Clear();
	dmapGlobals.renderModels.Clear();
	dmapGlobals.numEntities = 0;
	dmapGlobals.uEntities = NULL;
	dmapGlobals.entityNum = 0;
	dmapGlobals.mapLights.Clear();
	dmapGlobals.interAreaPortals.Clear();
	dmapGlobals.verbose = false;
	dmapGlobals.glview = false;
	dmapGlobals.noOptimize = false;
	dmapGlobals.verboseentities = false;
	dmapGlobals.noCurves = false;
	dmapGlobals.fullCarve = false;
	dmapGlobals.noModelBrushes = false;
	dmapGlobals.noTJunc = false;
	dmapGlobals.noMerge = false;
	dmapGlobals.noFlood = false;
	dmapGlobals.noClipSides = false;
	dmapGlobals.noLightCarve = false;
	dmapGlobals.exportOCL = false;
	dmapGlobals.drawBounds.Clear();
	dmapGlobals.drawflag = false;
	dmapGlobals.portal_sky_location.Zero();
	dmapGlobals.bspAlternateSplitWeights = false;
	dmapGlobals.blockSize = idVec3( 1024.0f, 1024.0f, 1024.0f );	// default block size for splitting
	dmapGlobals.inlineStatics = false;
	dmapGlobals.totalInlinedModels = 0;
}

/*
============
Dmap
============
*/
bool DmapPreInit( const idCmdArgs& args, bool threaded )
{
	int			i;
	char		path[1024];
	idStr		passedName;

	ResetDmapGlobals();

	if( args.Argc() < 2 )
	{
		DmapHelp();
		return false;
	}

	idLib::Printf( "---- dmap ----\n" );

	dmapGlobals.fullCarve = true;
	dmapGlobals.noLightCarve = true;

	for( i = 1 ; i < args.Argc() ; i++ )
	{
		const char* s;

		s = args.Argv( i );
		if( s[0] == '-' )
		{
			s++;
			if( s[0] == '\0' )
			{
				continue;
			}
		}

		if( !idStr::Icmp( s, "glview" ) )
		{
			dmapGlobals.glview = true;
		}
		else if( !idStr::Icmp( s, "v" ) )
		{
			idLib::Printf( "verbose = true\n" );
			dmapGlobals.verbose = true;
		}
		else if( !idStr::Icmp( s, "draw" ) )
		{
			idLib::Printf( "draw = true\n" );
			dmapGlobals.drawflag = true;
		}
		else if( !idStr::Icmp( s, "altsplit" ) )
		{
			idLib::Printf( "bspAlternateSplitWeights = true\n" );
			dmapGlobals.bspAlternateSplitWeights = true;
		}
		else if( !idStr::Icmp( s, "blockSize" ) )
		{
			if( i + 3 >= args.Argc() )
			{
				idLib::Error( "usage: dmap blockSize <x> <y> <z>" );
			}
			dmapGlobals.blockSize[0] = atof( args.Argv( i + 1 ) );
			dmapGlobals.blockSize[1] = atof( args.Argv( i + 2 ) );
			dmapGlobals.blockSize[2] = atof( args.Argv( i + 3 ) );
			idLib::Printf( "blockSize = %f %f %f\n", dmapGlobals.blockSize[0], dmapGlobals.blockSize[1], dmapGlobals.blockSize[2] );
			i += 3;
		}
		else if( !idStr::Icmp( s, "inlineAll" ) )
		{
			idLib::Printf( "inlineAll = true\n" );
			dmapGlobals.inlineStatics = true;
		}
		else if( !idStr::Icmp( s, "noMerge" ) )
		{
			idLib::Printf( "noMerge = true\n" );
			dmapGlobals.noMerge = true;
		}
		else if( !idStr::Icmp( s, "noFlood" ) )
		{
			idLib::Printf( "noFlood = true\n" );
			dmapGlobals.noFlood = true;
		}
		else if( !idStr::Icmp( s, "noLightCarve" ) )
		{
			idLib::Printf( "noLightCarve = true\n" );
			dmapGlobals.noLightCarve = true;
		}
		else if( !idStr::Icmp( s, "lightCarve" ) )
		{
			idLib::Printf( "noLightCarve = false\n" );
			dmapGlobals.noLightCarve = false;
		}
		else if( !idStr::Icmp( s, "noOpt" ) )
		{
			idLib::Printf( "noOptimize = true\n" );
			dmapGlobals.noOptimize = true;
		}
		else if( !idStr::Icmp( s, "verboseentities" ) )
		{
			idLib::Printf( "verboseentities = true\n" );
			dmapGlobals.verboseentities = true;
		}
		else if( !idStr::Icmp( s, "noCurves" ) )
		{
			idLib::Printf( "noCurves = true\n" );
			dmapGlobals.noCurves = true;
		}
		else if( !idStr::Icmp( s, "noModels" ) )
		{
			idLib::Printf( "noModels = true\n" );
			dmapGlobals.noModelBrushes = true;
		}
		else if( !idStr::Icmp( s, "noClipSides" ) )
		{
			idLib::Printf( "noClipSides = true\n" );
			dmapGlobals.noClipSides = true;
		}
		else if( !idStr::Icmp( s, "noCarve" ) )
		{
			idLib::Printf( "noCarve = true\n" );
			dmapGlobals.fullCarve = false;
		}
		else if( !idStr::Icmp( s, "noTjunc" ) )
		{
			// triangle optimization won't work properly without tjunction fixing
			idLib::Printf( "noTJunc = true\n" );
			dmapGlobals.noTJunc = true;
			dmapGlobals.noOptimize = true;
			idLib::Printf( "forcing noOptimize = true\n" );
		}
		else if( !idStr::Icmp( s, "noCM" ) )
		{
			dmapGlobals.noCM = true;
			idLib::Printf( "noCM = true\n" );
		}
		else if( !idStr::Icmp( s, "exportOCL" ) )
		{
			dmapGlobals.exportOCL = true;
			idLib::Printf( "exportOCL = true\n" );
		}
		else if( !idStr::Icmp( s, "noAAS" ) )
		{
			dmapGlobals.noAAS = true;
			idLib::Printf( "noAAS = true\n" );
		}
		else if( !idStr::Icmp( s, "editorOutput" ) )
		{
#ifdef _WIN32
			com_outputMsg = true;
#endif
		}
		else
		{
			break;
		}
	}

	if( i >= args.Argc() )
	{
		idLib::Error( "usage: dmap [options] mapfile" );
	}

	passedName = args.Argv( i );		// may have an extension
	passedName.BackSlashesToSlashes();
	if( passedName.Icmpn( "maps/", 4 ) != 0 )
	{
		passedName = "maps/" + passedName;
	}

	idStr stripped = passedName;
	stripped.StripFileExtension();
	idStr::Copynz( dmapGlobals.mapFileBase, stripped, sizeof( dmapGlobals.mapFileBase ) );

	// if this isn't a regioned map, delete the last saved region map
	if( passedName.Right( 4 ) != ".reg" )
	{
		idStr::snPrintf( path, sizeof( path ), "%s.reg", dmapGlobals.mapFileBase );
		fileSystem->RemoveFile( path );
	}
	else
	{
		dmapGlobals.region = true;
	}


	passedName = stripped;

	// delete any old line leak files
	idStr::snPrintf( path, sizeof( path ), "%s.lin", dmapGlobals.mapFileBase );
	fileSystem->RemoveFile( path );

	// delete any old generated binary proc files
	idStr generated = va( "generated/%s.bproc", dmapGlobals.mapFileBase );
	fileSystem->RemoveFile( generated.c_str() );

	// delete any old generated binary cm files
	generated = va( "generated/%s.bcm", dmapGlobals.mapFileBase );
	fileSystem->RemoveFile( generated.c_str() );

	// delete any old ASCII collision files
	idStr::snPrintf( path, sizeof( path ), "%s.cm", dmapGlobals.mapFileBase );
	fileSystem->RemoveFile( path );

	//
	// start from scratch
	//
	dmapGlobals.startTime = Sys_Milliseconds();

	if( !LoadDMapFile( passedName ) )
	{
		return false;
	}

	if( threaded )
	{
		DmapDisconnect();

		if( !dmapGlobals.noAAS && !dmapGlobals.region )
		{
			AASPreloadDefinitions();
			AASPreloadMap( dmapGlobals.dmapFile );
		}
	}

	DmapFindMaterial( "_default" );
	for( int entityNum = 0; entityNum < dmapGlobals.numEntities; entityNum++ )
	{
		idMapEntity* mapEntity = dmapGlobals.uEntities[entityNum].mapEntity;
		const char* className = mapEntity->epairs.GetString( "classname" );
		if( idStr::Icmp( className, "light" ) == 0 )
		{
			const char* texture = mapEntity->epairs.GetString( "texture", "" );
			if( texture[0] )
			{
				DmapFindMaterial( texture );
			}
		}

		const char* modelName = mapEntity->epairs.GetString( "model", "" );
		if( modelName[0] )
		{
			idStr modelPath = modelName;
			idStr extension;
			modelPath.ExtractFileExtension( extension );
			if( extension.Icmp( "lwo" ) == 0 || extension.Icmp( "ase" ) == 0 ||
					extension.Icmp( "ma" ) == 0 || extension.Icmp( "md3" ) == 0 ||
					extension.Icmp( "obj" ) == 0 )
			{
				DmapFindRenderModel( modelName );
			}
		}
	}

	dmapGlobals.prepared = true;
	return true;
}

/*
============
DmapInit
============
*/
void DmapInit( const idCmdArgs& args )
{
	if( !dmapGlobals.prepared )
	{
		idLib::Error( "dmap requested without being prepared" );
		return;
	}

	int start = dmapGlobals.startTime;
	int end;
	bool leaked = false;

	if( ProcessModels() )
	{
		WriteOutputFile();
		WriteOclFile();
	}
	else
	{
		leaked = true;
	}

	FreeDMapFile();

	idLib::Printf( "%i static models merged\n", dmapGlobals.totalInlinedModels );

	end = Sys_Milliseconds();
	idLib::Printf( "-----------------------\n" );
	idLib::Printf( "%5.0f seconds for dmap\n", ( end - start ) * 0.001f );

	if( !leaked )
	{
		if( !dmapGlobals.noCM )
		{
			// make sure the collision model manager is not used by the game
			if( idLib::IsMainThread() )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
			}

			// create the collision map
			start = Sys_Milliseconds();

			// write always a fresh .cm file
			collisionModelManager->LoadMap( dmapGlobals.dmapFile, true );
			collisionModelManager->FreeMap();

			end = Sys_Milliseconds();
			idLib::Printf( "-------------------------------------\n" );
			idLib::Printf( "%5.0f seconds to create collision map\n", ( end - start ) * 0.001f );
		}

		if( !dmapGlobals.noAAS && !dmapGlobals.region )
		{
			// create AAS files
			RunAAS_f( args );
		}
	}

	// free the common .map representation
	delete dmapGlobals.dmapFile;
	dmapGlobals.dmapFile = NULL;

	// clear the map plane list
	dmapGlobals.mapPlanes.Clear();

#ifdef _WIN32
	if( com_outputMsg && com_hwndMsg != NULL )
	{
		unsigned int msg = ::RegisterWindowMessage( DMAP_DONE );
		::PostMessage( com_hwndMsg, msg, 0, 0 );
	}
#endif
	dmapGlobals.prepared = false;
}

/*
============
DmapInit_f
============
*/
void DmapInit_f( const idCmdArgs& args )
{
	DmapInit( args );
}

/*
============
DmapDisconnect
============
*/
void DmapDisconnect()
{
	if( !dmapGlobals.noCM && idLib::IsMainThread() )
	{
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
	}
}

/*
============
DmapAborted
============
*/
void DmapAborted()
{
	if( !dmapGlobals.prepared )
	{
		return;
	}

	FreeDMapFile();
	delete dmapGlobals.dmapFile;
	dmapGlobals.dmapFile = NULL;
	dmapGlobals.mapPlanes.Clear();
	dmapGlobals.prepared = false;
}

/*
============
Dmap
============
*/
void Dmap( const idCmdArgs& args )
{
	if( DmapPreInit( args, false ) )
	{
		DmapInit( args );
	}
}

/*
============
Dmap_f
============
*/
void Dmap_f( const idCmdArgs& args )
{

	common->ClearWarnings( "running dmap" );

	// refresh the screen each time we print so it doesn't look
	// like it is hung
	if( idLib::IsMainThread() )
	{
		common->SetRefreshOnPrint( true );
	}
	Dmap( args );
	if( idLib::IsMainThread() )
	{
		common->SetRefreshOnPrint( false );
	}

	common->PrintWarnings();
}

/*
============
DmapDir_f
============
*/
void DmapDir_f( const idCmdArgs& args )
{
	if( args.Argc() != 2 )
	{
		idLib::Printf( "Usage: dmapDir <folder>\n" );
		return;
	}

	if( !idLib::IsMainThread() )
	{
		idLib::Error( "dmapDir must run on the main thread" );
		return;
	}

	idStr directory = DMapDirectoryPath( args.Argv( 1 ) );
	idFileList* mapFiles = fileSystem->ListFiles( directory, ".map" );
	if( !mapFiles || mapFiles->GetNumFiles() == 0 )
	{
		idLib::Printf( "No map files found in %s\n", directory.c_str() );
	}
	else
	{
		for( int i = 0; i < mapFiles->GetNumFiles(); i++ )
		{
			idStr mapName = directory + "/" + mapFiles->GetFile( i );
			idCmdArgs mapArgs;
			mapArgs.AppendArg( "dmap" );
			mapArgs.AppendArg( mapName.c_str() );
			Dmap_f( mapArgs );
		}
	}
	if( mapFiles )
	{
		fileSystem->FreeFileList( mapFiles );
	}
}