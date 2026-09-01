/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2026 Justin Marshall(justinmarshall20@gmail.com)

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

#include "AAS2_Map_Adapter.h"
#include "AAS2_Build.h"
#include "AAS2_Binary.h"
#include "AAS2_Brush_Surface.h"
#include "AAS2_Cspace.h"
#include "AAS2_Compiler.h"

struct AAS2ProgressState
{
	idStr stage;
	int lastPercent = -1;
	int startTime = 0;
};

/*
============
AAS2BuildProgress
============
*/
static void AAS2BuildProgress( const char* stage, size_t current, size_t total, void* userData )
{
	AAS2ProgressState* state = static_cast<AAS2ProgressState*>( userData );
	const int percent = total != 0 ?
						static_cast<int>( ( current * 100 ) / total ) : 100;
	if( state->stage.Icmp( stage ) != 0 )
	{
		state->stage = stage;
		state->lastPercent = -1;
	}

	if( percent != 100 && state->lastPercent >= 0 && percent < state->lastPercent + 5 )
	{
		return;
	}

	state->lastPercent = percent;
	common->Printf( "AAS2 %-28s %3d%%  (%u/%u, %.1fs)\n",
					stage, percent,
					static_cast<unsigned int>( current ),
					static_cast<unsigned int>( total ),
					( Sys_Milliseconds() - state->startTime ) * 0.001f );
}

/*
============
ReadAAS2Profile
============
*/
static bool ReadAAS2Profile( const char* requestedName, BuildSettings& build, BinaryMetadata& binary, idStr& profileName )
{
	profileName = requestedName && *requestedName ? requestedName : "aas48";
	if( profileName.Icmpn( "aas", 3 ) != 0 )
	{
		profileName.Insert( "aas", 0 );
	}

	const idDeclEntityDef* decl = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, profileName, false ) );
	if( decl == NULL )
	{
		common->Warning( "AAS2 settings decl '%s' was not found", profileName.c_str() );
		return false;
	}

	const idDict& dict = decl->dict;
	idVec3 mins, maxs, gravity;
	if( !dict.GetVector( "mins", NULL, mins ) || !dict.GetVector( "maxs", NULL, maxs ) )
	{
		common->Warning( "AAS2 settings decl '%s' has no mins/maxs", profileName.c_str() );
		return false;
	}

	build.agent.mins = {mins.x, mins.y, mins.z};
	build.agent.maxs = {maxs.x, maxs.y, maxs.z};
	build.navigation.agentHeight = maxs.z - mins.z;
	build.navigation.agentRadius = Max( Max( idMath::Fabs( mins.x ), idMath::Fabs( maxs.x ) ),
										Max( idMath::Fabs( mins.y ), idMath::Fabs( maxs.y ) ) );
	dict.GetFloat( "maxStepHeight", "18", build.navigation.maxStepHeight );
	dict.GetFloat( "maxFallHeight", "64", build.navigation.maxFallHeight );
	float minFloorCos = 0.7f;
	dict.GetFloat( "minFloorCos", "0.7", minFloorCos );
	minFloorCos = idMath::ClampFloat( -1.0f, 1.0f, minFloorCos );
	build.navigation.maxFloorSlopeDegrees = idMath::ACos( minFloorCos ) * idMath::M_RAD2DEG;

	binary.agent = build.agent;
	binary.fileExtensionAAS = dict.GetString( "fileExtension", profileName.c_str() );
	binary.groupName = dict.GetString( "groupName", "aas" );
	binary.explicitGroupName = dict.GetString( "explicitGroupName", profileName.c_str() );
	const char* type = dict.GetString( "type", "monster" );
	binary.type = idStr::Icmp( type, "player" ) == 0 ? 0 :
				  idStr::Icmp( type, "vehicle" ) == 0 ? 2 :
				  idStr::Icmp( type, "compass" ) == 0 ? 3 :
				  idStr::Icmp( type, "bot" ) == 0 ? 4 : 1;
	dict.GetInt( "primitiveModeBrush", "0", binary.primitiveModeBrush );
	dict.GetInt( "primitiveModePatch", dict.GetBool( "usePatches", "0" ) ? "0" : "1",
				 binary.primitiveModePatch );
	dict.GetInt( "primitiveModeModel", "1", binary.primitiveModeModel );
	dict.GetVector( "gravity", "0 0 -1066", gravity );
	binary.gravityValue = gravity.Length();
	if( binary.gravityValue > 0.0f )
	{
		gravity /= binary.gravityValue;
	}
	binary.gravityDir = {gravity.x, gravity.y, gravity.z};
	binary.maxStepHeight = build.navigation.maxStepHeight;
	dict.GetFloat( "maxBarrierHeight", "32", binary.maxBarrierHeight );
	dict.GetFloat( "maxWaterJumpHeight", "20", binary.maxWaterJumpHeight );
	binary.maxFallHeight = static_cast<int>( build.navigation.maxFallHeight );
	binary.minFloorCos = minFloorCos;
#define AAS2_FLOAT_SETTING(key, member, fallback) dict.GetFloat(key, fallback, binary.member)
	AAS2_FLOAT_SETTING( "minHighCeiling", minHighCeiling, "80" );
	AAS2_FLOAT_SETTING( "groundSpeed", groundSpeed, "250" );
	AAS2_FLOAT_SETTING( "waterSpeed", waterSpeed, "150" );
	AAS2_FLOAT_SETTING( "ladderSpeed", ladderSpeed, "50" );
	AAS2_FLOAT_SETTING( "wallCornerEdgeRadius", wallCornerEdgeRadius, "16" );
	AAS2_FLOAT_SETTING( "ledgeCornerEdgeRadius", ledgeCornerEdgeRadius, "16" );
	AAS2_FLOAT_SETTING( "obstaclePVSRadius", obstaclePVSRadius, "1024" );
	AAS2_FLOAT_SETTING( "minCrouchingCoverHeight", minCrouchingCoverHeight, "32" );
	AAS2_FLOAT_SETTING( "minStandingCoverHeight", minStandingCoverHeight, "64" );
	AAS2_FLOAT_SETTING( "crouchingFireHeight", crouchingFireHeight, "48" );
	AAS2_FLOAT_SETTING( "standingFireHeight", standingFireHeight, "72" );
	AAS2_FLOAT_SETTING( "minWallWidth", minWallWidth, "8" );
	AAS2_FLOAT_SETTING( "maxWallWidth", maxWallWidth, "32" );
	AAS2_FLOAT_SETTING( "minDoorWidth", minDoorWidth, "32" );
	AAS2_FLOAT_SETTING( "maxDoorWidth", maxDoorWidth, "80" );
	AAS2_FLOAT_SETTING( "coverCornerDistance", coverCornerDistance, "8" );
	AAS2_FLOAT_SETTING( "coverWallDistance", coverWallDistance, "8" );
	AAS2_FLOAT_SETTING( "chokePointWidth", chokePointWidth, "96" );
#undef AAS2_FLOAT_SETTING
	dict.GetInt( "tt_barrierJump", "100", binary.ttBarrierJump );
	dict.GetInt( "tt_waterJump", "100", binary.ttWaterJump );
	dict.GetInt( "tt_startWalkOffLedge", "100", binary.ttStartWalkOffLedge );
	dict.GetInt( "tt_startLadderClimb", "100", binary.ttStartLadderClimb );
	binary.usePatches = dict.GetBool( "usePatches", "0" );
	return true;
}

/*
============
AAS2OutputName
============
*/
static idStr AAS2OutputName( const char* mapName, const char* profile )
{
	idStr output = mapName;
	output.StripFileExtension();
	idStr suffix = profile;

	if( suffix.Icmpn( "aas_", 4 ) == 0 )
	{
		suffix = suffix.Mid( 4, suffix.Length() - 4 );
	}
	else if( suffix.Icmpn( "aas", 3 ) == 0 )
	{
		suffix = suffix.Mid( 3, suffix.Length() - 3 );
	}

	output += "_";
	output += suffix;
	output += ".aas2";
	return output;
}

/*
============
PrintAAS2BuildFailure
============
*/
static void PrintAAS2BuildFailure( const BuildResult& result )
{
	switch( result.error )
	{
		case BuildError::configurationSpace:
			common->Warning( "AAS2 build failed in configuration space: %s "
							 "at brush %u plane %u",
							 CSpaceErrorName( result.configurationSpace.error ),
							 static_cast<unsigned int>( result.configurationSpace.sourceBrush ),
							 static_cast<unsigned int>( result.configurationSpace.sourcePlane ) );
			return;
		case BuildError::brushSurface:
			if( result.brushSurface.error ==
					BrushSurfaceError::fragmentLimit )
			{
				common->Warning( "AAS2 floor CSG safety limit at face brush %u "
								 "while clipping brush %u (peak %u fragments, "
								 "%u points)",
								 static_cast<unsigned int>( result.brushSurface.sourceBrush ),
								 static_cast<unsigned int>( result.brushSurface.sourcePlane ),
								 static_cast<unsigned int>( result.brushSurface.peakFragments ),
								 static_cast<unsigned int>( result.brushSurface.peakFragmentPoints ) );
				return;
			}
			common->Warning( "AAS2 build failed in brush surfaces: %s "
							 "at brush %u plane %u",
							 BrushSurfaceErrorName( result.brushSurface.error ),
							 static_cast<unsigned int>( result.brushSurface.sourceBrush ),
							 static_cast<unsigned int>( result.brushSurface.sourcePlane ) );
			return;
		case BuildError::surfaceCompiler:
			common->Warning( "AAS2 build failed in surface compilation: %s "
							 "at triangle %u",
							 ErrorName( result.surface.error ),
							 static_cast<unsigned int>( result.surface.sourceTriangle ) );
			return;
		case BuildError::none:
			return;
	}
}

/*
============
AAS2CompilerSelfTest_f
============
*/
void AAS2CompilerSelfTest_f( const idCmdArgs& args )
{
	( void )args;

	ConvexBrush brush;
	brush.planes =
	{
		{{1.0f, 0.0f, 0.0f}, 32.0f},
		{{ -1.0f, 0.0f, 0.0f}, 0.0f},
		{{0.0f, 1.0f, 0.0f}, 32.0f},
		{{0.0f, -1.0f, 0.0f}, 0.0f},
		{{0.0f, 0.0f, 1.0f}, 64.0f},
		{{0.0f, 0.0f, -1.0f}, 0.0f}
	};
	std::vector<ConvexBrush> expandedBrushes;
	const CSpaceResult cspace = ExpandConfigurationSpace( {brush}, MakeAgentBounds( 16.0f, 72.0f ), expandedBrushes );
	if( !cspace )
	{
		common->Warning( "AAS2 configuration-space self-test failed: %s "
						 "at brush %u plane %u",
						 CSpaceErrorName( cspace.error ),
						 static_cast<unsigned int>( cspace.sourceBrush ),
						 static_cast<unsigned int>( cspace.sourcePlane ) );
		return;
	}

	SourceGeometry source;
	const BrushSurfaceResult surfaces =
		BuildBrushFloorGeometry( expandedBrushes, 45.0f, 0.01f, source );
	if( !surfaces )
	{
		common->Warning( "AAS2 brush-surface self-test failed: %s at brush %u plane %u",
						 BrushSurfaceErrorName( surfaces.error ),
						 static_cast<unsigned int>( surfaces.sourceBrush ),
						 static_cast<unsigned int>( surfaces.sourcePlane ) );
		return;
	}

	File output;
	const Result result = CompileSurface( source, Settings{}, output );
	if( !result )
	{
		common->Warning( "AAS2 compiler self-test failed: %s at triangle %u",
						 ErrorName( result.error ),
						 static_cast<unsigned int>( result.sourceTriangle ) );
		return;
	}

	common->Printf( "AAS2 compiler self-test passed: %u C-space brushes, "
					"%u brush faces, %u floor triangles, %u vertices, %u edges, "
					"%u areas, %u floor merges, %u reachabilities "
					"(%u step, %u fall, %u ledge-grab)\n",
					static_cast<unsigned int>( cspace.brushes ),
					static_cast<unsigned int>( surfaces.boundaryFaces ),
					static_cast<unsigned int>( surfaces.floorTriangles ),
					static_cast<unsigned int>( result.statistics.vertices ),
					static_cast<unsigned int>( result.statistics.edges ),
					static_cast<unsigned int>( result.statistics.areas ),
					static_cast<unsigned int>( result.statistics.mergedFloorPairs ),
					static_cast<unsigned int>( result.statistics.reachabilities ),
					static_cast<unsigned int>( result.statistics.stepReachabilities ),
					static_cast<unsigned int>( result.statistics.fallReachabilities ),
					static_cast<unsigned int>( result.statistics.ledgeGrabReachabilities ) );
}

/*
============
AAS2ResolveMapPath

Accepts a map name typed relative to the maps/ directory (e.g.
"game/alphalabs2.map") and returns the full path idMapFile::Parse
expects (e.g. "maps/game/alphalabs2.map"). If the caller already
included the "maps/" prefix, it is left as-is.
============
*/
static idStr AAS2ResolveMapPath( const char* rawName )
{
	idStr path = rawName;
	path.BackSlashesToSlashes();
	if( path.Icmpn( "maps/", 5 ) != 0 )
	{
		path.Insert( "maps/", 0 );
	}
	return path;
}

/*
============
AAS2HasExtraEntsSuffix
============
*/
static bool AAS2HasExtraEntsSuffix( const idStr& baseName )
{
	static const char* const suffix = "_extra_ents";
	const int suffixLen = 11;
	if( baseName.Length() < suffixLen )
	{
		return false;
	}
	const idStr tail = baseName.Mid( baseName.Length() - suffixLen, suffixLen );
	return idStr::Icmp( tail, suffix ) == 0;
}

/*
============
AAS2BuildMap
============
*/
static bool AAS2BuildMap( const char* mapPath, const char* profileArg )
{
	BuildSettings buildSettings;
	BinaryMetadata metadata;
	idStr profileName;
	if( !ReadAAS2Profile( profileArg, buildSettings, metadata, profileName ) )
	{
		return false;
	}

	idMapFile mapFile;
	common->Printf( "AAS2 parsing map '%s'...\n", mapPath );
	if( !mapFile.Parse( mapPath ) )
	{
		common->Warning( "AAS2 could not parse map '%s'", mapPath );
		return false;
	}
	std::vector<ConvexBrush> brushes;
	std::vector<AAS2AreaVolume> areaVolumes;
	AAS2MapStatistics mapStatistics;
	AAS2PrimitivePolicy primitivePolicy;
	primitivePolicy.includeBrushes = metadata.primitiveModeBrush == 0;
	primitivePolicy.includePatches = metadata.primitiveModePatch == 0;
	if( !CollectAAS2Brushes( mapFile, primitivePolicy,
							 brushes, areaVolumes,
							 mapStatistics ) )
	{
		common->Warning( "AAS2 found no participating solid brushes in '%s'",
						 mapPath );
		return false;
	}
	common->Printf( "AAS2 collected %u solid brushes and %u dynamic volumes\n",
					static_cast<unsigned int>( brushes.size() ),
					static_cast<unsigned int>( areaVolumes.size() ) );
	common->Printf( "AAS2 primitive policy: brushes %s (%u map brushes, %u "
					"ignored), patches %s (%u patches, %u generated triangles, "
					"%u ignored)\n",
					primitivePolicy.includeBrushes ? "enabled" : "disabled",
					static_cast<unsigned int>( mapStatistics.brushPrimitives ),
					static_cast<unsigned int>( mapStatistics.ignoredBrushes ),
					primitivePolicy.includePatches ? "enabled" : "disabled",
					static_cast<unsigned int>( mapStatistics.patchPrimitives ),
					static_cast<unsigned int>( mapStatistics.acceptedPatchBrushes ),
					static_cast<unsigned int>( mapStatistics.ignoredPatches ) );

	File output;
	AAS2ProgressState progress;
	progress.startTime = Sys_Milliseconds();
	buildSettings.progress = AAS2BuildProgress;
	buildSettings.progressUserData = &progress;

	common->SetRefreshOnPrint( true );

	const BuildResult result = CompileBrushes( brushes, buildSettings, output );

	common->SetRefreshOnPrint( false );

	if( !result )
	{
		PrintAAS2BuildFailure( result );
		return false;
	}

	ApplyAAS2AreaVolumes( output, areaVolumes, buildSettings.agent, mapStatistics );

	const size_t dynamicReachabilities = AddDynamicVolumeReachabilities( output, buildSettings.navigation );
	common->Printf( "AAS2 compiled model: %u vertices, %u edges, %u areas, "
					"%u reachabilities (%u dynamic-volume), %u nodes\n",
					static_cast<unsigned int>( output.vertices.size() ),
					static_cast<unsigned int>( output.edges.size() ),
					static_cast<unsigned int>( output.areas.size() ),
					static_cast<unsigned int>( output.reachabilities.size() ),
					static_cast<unsigned int>( dynamicReachabilities ),
					static_cast<unsigned int>( output.nodes.size() ) );

	metadata.mapCRC = mapFile.GetGeometryCRC();
	metadata.sourceTimestamp = static_cast<unsigned int>( mapFile.GetFileTime() );
	metadata.name = mapFile.GetName();
	metadata.sourcePath = mapPath;

	std::vector<uint8> bytes;
	const BinaryError binary = SerializeAAS2( output, metadata, bytes );
	const idStr outputName = AAS2OutputName( mapPath, metadata.fileExtensionAAS.c_str() );
	if( binary != BinaryError::none )
	{
		common->Warning( "AAS2 failed to serialize '%s': %s "
						 "(%u areas, %u reachabilities)", outputName.c_str(),
						 BinaryErrorName( binary ),
						 static_cast<unsigned int>( output.areas.size() ),
						 static_cast<unsigned int>( output.reachabilities.size() ) );
		return false;
	}

	idFile* destination = fileSystem->OpenFileWrite( outputName, "fs_basepath" );
	if( destination == NULL || destination->Write( bytes.data(), bytes.size() ) != bytes.size() )
	{
		if( destination != NULL )
		{
			fileSystem->CloseFile( destination );
		}
		common->Warning( "AAS2 failed to write '%s'", outputName.c_str() );
		return false;
	}

	fileSystem->CloseFile( destination );

	common->Printf( "AAS2 wrote '%s' using profile '%s'\n", outputName.c_str(), profileName.c_str() );

	common->Printf( "AAS2 floor CSG: %u broad-phase pairs, %u rejected before "
					"clipping, peak %u fragments / %u points per face\n",
					static_cast<unsigned int>( result.brushSurface.candidatePairs ),
					static_cast<unsigned int>( result.brushSurface.rejectedCandidatePairs ),
					static_cast<unsigned int>( result.brushSurface.peakFragments ),
					static_cast<unsigned int>( result.brushSurface.peakFragmentPoints ) );

	common->Printf( "AAS2 discarded %u collapsed CSG triangles, %u triangles "
					"collapsed by surface welding, and %u duplicate triangles; "
					"%u multi-owned edges remain isolated\n",
					static_cast<unsigned int>( result.brushSurface.discardedDegenerateTriangles ),
					static_cast<unsigned int>( result.surface.statistics.rejectedDegenerateTriangles ),
					static_cast<unsigned int>( result.surface.statistics.rejectedDuplicateTriangles ),
					static_cast<unsigned int>( result.surface.statistics.nonManifoldEdges ) );

	common->Printf( "AAS2 in-memory map build passed: %u/%u brushes, "
					"%u ignored patches, %u floor faces, %u areas, "
					"%u dynamic volumes tagged %u areas, %u reachabilities "
					"(%u step, %u fall, %u ledge-grab)\n",
					static_cast<unsigned int>( mapStatistics.acceptedBrushes ),
					static_cast<unsigned int>( mapStatistics.brushPrimitives ),
					static_cast<unsigned int>( mapStatistics.ignoredPatches ),
					static_cast<unsigned int>( result.brushSurface.floorFaces ),
					static_cast<unsigned int>( result.surface.statistics.areas ),
					static_cast<unsigned int>( mapStatistics.areaVolumes ),
					static_cast<unsigned int>( mapStatistics.taggedAreas ),
					static_cast<unsigned int>( result.surface.statistics.reachabilities ),
					static_cast<unsigned int>( result.surface.statistics.stepReachabilities ),
					static_cast<unsigned int>( result.surface.statistics.fallReachabilities ),
					static_cast<unsigned int>( result.surface.statistics.ledgeGrabReachabilities ) );

	return true;
}

/*
============
AAS2Build_f
============
*/
void AAS2Build_f( const idCmdArgs& args )
{
	if( args.Argc() < 2 || args.Argc() > 3 )
	{
		common->Printf( "usage: aas2build <mapName> [aasType]\n"
						"example: aas2build game/alphalabs2.map aas48\n" );
		return;
	}

	const idStr mapPath = AAS2ResolveMapPath( args.Argv( 1 ) );
	AAS2BuildMap( mapPath.c_str(), args.Argc() == 3 ? args.Argv( 2 ) : "aas48" );
}

/*
============
AAS2BuildAll_f
============
*/
void AAS2BuildAll_f( const idCmdArgs& args )
{
	if( args.Argc() > 3 )
	{
		common->Printf( "usage: aas2buildall [folder] [aasType]\n"
						"example: aas2buildall game aas48\n"
						"omit folder to process every map under maps/\n" );
		return;
	}

	idStr folder = args.Argc() >= 2 ? args.Argv( 1 ) : "";
	folder.BackSlashesToSlashes();
	while( folder.Length() > 0 && folder.Icmpn( "/", 1 ) == 0 )
	{
		folder = folder.Mid( 1, folder.Length() - 1 );
	}
	while( folder.Length() > 0 && folder.Mid( folder.Length() - 1, 1 ).Icmpn( "/", 1 ) == 0 )
	{
		folder = folder.Mid( 0, folder.Length() - 1 );
	}

	idStr relativePath = "maps";
	if( folder.Length() > 0 )
	{
		relativePath += "/";
		relativePath += folder;
	}

	idFileList* mapList = fileSystem->ListFilesTree( relativePath, ".map", true );
	if( mapList == NULL || mapList->GetNumFiles() == 0 )
	{
		common->Warning( "AAS2 found no .map files under '%s'", relativePath.c_str() );
		if( mapList != NULL )
		{
			fileSystem->FreeFileList( mapList );
		}
		return;
	}

	const char* profileArg = args.Argc() == 3 ? args.Argv( 2 ) : "aas48";

	int built = 0;
	int skipped = 0;
	int failed = 0;
	for( int i = 0; i < mapList->GetNumFiles(); i++ )
	{
		idStr mapPath = mapList->GetFile( i );
		mapPath.BackSlashesToSlashes();

		idStr baseName = mapPath;
		baseName.StripFileExtension();
		if( AAS2HasExtraEntsSuffix( baseName ) )
		{
			common->Printf( "AAS2 skipping '%s' (_extra_ents)\n", mapPath.c_str() );
			skipped++;
			continue;
		}

		if( AAS2BuildMap( mapPath.c_str(), profileArg ) )
		{
			built++;
		}
		else
		{
			failed++;
		}
	}

	fileSystem->FreeFileList( mapList );

	common->Printf( "AAS2 batch build finished: %d built, %d skipped, %d failed\n",
					built, skipped, failed );
}