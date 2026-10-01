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

#include "compiler_resources.h"
#include "compiler_public.h"
#include "dmap/dmap.h"

/*
============
DmapFindMaterial
============
*/
const idMaterial* DmapFindMaterial( const char* name )
{
	for( int i = 0; i < dmapGlobals.materialNames.Num(); i++ )
	{
		if( dmapGlobals.materialNames[i].Icmp( name ) == 0 )
		{
			return dmapGlobals.materials[i];
		}
	}

	if( !idLib::IsMainThread() )
	{
		idLib::Error( "Material '%s' was not ready on the main thread", name );
		return NULL;
	}

	const idMaterial* material = declManager->FindMaterial( name );
	dmapGlobals.materialNames.Append( name );
	dmapGlobals.materials.Append( material );
	return material;
}

/*
============
DmapFindRenderModel
============
*/
idRenderModel* DmapFindRenderModel( const char* name )
{
	for( int i = 0; i < dmapGlobals.renderModelNames.Num(); i++ )
	{
		if( dmapGlobals.renderModelNames[i].Icmp( name ) == 0 )
		{
			return dmapGlobals.renderModels[i];
		}
	}

	if( !idLib::IsMainThread() )
	{
		idLib::Error( "Render model '%s' was not ready on the main thread", name );
		return NULL;
	}

	idRenderModel* model = renderModelManager->FindModel( name );
	dmapGlobals.renderModelNames.Append( name );
	dmapGlobals.renderModels.Append( model );
	return model;
}

/*
============
NormalizeMapDirectory
============
*/
static idStr NormalizeMapDirectory( const char* directory )
{
	idStr path = directory;
	path.BackSlashesToSlashes();
	if( path.Icmpn( "maps/", 5 ) != 0 )
	{
		path = "maps/" + path;
	}
	return path;
}

/*
============
DMapDirectoryPath
============
*/
idStr DMapDirectoryPath( const char* directory )
{
	return NormalizeMapDirectory( directory );
}

/*
============
AASMapDirectoryPath
============
*/
idStr AASMapDirectoryPath( const char* directory )
{
	return NormalizeMapDirectory( directory );
}


/*
============
AASPreloadDefinitions
============
*/
void AASPreloadDefinitions()
{
	if( !idLib::IsMainThread() )
	{
		idLib::Error( "AAS declarations must be ready on the main thread" );
		return;
	}

	const idDict* aasTypes = gameEdit->FindEntityDefDict( "aas_types", false );
	if( !aasTypes )
	{
		return;
	}

	for( const idKeyValue* type = aasTypes->MatchPrefix( "type" ); type; type = aasTypes->MatchPrefix( "type", type ) )
	{
		gameEdit->FindEntityDefDict( type->GetValue(), false );
	}
}

/*
============
AASPreloadMap
============
*/
void AASPreloadMap( const idMapFile* mapFile )
{
	if( !idLib::IsMainThread() )
	{
		idLib::Error( "AAS map resources must be ready on the main thread" );
		return;
	}

	for( int entityIndex = 0; entityIndex < mapFile->GetNumEntities(); entityIndex++ )
	{
		const idMapEntity* entity = mapFile->GetEntity( entityIndex );
		const char* className = entity->epairs.GetString( "classname", "" );
		if( className[0] )
		{
			gameEdit->FindEntityDefDict( className, false );
			declManager->FindType( DECL_ENTITYDEF, className, false );
		}

		for( int primitiveIndex = 0; primitiveIndex < entity->GetNumPrimitives(); primitiveIndex++ )
		{
			const idMapPrimitive* primitive = entity->GetPrimitive( primitiveIndex );
			switch( primitive->GetType() )
			{
				case idMapPrimitive::TYPE_BRUSH:
				{
					const idMapBrush* brush = static_cast<const idMapBrush*>( primitive );
					for( int sideIndex = 0; sideIndex < brush->GetNumSides(); sideIndex++ )
					{
						declManager->FindMaterial( brush->GetSide( sideIndex )->GetMaterial() );
					}
					break;
				}
				case idMapPrimitive::TYPE_PATCH:
					declManager->FindMaterial( static_cast<const idMapPatch*>( primitive )->GetMaterial() );
					break;
				case idMapPrimitive::TYPE_MESH:
				{
					const idMapPolygonMesh* mesh = static_cast<const idMapPolygonMesh*>( primitive );
					for( int faceIndex = 0; faceIndex < mesh->GetNumPolygons(); faceIndex++ )
					{
						declManager->FindMaterial( mesh->GetFace( faceIndex ).GetMaterial() );
					}
					break;
				}
			}
		}
	}
}

/*
============
AASPreloadMapFile
============
*/
void AASPreloadMapFile( const char* mapName )
{
	if( !idLib::IsMainThread() )
	{
		idLib::Error( "map materials must be ready on the main thread" );
		return;
	}

	idStr path = mapName;
	path.BackSlashesToSlashes();
	if( path.Icmpn( "maps/", 5 ) != 0 )
	{
		path = "maps/" + path;
	}
	path.SetFileExtension( "map" );

	idMapFile mapFile;
	if( !mapFile.Parse( path ) )
	{
		return;
	}

	AASPreloadMap( &mapFile );
}

/*
============
AASPreloadMapDirectory
============
*/
void AASPreloadMapDirectory( const char* directory )
{
	if( !idLib::IsMainThread() )
	{
		idLib::Error( "AAS map resources must be ready on the main thread" );
		return;
	}

	idStr mapDirectory = AASMapDirectoryPath( directory );
	idFileList* mapFiles = fileSystem->ListFiles( mapDirectory, ".map" );
	for( int i = 0; mapFiles && i < mapFiles->GetNumFiles(); i++ )
	{
		idStr mapName = mapDirectory + "/" + mapFiles->GetFile( i );
		AASPreloadMapFile( mapName );
	}
	if( mapFiles )
	{
		fileSystem->FreeFileList( mapFiles );
	}
}