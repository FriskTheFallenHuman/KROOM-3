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

#include <wx/wx.h>

#include "MaterialDoc.h"
#include "MaterialView.h"

/*
================
MaterialDoc::MaterialDoc
================
*/
MaterialDoc::MaterialDoc()
{
	modified = false;
	applyWaiting = false;
	sourceModify = false;
}

/*
================
MaterialDoc::~MaterialDoc
================
*/
MaterialDoc::~MaterialDoc()
{
	ClearEditMaterial();
}

/*
================
MaterialDoc::SetRenderMaterial
================
*/
void MaterialDoc::SetRenderMaterial( idMaterial* material, bool parseMaterial, bool parseRenderMatierial )
{

	renderMaterial = material;


	if( !parseMaterial ||  !renderMaterial )
	{
		return;
	}

	if( parseRenderMatierial )
	{
		char* declText = ( char* ) _alloca( material->GetTextLength() + 1 );
		material->GetText( declText );

		renderMaterial->GetText( declText );
		ParseMaterialText( declText );

	}

	ClearEditMaterial();

	name = material->GetName();

	idLexer		src;

	char* declText = ( char* ) _alloca( material->GetTextLength() + 1 );
	material->GetText( declText );

	renderMaterial->GetText( declText );
	src.LoadMemory( declText, strlen( declText ), "Material" );

	ParseMaterial( &src );
}

/*
================
MaterialDoc::GetStageCount
================
*/
int	MaterialDoc::GetStageCount()
{
	return editMaterial.stages.Num();
}

/*
================
MaterialDoc::FindStage
================
*/
int	MaterialDoc::FindStage( int stageType, const char* name )
{

	for( int i = 0; i < editMaterial.stages.Num(); i++ )
	{
		int type = GetAttributeInt( i, "stagetype" );
		idStr localname = GetAttribute( i, "name" );
		if( stageType == type && !localname.Icmp( name ) )
		{
			return i;
		}
	}
	return -1;
}

/*
================
MaterialDoc::GetStage
================
*/
MEStage_t MaterialDoc::GetStage( int stage )
{
	assert( stage >= 0 && stage < GetStageCount() );
	return *editMaterial.stages[stage];

}

/*
================
MaterialDoc::EnableStage
================
*/
void MaterialDoc::EnableStage( int stage, bool enabled )
{

	assert( stage >= 0 && stage < GetStageCount() );
	editMaterial.stages[stage]->enabled = enabled;

	OnMaterialChanged();
}

/*
================
MaterialDoc::EnableAllStages
================
*/
void MaterialDoc::EnableAllStages( bool enabled )
{
	for( int i = 0; i < GetStageCount(); i++ )
	{
		editMaterial.stages[i]->enabled = enabled;
	}
}

/*
================
MaterialDoc::IsStageEnabled
================
*/
bool MaterialDoc::IsStageEnabled( int stage )
{
	assert( stage >= 0 && stage < GetStageCount() );
	return editMaterial.stages[stage]->enabled;
}

/*
================
MaterialDoc::GetAttribute
================
*/
const char*	MaterialDoc::GetAttribute( int stage, const char* attribName, const char* defaultString )
{

	if( stage == -1 )
	{
		return editMaterial.materialData.GetString( attribName, defaultString );
	}
	else
	{
		assert( stage >= 0 && stage < GetStageCount() );
		MEStage_t* pStage = editMaterial.stages[stage];
		return pStage->stageData.GetString( attribName, defaultString );
	}
}

/*
================
MaterialDoc::GetAttributeInt
================
*/
int MaterialDoc::GetAttributeInt( int stage, const char* attribName, const char* defaultString )
{
	if( stage == -1 )
	{
		return editMaterial.materialData.GetInt( attribName, defaultString );
	}
	else
	{
		assert( stage >= 0 && stage < GetStageCount() );
		MEStage_t* pStage = editMaterial.stages[stage];
		return pStage->stageData.GetInt( attribName, defaultString );
	}
}

/*
================
MaterialDoc::GetAttributeFloat
================
*/
float MaterialDoc::GetAttributeFloat( int stage, const char* attribName, const char* defaultString )
{
	if( stage == -1 )
	{
		return editMaterial.materialData.GetFloat( attribName, defaultString );
	}
	else
	{
		assert( stage >= 0 && stage < GetStageCount() );
		MEStage_t* pStage = editMaterial.stages[stage];
		return pStage->stageData.GetFloat( attribName, defaultString );
	}
}

/*
================
MaterialDoc::GetAttributeBool
================
*/
bool MaterialDoc::GetAttributeBool( int stage, const char* attribName, const char* defaultString )
{
	if( stage == -1 )
	{
		return editMaterial.materialData.GetBool( attribName, defaultString );
	}
	else
	{
		assert( stage >= 0 && stage < GetStageCount() );
		MEStage_t* pStage = editMaterial.stages[stage];
		return pStage->stageData.GetBool( attribName, defaultString );
	}
}

/*
================
MaterialDoc::SetAttribute
================
*/
void MaterialDoc::SetAttribute( int stage, const char* attribName, const char* value, bool addUndo )
{

	//Make sure we need to set the attribute
	idStr orig  = GetAttribute( stage, attribName );
	if( orig.Icmp( value ) )
	{

		idDict* dict;
		if( stage == -1 )
		{
			dict = &editMaterial.materialData;
		}
		else
		{
			assert( stage >= 0 && stage < GetStageCount() );
			dict = &editMaterial.stages[stage]->stageData;
		}

		if( addUndo )
		{
			//Create a new Modifier for this change so we can undo and redo later
			AttributeMaterialModifierString* mod = new AttributeMaterialModifierString( manager, name, stage, attribName, value, orig );
			manager->AddMaterialUndoModifier( mod );
		}

		dict->Set( attribName, value );

		manager->AttributeChanged( this, stage, attribName );
		OnMaterialChanged();
	}
}

/*
================
MaterialDoc::SetAttributeInt
================
*/
void MaterialDoc::SetAttributeInt( int stage, const char* attribName, int value, bool addUndo )
{
	//Make sure we need to set the attribute
	int orig  = GetAttributeInt( stage, attribName );
	if( orig != value )
	{

		idDict* dict;
		if( stage == -1 )
		{
			dict = &editMaterial.materialData;
		}
		else
		{
			assert( stage >= 0 && stage < GetStageCount() );
			dict = &editMaterial.stages[stage]->stageData;
		}

		dict->SetInt( attribName, value );

		manager->AttributeChanged( this, stage, attribName );
		OnMaterialChanged();
	}
}

/*
================
MaterialDoc::SetAttributeFloat
================
*/
void MaterialDoc::SetAttributeFloat( int stage, const char* attribName, float value, bool addUndo )
{
	//Make sure we need to set the attribute
	float orig  = GetAttributeFloat( stage, attribName );
	if( orig != value )
	{

		idDict* dict;
		if( stage == -1 )
		{
			dict = &editMaterial.materialData;
		}
		else
		{
			assert( stage >= 0 && stage < GetStageCount() );
			dict = &editMaterial.stages[stage]->stageData;
		}

		dict->SetFloat( attribName, value );

		manager->AttributeChanged( this, stage, attribName );
		OnMaterialChanged();
	}
}

/*
================
MaterialDoc::SetAttributeBool
================
*/
void MaterialDoc::SetAttributeBool( int stage, const char* attribName, bool value, bool addUndo )
{
	//Make sure we need to set the attribute
	bool orig  = GetAttributeBool( stage, attribName );
	if( orig != value )
	{

		idDict* dict;
		if( stage == -1 )
		{
			dict = &editMaterial.materialData;
		}
		else
		{
			assert( stage >= 0 && stage < GetStageCount() );
			dict = &editMaterial.stages[stage]->stageData;
		}

		if( addUndo )
		{
			//Create a new Modifier for this change so we can undo and redo later
			AttributeMaterialModifierBool* mod = new AttributeMaterialModifierBool( manager, name, stage, attribName, value, orig );
			manager->AddMaterialUndoModifier( mod );
		}

		dict->SetBool( attribName, value );

		manager->AttributeChanged( this, stage, attribName );
		OnMaterialChanged();
	}
}

/*
================
MaterialDoc::SetMaterialName
================
*/
void MaterialDoc::SetMaterialName( const char* materialName, bool addUndo )
{
	idStr oldName = name;

	declManager->RenameDecl( DECL_MATERIAL, oldName, materialName );
	name = renderMaterial->GetName();

	if( addUndo )
	{
		RenameMaterialModifier* mod = new RenameMaterialModifier( manager, name, oldName );
		manager->AddMaterialUndoModifier( mod );
	}

	manager->MaterialNameChanged( oldName, this );

	OnMaterialChanged();

	//Need to do an instant apply for material name changes
	ApplyMaterialChanges();
}

/*
================
MaterialDoc::SetData
================
*/
void MaterialDoc::SetData( int stage, idDict* data )
{
	idDict* dict;
	if( stage == -1 )
	{
		dict = &editMaterial.materialData;
	}
	else
	{
		assert( stage >= 0 && stage < GetStageCount() );
		dict = &editMaterial.stages[stage]->stageData;
	}
	dict->Clear();
	dict->Copy( *data );
}

/*
================
MaterialDoc::SourceModify
================
*/
void MaterialDoc::SourceModify( SourceModifyOwner* owner )
{

	sourceModifyOwner = owner;
	sourceModify = true;
	OnMaterialChanged();
}

/*
================
MaterialDoc::IsSourceModified
================
*/
bool MaterialDoc::IsSourceModified()
{
	return sourceModify;
}

/*
================
MaterialDoc::ApplySourceModify
================
*/
void MaterialDoc::ApplySourceModify( idStr& text )
{

	if( sourceModify )
	{

		//Changes in the source need to clear any undo redo buffer because we have no idea what has changed
		manager->ClearUndo();
		manager->ClearRedo();

		ClearEditMaterial();

		idLexer		src;
		src.LoadMemory( text, text.Length(), "Material" );

		src.SetFlags(
			LEXFL_NOSTRINGCONCAT |			// multiple strings seperated by whitespaces are not concatenated
			LEXFL_NOSTRINGESCAPECHARS |		// no escape characters inside strings
			LEXFL_ALLOWPATHNAMES |			// allow path seperators in names
			LEXFL_ALLOWMULTICHARLITERALS |	// allow multi character literals
			LEXFL_ALLOWBACKSLASHSTRINGCONCAT |	// allow multiple strings seperated by '\' to be concatenated
			LEXFL_NOFATALERRORS				// just set a flag instead of fatal erroring
		);

		idToken token;
		if( !src.ReadToken( &token ) )
		{
			src.Warning( "Missing decl name" );
			return;
		}

		ParseMaterial( &src );
		sourceModify = false;

		//Check to see if the name has changed
		if( token.Icmp( name ) )
		{
			SetMaterialName( token, false );
		}
	}
}

/*
================
MaterialDoc::GetEditSourceText
================
*/
const char*	MaterialDoc::GetEditSourceText()
{

	return GenerateSourceText();
}

/*
================
MaterialDoc::AddStage
================
*/
void MaterialDoc::AddStage( int stageType, const char* stageName, bool addUndo )
{
	MEStage_t* newStage = new MEStage_t();

	int index = editMaterial.stages.Append( newStage );
	newStage->stageData.Set( "name", stageName );
	newStage->stageData.SetInt( "stagetype", stageType );
	newStage->enabled = true;

	if( addUndo )
	{
		StageInsertModifier* mod = new StageInsertModifier( manager, name, index, stageType, stageName );
		manager->AddMaterialUndoModifier( mod );
	}

	manager->StageAdded( this, index );

	OnMaterialChanged();
}

/*
================
MaterialDoc::InsertStage
================
*/
void MaterialDoc::InsertStage( int stage, int stageType, const char* stageName, bool addUndo )
{
	MEStage_t* newStage = new MEStage_t();

	editMaterial.stages.Insert( newStage, stage );
	newStage->stageData.Set( "name", stageName );
	newStage->stageData.SetInt( "stagetype", stageType );
	newStage->enabled = true;

	if( addUndo )
	{
		StageInsertModifier* mod = new StageInsertModifier( manager, name, stage, stageType, stageName );
		manager->AddMaterialUndoModifier( mod );
	}

	manager->StageAdded( this, stage );

	OnMaterialChanged();
}

/*
================
MaterialDoc::RemoveStage
================
*/
void MaterialDoc::RemoveStage( int stage, bool addUndo )
{
	assert( stage >= 0 && stage < GetStageCount() );

	if( addUndo )
	{
		//Add modifier to undo this operation
		StageDeleteModifier* mod = new StageDeleteModifier( manager, name, stage, editMaterial.stages[stage]->stageData );
		manager->AddMaterialUndoModifier( mod );
	}

	//delete the stage and remove it from the list
	delete editMaterial.stages[stage];
	editMaterial.stages.RemoveIndex( stage );

	manager->StageDeleted( this, stage );

	OnMaterialChanged();
}

/*
================
MaterialDoc::ClearStages
================
*/
void MaterialDoc::ClearStages()
{

	//Delete each stage and clear the list
	for( int i = GetStageCount() - 1; i >= 0; i-- )
	{
		RemoveStage( i );
	}
}

/*
================
MaterialDoc::MoveStage
================
*/
void MaterialDoc::MoveStage( int from, int to, bool addUndo )
{
	assert( from >= 0 && from < GetStageCount() );
	assert( to >= 0 && to < GetStageCount() );

	int origFrom = from;
	int origTo = to;

	if( from < to )
	{
		to++;
	}

	MEStage_t* pMove = editMaterial.stages[from];
	editMaterial.stages.Insert( pMove, to );

	if( from > to )
	{
		from++;
	}

	editMaterial.stages.RemoveIndex( from );

	manager->StageMoved( this, origFrom, origTo );

	if( addUndo )
	{
		StageMoveModifier* mod = new StageMoveModifier( manager, name, origFrom, origTo );
		manager->AddMaterialUndoModifier( mod );
	}

	OnMaterialChanged();
}

/*
================
MaterialDoc::ApplyMaterialChanges
================
*/
void MaterialDoc::ApplyMaterialChanges( bool force )
{

	if( force || applyWaiting )
	{

		if( sourceModify && sourceModifyOwner )
		{
			idStr text = sourceModifyOwner->GetSourceText();
			ApplySourceModify( text );
		}

		ReplaceSourceText();

		char* declText = ( char* ) _alloca( renderMaterial->GetTextLength() + 1 );
		renderMaterial->GetText( declText );

		renderMaterial->GetText( declText );

		ParseMaterialText( declText );

		applyWaiting = false;

		assert( manager );
		manager->MaterialApplied( this );
	}
}

/*
================
MaterialDoc::Save
================
*/
void MaterialDoc::Save()
{

	EnableAllStages( true );

	//Apply the material so that the renderMaterial has the source text
	if( !deleted )
	{
		ApplyMaterialChanges( true );
	}
	else
	{
		//Replace the text with nothing
		renderMaterial->SetText( " " );
	}

	if( renderMaterial->Save() )
	{

		modified = false;

		//Notify the world
		assert( manager );
		manager->MaterialSaved( this );
	}
	else
	{
		wxMessageBox( wxString::Format( "Unable to save '%s'. It may be read-only", name.c_str() ), "Save Error", wxOK | wxICON_ERROR, GetMaterialEditorWindow() );
	}
}

/*
================
MaterialDoc::Delete
================
*/
void MaterialDoc::Delete()
{
	deleted = true;

	OnMaterialChanged();
}

/*
================
MaterialDoc::OnMaterialChanged
================
*/
void MaterialDoc::OnMaterialChanged()
{

	modified = true;
	applyWaiting = true;

	assert( manager );
	manager->MaterialChanged( this );
}

/*
================
MaterialDoc::ParseMaterialText
================
*/
void MaterialDoc::ParseMaterialText( const char* source )
{
	renderMaterial->Parse( source, strlen( source ), false );
}

/*
================
MaterialDoc::ParseMaterial
================
*/
void MaterialDoc::ParseMaterial( idLexer* src )
{

	idToken		token;

	//Parse past the name
	src->SkipUntilString( "{" );

	while( 1 )
	{
		if( !src->ExpectAnyToken( &token ) )
		{
			//Todo: Add some error checking here
			return;
		}

		if( token == "}" )
		{
			break;
		}

		if( ParseMaterialDef( &token, src, MaterialDefManager::MATERIAL_DEF_MATERIAL, &editMaterial.materialData ) )
		{
			continue;
		}

		if( !token.Icmp( "diffusemap" ) )
		{
			//Added as a special stage
			idStr str;
			src->ReadRestOfLine( str );
			AddSpecialMapStage( "diffusemap", str );
		}
		else if( !token.Icmp( "specularmap" ) )
		{
			idStr str;
			src->ReadRestOfLine( str );
			AddSpecialMapStage( "specularmap", str );
		}
		else if( !token.Icmp( "bumpmap" ) )
		{
			idStr str;
			src->ReadRestOfLine( str );
			AddSpecialMapStage( "bumpmap", str );
		}
		else if( token == "{" )
		{
			ParseStage( src );
		}
	}
}

/*
================
MaterialDoc::ParseStage
================
*/
void MaterialDoc::ParseStage( idLexer* src )
{

	MEStage_t* newStage = new MEStage_t();
	int index = editMaterial.stages.Append( newStage );

	newStage->stageData.SetInt( "stagetype", STAGE_TYPE_NORMAL );
	newStage->enabled = true;

	idToken		token;

	while( 1 )
	{

		if( !src->ExpectAnyToken( &token ) )
		{
			//Todo: Add some error checking here
			return;
		}

		if( token == "}" )
		{
			break;
		}

		if( ParseMaterialDef( &token, src, MaterialDefManager::MATERIAL_DEF_STAGE, &newStage->stageData ) )
		{
			continue;
		}

		if( !token.Icmp( "name" ) )
		{

			idStr str;
			src->ReadRestOfLine( str );
			str.StripTrailing( '\"' );
			str.StripLeading( '\"' );
			newStage->stageData.Set( "name", str );
			continue;
		}
	}

	idStr name;
	newStage->stageData.GetString( "name", "", name );
	if( name.Length() <= 0 )
	{
		newStage->stageData.Set( "name", va( "Stage %d", index + 1 ) );
	}

}

/*
================
MaterialDoc::AddSpecialMapStage
================
*/
void MaterialDoc::AddSpecialMapStage( const char* stageName, const char* map )
{
	MEStage_t* newStage = new MEStage_t();
	newStage->stageData.Set( "name", stageName );
	newStage->stageData.Set( "map", map );
	newStage->stageData.SetInt( "stagetype", STAGE_TYPE_SPECIALMAP );
	newStage->enabled = true;
}

/*
================
MaterialDoc::ParseMaterialDef
================
*/
bool MaterialDoc::ParseMaterialDef( idToken* token, idLexer* src, int type, idDict* dict )
{

	MaterialDefList* defs = MaterialDefManager::GetMaterialDefs( type );

	for( int i = 0; i < defs->Num(); i++ )
	{
		if( !token->Icmp( ( *defs )[i]->dictName ) )
		{

			switch( ( *defs )[i]->type )
			{
				case MaterialDef::MATERIAL_DEF_TYPE_STRING:
				{
					idStr str;
					src->ReadRestOfLine( str );
					if( ( *defs )[i]->quotes )
					{
						str.StripTrailing( '\"' );
						str.StripLeading( '\"' );
					}
					dict->Set( ( *defs )[i]->dictName, str );
				}
				break;
				case MaterialDef::MATERIAL_DEF_TYPE_BOOL:
				{
					src->SkipRestOfLine();
					dict->SetBool( ( *defs )[i]->dictName, true );
				}
				break;
				case MaterialDef::MATERIAL_DEF_TYPE_FLOAT:
				{
					idStr str;
					src->ReadRestOfLine( str );
					dict->Set( ( *defs )[i]->dictName, str );
				}
				break;
				case MaterialDef::MATERIAL_DEF_TYPE_INT:
				{
					idStr str;
					src->ReadRestOfLine( str );
					dict->Set( ( *defs )[i]->dictName, str );
				}
				break;
			}
			return true;
		}
	}
	return false;
}

/*
================
MaterialDoc::ClearEditMaterial
================
*/
void MaterialDoc::ClearEditMaterial()
{

	for( int i = 0; i < GetStageCount(); i++ )
	{
		delete editMaterial.stages[i];
	}
	editMaterial.stages.Clear();
	editMaterial.materialData.Clear();
}

/*
================
MaterialDoc::GenerateSourceText
================
*/
const char*	MaterialDoc::GenerateSourceText()
{

	idFile_Memory f;

	f.WriteFloatString( "\n\n/*\n"
						"\tGenerated by the Material Editor.\n"
						"\tType 'materialeditor' at the console to launch the material editor.\n"
						"*/\n" );

	f.WriteFloatString( "%s\n", name.c_str() );
	f.WriteFloatString( "{\n" );
	WriteMaterialDef( -1, &f, MaterialDefManager::MATERIAL_DEF_MATERIAL, 1 );

	for( int i = 0; i < editMaterial.stages.Num(); i++ )
	{
		if( editMaterial.stages[i]->enabled )
		{
			WriteStage( i, &f );
		}
	}

	f.WriteFloatString( "}\n" );

	return f.GetDataPtr();

}

/*
================
MaterialDoc::ReplaceSourceText
================
*/
void MaterialDoc::ReplaceSourceText()
{
	renderMaterial->SetText( GenerateSourceText() );
}

/*
================
MaterialDoc::WriteStage
================
*/
void MaterialDoc::WriteStage( int stage, idFile_Memory* file )
{

	//idStr stageName = GetAttribute(stage, "name");
	int type = GetAttributeInt( stage, "stagetype" );
	//if(!stageName.Icmp("diffusemap") || !stageName.Icmp("specularmap") || !stageName.Icmp("bumpmap")) {
	if( type == STAGE_TYPE_SPECIALMAP )
	{
		WriteSpecialMapStage( stage, file );
		return;
	}

	file->WriteFloatString( "\t{\n" );
	idStr name = GetAttribute( stage, "name" );
	if( name.Length() > 0 )
	{
		file->WriteFloatString( "\t\tname\t\"%s\"\n", name.c_str() );
	}
	WriteMaterialDef( stage, file, MaterialDefManager::MATERIAL_DEF_STAGE, 2 );
	file->WriteFloatString( "\t}\n" );

}

/*
================
MaterialDoc::WriteSpecialMapStage
================
*/
void MaterialDoc::WriteSpecialMapStage( int stage, idFile_Memory* file )
{
	idStr stageName = GetAttribute( stage, "name" );
	idStr map = GetAttribute( stage, "map" );

	file->WriteFloatString( "\t%s\t%s\n", stageName.c_str(), map.c_str() );
}

/*
================
MaterialDoc::WriteMaterialDef
================
*/
void MaterialDoc::WriteMaterialDef( int stage, idFile_Memory* file, int type, int indent )
{

	idStr prefix = "";
	for( int i = 0; i < indent; i++ )
	{
		prefix += "\t";
	}

	MaterialDefList* defs = MaterialDefManager::GetMaterialDefs( type );
	for( int i = 0; i < defs->Num(); i++ )
	{
		switch( ( *defs )[i]->type )
		{
			case MaterialDef::MATERIAL_DEF_TYPE_STRING:
			{
				idStr attrib = GetAttribute( stage, ( *defs )[i]->dictName );
				if( attrib.Length() > 0 )
				{
					if( ( *defs )[i]->quotes )
					{
						file->WriteFloatString( "%s%s\t\"%s\"\n", prefix.c_str(), ( *defs )[i]->dictName.c_str(), attrib.c_str() );
					}
					else
					{
						file->WriteFloatString( "%s%s\t%s\n", prefix.c_str(), ( *defs )[i]->dictName.c_str(), attrib.c_str() );
					}
				}
			}
			break;
			case MaterialDef::MATERIAL_DEF_TYPE_BOOL:
			{
				if( GetAttributeBool( stage, ( *defs )[i]->dictName ) )
				{
					file->WriteFloatString( "%s%s\t\n", prefix.c_str(), ( *defs )[i]->dictName.c_str() );
				}
			}
			break;
			case MaterialDef::MATERIAL_DEF_TYPE_FLOAT:
			{
				float val = GetAttributeFloat( stage, ( *defs )[i]->dictName );
				file->WriteFloatString( "%s%s\t%f\n", prefix.c_str(), ( *defs )[i]->dictName.c_str(), val );
			}
			break;
			case MaterialDef::MATERIAL_DEF_TYPE_INT:
			{
				int val = GetAttributeInt( stage, ( *defs )[i]->dictName );
				file->WriteFloatString( "%s%s\t%d\n", prefix.c_str(), ( *defs )[i]->dictName.c_str(), val );
			}
			break;
		}
	}
}
