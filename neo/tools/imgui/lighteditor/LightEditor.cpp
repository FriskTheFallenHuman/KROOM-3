/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson
Copyright (C) 2020-2023 Robert Beckebans

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

#include "LightEditor.h"

#include "../../../extern/imgui/imgui_internal.h"

#include "renderer/GLMatrix.h"
#include "renderer/RenderCommon.h"
#include "renderer/Material.h"

void LightInfo::Defaults()
{
	lightType = LIGHT_POINT;

	strTexture = "";
	equalRadius = true;
	explicitStartEnd = false;
	lightStart.Zero();
	lightEnd.Zero();
	lightUp.Zero();
	lightRight.Zero();
	lightTarget.Zero();
	lightCenter.Zero();
	color[0] = color[1] = color[2] = 1.0f;

	lightRadius.Zero();
	castShadows = true;
	skipSpecular = false;
	hasCenter = false;
	lightStyle = -1;

	angles.Zero();
	scale.Set( 1, 1, 1 );
}
void LightInfo::DefaultPoint()
{
	idVec3 oldColor = color;
	Defaults();
	color = oldColor;
	lightType = LIGHT_POINT;
	lightRadius[0] = lightRadius[1] = lightRadius[2] = 300;
	equalRadius = true;
}

void LightInfo::DefaultProjected()
{
	idVec3 oldColor = color;
	Defaults();
	color = oldColor;
	lightType = LIGHT_SPOT;
	lightTarget[2] = -256;
	lightUp[1] = -128;
	lightRight[0] = -128;
}

void LightInfo::DefaultSun()
{
	idVec3 oldColor = color;
	Defaults();
	color = oldColor;
	lightType = LIGHT_SUN;
	lightCenter.Set( 4, 4, 32 );
	lightRadius[0] = lightRadius[1] = 2048;
	lightRadius[2] = 1024;
	equalRadius = false;
}

void LightInfo::FromDict( const idDict* e )
{
	e->GetVector( "origin", "", origin );

	lightRadius.Zero();
	lightTarget.Zero();
	lightRight.Zero();
	lightUp.Zero();
	lightStart.Zero();
	lightEnd.Zero();
	lightCenter.Zero();

	castShadows = !e->GetBool( "noshadows" );
	skipSpecular = e->GetBool( "nospecular" );

	strTexture = e->GetString( "texture" );

	bool isParallel = e->GetBool( "parallel" );

	if( !e->GetVector( "_color", "", color ) )
	{
		color[0] = color[1] = color[2] = 1.0f;
		// NOTE: like the game, imgui uses color values between 0.0 and 1.0
		//       even though it displays them as 0 to 255
	}

	if( e->GetVector( "light_right", "", lightRight ) )
	{
		// projected light
		lightType = LIGHT_SPOT;
		e->GetVector( "light_target", "", lightTarget );
		e->GetVector( "light_up", "", lightUp );
		if( e->GetVector( "light_start", "", lightStart ) )
		{
			// explicit start and end points
			explicitStartEnd = true;
			if( !e->GetVector( "light_end", "", lightEnd ) )
			{
				// no end, use target
				lightEnd = lightTarget;
			}
		}
		else
		{
			explicitStartEnd = false;
			// create a start a quarter of the way to the target
			lightStart = lightTarget * 0.25;
			lightEnd = lightTarget;
		}
	}
	else
	{
		lightType = isParallel ? LIGHT_SUN : LIGHT_POINT;

		if( e->GetVector( "light_radius", "", lightRadius ) )
		{
			equalRadius = ( lightRadius.x == lightRadius.y && lightRadius.x == lightRadius.z );
		}
		else
		{
			float radius = e->GetFloat( "light" );
			if( radius == 0 )
			{
				radius = 300;
			}
			lightRadius[0] = lightRadius[1] = lightRadius[2] = radius;
			equalRadius = true;
		}
		if( e->GetVector( "light_center", "", lightCenter ) )
		{
			hasCenter = true;
		}
	}

	lightStyle = e->GetInt( "style", -1 );

	// get the rotation matrix in either full form, or single angle form
	idMat3 axis;

	if( !e->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", axis ) )
	{
		if( e->GetAngles( "angles", "0 0 0", angles ) )
		{
			axis = angles.ToMat3();
		}
		else
		{
			float angle = e->GetFloat( "angle" );
			if( angle != 0.0f )
			{
				axis = idAngles( 0.0f, angle, 0.0f ).ToMat3();
			}
			else
			{
				axis.Identity();
			}
		}
	}

	angles = axis.ToAngles();
	scale.Set( 1, 1, 1 );
}

// the returned idDict is supposed to be used by idGameEdit::EntityChangeSpawnArgs()
// and thus will contain pairs with value "" if the key should be removed from entity
void LightInfo::ToDict( idDict* e )
{
	e->SetVector( "origin", origin );

	// idGameEdit::EntityChangeSpawnArgs() will delete key/value from entity,
	// if value is "" => use DELETE_VAL for readability
	static const char* DELETE_VAL = "";

	e->Set( "light", DELETE_VAL ); // we always use "light_radius" instead

	e->Set( "noshadows", ( !castShadows ) ? "1" : "0" );
	e->Set( "nospecular", ( skipSpecular ) ? "1" : "0" );

	if( strTexture.Length() > 0 )
	{
		e->Set( "texture", strTexture );
	}
	else
	{
		e->Set( "texture", DELETE_VAL );
	}

	// NOTE: e->SetVector() uses precision of 2, not enough for color
	e->Set( "_color", color.ToString( 4 ) );

	if( lightType == LIGHT_POINT || lightType == LIGHT_SUN )
	{
		if( !equalRadius )
		{
			e->SetVector( "light_radius", lightRadius );
		}
		else
		{
			idVec3 tmp( lightRadius[0] ); // x, y and z have the same value
			e->SetVector( "light_radius", tmp );
		}

		if( hasCenter )
		{
			e->SetVector( "light_center", lightCenter );
		}
		else
		{
			e->Set( "light_center", DELETE_VAL );
		}

		if( lightType == LIGHT_SUN )
		{
			e->Set( "parallel", "1" );
			e->Set( "style", DELETE_VAL );
		}

		// get rid of all the projected light specific stuff
		e->Set( "light_target", DELETE_VAL );
		e->Set( "light_up", DELETE_VAL );
		e->Set( "light_right", DELETE_VAL );
		e->Set( "light_start", DELETE_VAL );
		e->Set( "light_end", DELETE_VAL );
	}
	else
	{
		e->SetVector( "light_target", lightTarget );
		e->SetVector( "light_up", lightUp );
		e->SetVector( "light_right", lightRight );
		if( explicitStartEnd )
		{
			e->SetVector( "light_start", lightStart );
			e->SetVector( "light_end", lightEnd );
		}
		else
		{
			e->Set( "light_start", DELETE_VAL );
			e->Set( "light_end", DELETE_VAL );
		}

		// get rid of the pointlight specific stuff
		e->Set( "light_radius", DELETE_VAL );
		e->Set( "light_center", DELETE_VAL );
		e->Set( "parallel", DELETE_VAL );
	}

	if( lightStyle != -1 && lightType != LIGHT_SUN )
	{
		e->SetInt( "style", lightStyle );
	}
	else
	{
		e->Set( "style", DELETE_VAL );
	}

	e->Set( "rotation", DELETE_VAL );
	//if( angles.yaw != 0.0f || angles.pitch != 0.0f || angles.roll != 0.0f )
	{
		e->SetAngles( "angles", angles );
	}
}
LightInfo::LightInfo()
{
	Defaults();
}


// ########### LightEditor #############

LightEditor& LightEditor::Instance()
{
	static LightEditor instance;
	return instance;
}


// static
void LightEditor::ReInit( const idDict* dict, idEntity* light )
{
	Instance().Init( dict, light );
}

void LightEditor::Init( const idDict* dict, idEntity* light )
{
	Reset();

	if( textureNames.Num() == 0 )
	{
		LoadLightTextures();
	}

	if( styleNames.Num() == 0 )
	{
		LoadLightStyles();
	}

	if( dict )
	{
		original.FromDict( dict );
		cur.FromDict( dict );

		gameEdit->EntityGetOrigin( light, entityPos );

		idStr name = dict->GetString( "name", NULL );
		entityName = name.Length() ? name.c_str() :  gameEdit->GetUniqueEntityName( "light" );
		name.Format( "Light Editor: %s at (%s)###LightEditor", entityName.c_str(), entityPos.ToString() );
		title = name;

		currentTextureIndex = 0;
		currentTexture = NULL;
		if( original.strTexture.Length() > 0 )
		{
			const char* curTex = original.strTexture.c_str();
			for( int i = 0; i < textureNames.Num(); ++i )
			{
				if( textureNames[i] == curTex )
				{
					currentTextureIndex = i + 1; // remember, 0 is "<No Texture>"
					LoadCurrentTexture();
					break;
				}
			}
		}

		if( original.lightStyle >= 0 )
		{
			currentStyleIndex = original.lightStyle + 1;
		}
	}

	this->lightEntity = light;
}

void LightEditor::Reset()
{
	title = "Light Editor: no Light selected!";
	entityPos.x = idMath::INFINITUM;
	entityPos.y = idMath::INFINITUM;
	entityPos.z = idMath::INFINITUM;

	original.Defaults();
	cur.Defaults();

	lightEntity = NULL;
	currentTextureIndex = 0;
	currentTexture = NULL;
	currentTextureMaterial = NULL;
	currentStyleIndex = 0;

	mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	mCurrentGizmoMode = ImGuizmo::WORLD;

	useSnap = false;
	//snap = { 1.f, 1.f, 1.f };
	//bounds[] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
	//boundsSnap[] = { 0.1f, 0.1f, 0.1f };
	boundSizing = false;
	boundSizingSnap = false;

	shortcutSaveMapEnabled = true;
	shortcutDuplicateLightEnabled = true;
}

class idSort_textureNames : public idSort_Quick< idStr, idSort_textureNames >
{
public:
	int Compare( const idStr& a, const idStr& b ) const
	{
		return a.Icmp( b );
	}
};

void LightEditor::LoadLightTextures()
{
	textureNames.Clear();

	int count = declManager->GetNumDecls( DECL_MATERIAL );

	for( int i = 0; i < count; i++ )
	{
		// just get the name of the light material
		const idMaterial* mat = declManager->MaterialByIndex( i, false );

		idStr matName = mat->GetName();
		matName.ToLower();

		if( matName.Icmpn( "lights/", strlen( "lights/" ) ) == 0 || matName.Icmpn( "fogs/", strlen( "fogs/" ) ) == 0 )
		{
			// actually load the material
			const idMaterial* material = declManager->FindMaterial( matName, false );
			if( material != NULL )
			{
				// check if the material has textures or is just a leftover from the development
				idImage* editorImage = mat->GetEditorImage();
				if( !editorImage->IsLoaded() )
				{
					editorImage->ActuallyLoadImage( false );
				}

				if( !editorImage->IsDefaulted() )
				{
					textureNames.Append( matName );
				}
			}
		}
	}

	textureNames.SortWithTemplate( idSort_textureNames() );
}

// static
const char* LightEditor::TextureItemsGetter( void* data, int idx )
{
	LightEditor* self = static_cast<LightEditor*>( data );
	if( idx == 0 )
	{
		return "<No Texture>";
	}

	// as index 0 has special purpose, the "real" index is one less
	--idx;

	if( idx < 0 || idx >= self->textureNames.Num() )
	{
		return "<Invalid Index!>";
	}

	return self->textureNames[idx].c_str();
}

void LightEditor::LoadCurrentTexture()
{
	currentTexture = NULL;
	currentTextureMaterial = NULL;

	if( currentTextureIndex > 0 && cur.strTexture.Length() > 0 )
	{
		const idMaterial* mat = declManager->FindMaterial( cur.strTexture, false );
		if( mat != NULL )
		{
			currentTexture = mat->GetEditorImage();
			if( currentTexture )
			{
				// RB: create extra 2D material of the image for UI rendering

				// HACK that deserves being called a hack
				idStr uiName( "lighteditor/" );
				uiName += currentTexture->GetName();

				currentTextureMaterial = declManager->FindMaterial( uiName, true );
			}
		}
	}
}

bool LightEditor::DrawLightTextureBrowser()
{
	bool changed = false;

	if( ImGui::Begin( "Light Texture Browser" ) )
	{
		ImGui::BeginChild( "LightTextureNames", ImVec2( 240.0f, 0.0f ), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX );
		ImGui::SetCursorPosY( 6.0f );
		if( ImGui::Selectable( "<No Texture>", currentTextureIndex == 0 ) )
		{
			currentTextureIndex = 0;
			cur.strTexture = "";
			LoadCurrentTexture();
			changed = true;
		}
		for( int i = 0; i < textureNames.Num(); ++i )
		{
			const bool selected = currentTextureIndex == i + 1;
			if( ImGui::Selectable( textureNames[i].c_str(), selected ) )
			{
				currentTextureIndex = i + 1;
				cur.strTexture = textureNames[i];
				LoadCurrentTexture();
				changed = true;
			}
			if( selected )
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild( "LightTexturePreview", ImVec2( 0.0f, 0.0f ), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar );
		ImGui::SetCursorPos( ImVec2( 6.0f, 6.0f ) );
		const float cellWidth = 112.0f;
		const float thumbnailSize = 72.0f;
		const int columns = Max( 1, ( int )( ImGui::GetContentRegionAvail().x / cellWidth ) );
		for( int i = 0; i < textureNames.Num(); ++i )
		{
			const idMaterial* material = declManager->FindMaterial( textureNames[i], false );
			idImage* image = material != NULL ? material->GetEditorImage() : NULL;
			if( image != NULL )
			{
				idStr uiName( "lighteditor/" );
				uiName += image->GetName();
				const idMaterial* previewMaterial = declManager->FindMaterial( uiName, true );
				if( previewMaterial != NULL )
				{
					ImGui::BeginGroup();
					ImGui::Image( ( void* )previewMaterial, ImVec2( thumbnailSize, thumbnailSize ) );
					if( ImGui::Selectable( textureNames[i].c_str(), currentTextureIndex == i + 1, 0, ImVec2( cellWidth - 8.0f, 0.0f ) ) )
					{
						currentTextureIndex = i + 1;
						cur.strTexture = textureNames[i];
						LoadCurrentTexture();
						changed = true;
					}
					ImGui::EndGroup();
					if( ( i + 1 ) % columns != 0 )
					{
						ImGui::SameLine();
					}
				}
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();

	return changed;
}

void LightEditor::LoadLightStyles()
{
	styleNames.Clear();

	const idDeclEntityDef* decl = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, "light", false ) );
	if( decl == NULL )
	{
		return;
	}

	int numStyles = decl->dict.GetInt( "num_styles", "0" );
	if( numStyles > 0 )
	{
		for( int i = 0; i < numStyles; i++ )
		{
			idStr style = decl->dict.GetString( va( "light_style%d", i ) );
			styleNames.Append( style );
		}
	}
	else
	{
		// RB: it's not defined in entityDef light so use predefined Quake 1 table
		for( int i = 0; i < 12; i++ )
		{
			idStr style( predef_lightstylesinfo[ i ] );
			styleNames.Append( style );
		}
	}
}

// static
const char* LightEditor::StyleItemsGetter( void* data, int idx )
{
	LightEditor* self = static_cast<LightEditor*>( data );
	if( idx == 0 )
	{
		return "<No Lightstyle>";
	}

	// as index 0 has special purpose, the "real" index is one less
	--idx;

	if( idx < 0 || idx >= self->styleNames.Num() )
	{
		return "<Invalid Index!>";
	}

	return self->styleNames[idx].c_str();
}

void LightEditor::TempApplyChanges()
{
	if( lightEntity != NULL )
	{
		idDict d;
		cur.ToDict( &d );

		gameEdit->EntityChangeSpawnArgs( lightEntity, &d );
		gameEdit->EntityUpdateChangeableSpawnArgs( lightEntity, NULL );
	}
}

void LightEditor::SaveChanges( bool saveMap )
{
	idDict d;
	cur.ToDict( &d );
	if( entityName[0] != '\0' )
	{
		gameEdit->MapCopyDictToEntity( entityName, &d );
	}
	else if( entityPos.x != idMath::INFINITUM )
	{
		entityName = gameEdit->GetUniqueEntityName( "light" );
		d.Set( "name", entityName );
		gameEdit->MapCopyDictToEntityAtOrigin( entityPos, &d );
	}

	original = cur;

	if( saveMap )
	{
		gameEdit->MapSave();
	}
}

void LightEditor::CancelChanges()
{
	if( lightEntity != NULL )
	{
		idDict d;
		original.ToDict( &d );

		gameEdit->EntityChangeSpawnArgs( lightEntity, &d );
		gameEdit->EntityUpdateChangeableSpawnArgs( lightEntity, NULL );
	}
}

void LightEditor::DuplicateLight()
{
	if( lightEntity != NULL )
	{
		// store current light properties to game idMapFile
		SaveChanges( false );

		// spawn the new light
		idDict d;
		cur.ToDict( &d );
		d.DeleteEmptyKeys();

		entityName = gameEdit->GetUniqueEntityName( "light" );
		d.Set( "name", entityName );
		d.Set( "classname", "light" );

		idEntity* light = NULL;
		gameEdit->SpawnEntityDef( d, &light );

		if( light )
		{
			gameEdit->MapAddEntity( &d );
			gameEdit->ClearEntitySelection();
			gameEdit->AddSelectedEntity( light );

			Init( &d, light );
		}
	}
}

// a kinda ugly hack to get a float* (as used by imgui) from idVec3
static float* vecToArr( idVec3& v )
{
	return &v.x;
}

void LightEditor::DrawContents( bool& showTool )
{
	ImGuiIO& io = ImGui::GetIO();
	bool changes = false;

	// RB: handle arrow key inputs like in TrenchBroom
	if( ImGui::IsKeyDown( ImGuiKey_Escape ) )
	{
		CancelChanges();
		showTool = false;
	}

	// TODO use view direction like just global values
	if( io.KeyCtrl )
	{
		if( ImGui::IsKeyDown( ImGuiKey_S ) && shortcutSaveMapEnabled )
		{
			SaveChanges( true );
			shortcutSaveMapEnabled = false;
		}
		else if( ImGui::IsKeyDown( ImGuiKey_D ) && shortcutDuplicateLightEnabled )
		{
			DuplicateLight();
			shortcutDuplicateLightEnabled = false;
		}
	}
	else if( io.KeyAlt )
	{
		if( ImGui::IsKeyDown( ImGuiKey_R ) )
		{
			// reset rotation like in Blender
			cur.angles.Zero();
			changes = true;
		}
		else if( ImGui::IsKeyDown( ImGuiKey_UpArrow ) )
		{
			cur.origin.z += 1;
			changes = true;
		}
		else if( ImGui::IsKeyDown( ImGuiKey_DownArrow ) )
		{
			cur.origin.z -= 1;
			changes = true;
		}
	}
	else if( ImGui::IsKeyDown( ImGuiKey_RightArrow ) )
	{
		cur.origin.x += 1;
		changes = true;
	}
	else if( ImGui::IsKeyDown( ImGuiKey_LeftArrow ) )
	{
		cur.origin.x -= 1;
		changes = true;
	}
	else if( ImGui::IsKeyDown( ImGuiKey_UpArrow ) )
	{
		cur.origin.y += 1;
		changes = true;
	}
	else if( ImGui::IsKeyDown( ImGuiKey_DownArrow ) )
	{
		cur.origin.y -= 1;
		changes = true;
	}

	// reenable commands if keys were released
	if( ( !io.KeyCtrl || !ImGui::IsKeyDown( ImGuiKey_S ) ) && !shortcutSaveMapEnabled )
	{
		shortcutSaveMapEnabled = true;
	}

	if( ( !io.KeyCtrl || !ImGui::IsKeyDown( ImGuiKey_D ) ) && !shortcutDuplicateLightEnabled )
	{
		shortcutDuplicateLightEnabled = true;
	}

	if( !entityName.IsEmpty() )
	{
		ImGui::SeparatorText( entityName.c_str() );
	}

	ImGui::SeparatorText( "Light Volume" );

	ImGui::Spacing();

	int lightSelectionRadioBtn = cur.lightType;

	changes |= ImGui::RadioButton( "Point Light", &lightSelectionRadioBtn, 0 );
	ImGui::SameLine();
	changes |= ImGui::RadioButton( "Spot Light", &lightSelectionRadioBtn, 1 );
	ImGui::SameLine();
	changes |= ImGui::RadioButton( "Sun Light", &lightSelectionRadioBtn, 2 );

	ImGui::Indent();

	ImGui::Spacing();

	if( lightSelectionRadioBtn == LIGHT_POINT || lightSelectionRadioBtn == LIGHT_SUN )
	{
		if( lightSelectionRadioBtn == LIGHT_POINT && lightSelectionRadioBtn != cur.lightType )
		{
			cur.DefaultPoint();
			changes = true;
		}
		else if( lightSelectionRadioBtn == LIGHT_SUN && lightSelectionRadioBtn != cur.lightType )
		{
			cur.DefaultSun();
			changes = true;
		}

		ImGui::PushItemWidth( -1.0f ); // align end of Drag* with right window border

		changes |= ImGui::Checkbox( "Equilateral Radius", &cur.equalRadius );
		ImGui::Text( "Radius:" );
		ImGui::Indent();
		if( cur.equalRadius )
		{
			if( ImGui::DragFloat( "##radEquil", &cur.lightRadius.x, 1.0f, 0.0f, 10000.0f, "%.1f" ) )
			{
				cur.lightRadius.z = cur.lightRadius.y = cur.lightRadius.x;
				changes = true;
			}
		}
		else
		{
			changes |= ImGui::DragVec3( "##radXYZ", cur.lightRadius );
		}
		ImGui::Unindent();

		ImGui::Spacing();

		//changes |= ImGui::Checkbox( "Parallel", &cur.isParallel );

		//ImGui::Spacing();

		changes |= ImGui::Checkbox( "Center", &cur.hasCenter );
		if( cur.hasCenter )
		{
			ImGui::Indent();
			changes |= ImGui::DragVec3( "##centerXYZ", cur.lightCenter, 1.0f, 0.0f, 10000.0f, "%.1f" );
			ImGui::Unindent();
		}
		ImGui::PopItemWidth(); // back to default alignment on right side
	}
	else if( lightSelectionRadioBtn == LIGHT_SPOT )
	{
		if( cur.lightType != lightSelectionRadioBtn )
		{
			cur.DefaultProjected();
			changes = true;
		}

		changes |= ImGui::DragVec3( "Target", cur.lightTarget, 1.0f, 0.0f, 0.0f, "%.1f" );
		changes |= ImGui::DragVec3( "Right", cur.lightRight, 1.0f, 0.0f, 0.0f, "%.1f" );
		changes |= ImGui::DragVec3( "Up", cur.lightUp, 1.0f, 0.0f, 0.0f, "%.1f" );

		ImGui::Spacing();

		changes |= ImGui::Checkbox( "Explicit start/end points", &cur.explicitStartEnd );

		ImGui::Spacing();
		if( cur.explicitStartEnd )
		{
			changes |= ImGui::DragVec3( "Start", cur.lightStart, 1.0f, 0.0f, 0.0f, "%.1f" );
			changes |= ImGui::DragVec3( "End", cur.lightEnd, 1.0f, 0.0f, 0.0f, "%.1f" );
		}
	}

	cur.lightType = ELightType( lightSelectionRadioBtn );

	ImGui::Unindent();

	ImGui::SeparatorText( "Transform" );

	if( ImGui::IsKeyDown( ImGuiKey_G ) )
	{
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	}

	if( ImGui::IsKeyDown( ImGuiKey_R ) )
	{
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	}

	//if( ImGui::IsKeyPressed( ImGuiKey_S ) )
	if( ImGui::IsKeyDown( ImGuiKey_S ) )
	{
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	}

	if( mCurrentGizmoOperation != ImGuizmo::SCALE )
	{
		if( ImGui::RadioButton( "Local", mCurrentGizmoMode == ImGuizmo::LOCAL ) )
		{
			mCurrentGizmoMode = ImGuizmo::LOCAL;
		}
		ImGui::SameLine();
		if( ImGui::RadioButton( "World", mCurrentGizmoMode == ImGuizmo::WORLD ) )
		{
			mCurrentGizmoMode = ImGuizmo::WORLD;
		}
	}
	else
	{
		mCurrentGizmoMode = ImGuizmo::LOCAL;
	}

	if( ImGui::RadioButton( "Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE ) )
	{
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	}
	ImGui::SameLine();
	if( ImGui::RadioButton( "Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE ) )
	{
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	}
	ImGui::SameLine();
	if( ImGui::RadioButton( "Scale", mCurrentGizmoOperation == ImGuizmo::SCALE ) )
	{
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	}
	//if( ImGui::RadioButton( "Universal", mCurrentGizmoOperation == ImGuizmo::UNIVERSAL ) )
	//{
	//	mCurrentGizmoOperation = ImGuizmo::UNIVERSAL;
	//}

	changes |= ImGui::DragVec3( "Origin", cur.origin, 1.0f, 0.0f, 0.0f, "%.1f" );
	changes |= ImGui::InputFloat3( "Angles", cur.angles.ToFloatPtr() );
	//changes |= ImGui::DragVec3( "Angles", cur.origin, 1.0f, 0.0f, 0.0f, "%.1f" );

	ImGui::SeparatorText( "Snapping" );

	ImGui::Checkbox( "Use Snapping", &useSnap );
	//ImGui::SameLine();

	if( useSnap )
	{
		switch( mCurrentGizmoOperation )
		{
			case ImGuizmo::TRANSLATE:
				ImGui::InputFloat3( "Grid Snap", &gridSnap[0] );
				break;
			case ImGuizmo::ROTATE:
				ImGui::InputFloat( "Angle Snap", &angleSnap );
				break;
			case ImGuizmo::SCALE:
				ImGui::InputFloat( "Scale Snap", &scaleSnap );
				break;
		}
	}

#if 0
	ImGui::Checkbox( "Bound Sizing", &boundSizing );
	if( boundSizing )
	{
		ImGui::PushID( 3 );
		ImGui::Checkbox( "##BoundSizing", &boundSizingSnap );
		ImGui::SameLine();
		ImGui::InputFloat3( "Snap", boundsSnap );
		ImGui::PopID();
	}
#endif

	ImGui::SeparatorText( "Color & Texturing" );

	changes |= ImGui::ColorEdit3( "Color", vecToArr( cur.color ) );

	ImGui::Spacing();

	ImGui::SeparatorText( "Flicker Style" );

	if( ImGui::Combo( "Style", &currentStyleIndex, StyleItemsGetter, this, styleNames.Num() + 1 ) )
	{
		changes = true;

		// -1 because 0 is "<No Lightstyle>"
		cur.lightStyle = ( currentStyleIndex > 0 ) ? currentStyleIndex - 1 : -1;
	}

	ImGui::SeparatorText( "Misc Options" );

	changes |= ImGui::Checkbox( "Cast Shadows", &cur.castShadows );
	changes |= ImGui::Checkbox( "Skip Specular", &cur.skipSpecular );

	// TODO: allow multiple lights selected at the same time + "apply different" button?
	//       then only the changed attribute (e.g. color) would be set to all lights,
	//       but they'd keep their other individual properties (eg radius)

	viewDef_t viewDef = {};
	if( gameEdit->PlayerGetRenderView( viewDef.renderView ) )
	{
		ImGui::Separator();

		ImGui::Text( "X: %f Y: %f", io.MousePos.x, io.MousePos.y );
		if( ImGuizmo::IsUsing() )
		{
			ImGui::Text( "Using gizmo" );
		}
		else
		{
			ImGui::Text( ImGuizmo::IsOver() ? "Over gizmo" : "" );
			ImGui::SameLine();
			ImGui::Text( ImGuizmo::IsOver( ImGuizmo::TRANSLATE ) ? "Over translate gizmo" : "" );
			ImGui::SameLine();
			ImGui::Text( ImGuizmo::IsOver( ImGuizmo::ROTATE ) ? "Over rotate gizmo" : "" );
			ImGui::SameLine();
			ImGui::Text( ImGuizmo::IsOver( ImGuizmo::SCALE ) ? "Over scale gizmo" : "" );
		}
	}

	static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
									| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs;// | ImGuiWindowFlags_MenuBar;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 scenePos = viewport->WorkPos;
	ImVec2 sceneSize = viewport->WorkSize;
	ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode( ImHashStr( "Kroom3MainDockSpace" ) );
	if( centralNode != NULL )
	{
		scenePos = centralNode->Pos;
		sceneSize = centralNode->Size;
	}
	ImGui::SetNextWindowPos( scenePos );
	ImGui::SetNextWindowSize( sceneSize );

	if( ImGui::Begin( "###LightEditorToolBar", &showTool, flags ) )
	{
		if( ImGui::BeginMainMenuBar() )
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				//ShowExampleMenuFile();
				if( ImGui::MenuItem( "Save Map", "Ctrl+S" ) )
				{
					SaveChanges( true );
				}

				ImGui::Separator();

				if( ImGui::MenuItem( "Close" ) )
				{
					CancelChanges();
					showTool = false;
				}

				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu( "Edit" ) )
			{
				//if( ImGui::MenuItem( "Undo", "CTRL+Z" ) ) {}
				//if( ImGui::MenuItem( "Redo", "CTRL+Y", false, false ) ) {} // Disabled item

				//ImGui::Separator();

				//if( ImGui::MenuItem( "Cut", "CTRL+X" ) ) {}
				//if( ImGui::MenuItem( "Copy", "CTRL+C" ) ) {}
				//if( ImGui::MenuItem( "Paste", "CTRL+V" ) ) {}

				if( ImGui::MenuItem( "Duplicate", "CTRL+D" ) )
				{
					DuplicateLight();
				}

				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		// backup state before moving the light
		if( !ImGuizmo::IsUsing() )
		{
			curNotMoving = cur;
		}

		//
		// GIZMO
		//
		ImGuizmo::SetRect( 0, 0, io.DisplaySize.x, io.DisplaySize.y );
		ImGuizmo::SetOrthographic( false );
		ImGuizmo::SetDrawlist();

		ImGuizmo::SetID( 0 );


		//viewDef_t viewDef;
		//if( gameEdit->PlayerGetRenderView( viewDef.renderView ) )
		{
			R_SetupViewMatrix( &viewDef );
			R_SetupProjectionMatrix( &viewDef );

			float* cameraView = viewDef.worldSpace.modelViewMatrix;
			float* cameraProjection = viewDef.projectionMatrix;

			idMat3 rotateMatrix = cur.angles.ToMat3();
			idMat3 scaleMatrix = mat3_identity;
			scaleMatrix[0][0] = 16;
			scaleMatrix[1][1] = 16;
			scaleMatrix[2][2] = 16;

			idMat4 objectMatrix( scaleMatrix * rotateMatrix,  cur.origin );
			ImGuizmo::DrawCubes( cameraView, cameraProjection, objectMatrix.Transpose().ToFloatPtr(), 1 );

			scaleMatrix[0][0] = 1;
			scaleMatrix[1][1] = 1;
			scaleMatrix[2][2] = 1;

			idMat4 gizmoMatrix( scaleMatrix * rotateMatrix,  cur.origin );
			idMat4 manipMatrix = gizmoMatrix.Transpose();

			const float* snap = NULL;
			if( useSnap )
			{
				switch( mCurrentGizmoOperation )
				{
					case ImGuizmo::TRANSLATE:
						snap = &gridSnap[0];
						break;
					case ImGuizmo::ROTATE:
						snap = &angleSnap;
						break;
					case ImGuizmo::SCALE:
						snap = &scaleSnap;
						break;
				}
			}

			ImGuizmo::Manipulate( cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, manipMatrix.ToFloatPtr(), NULL, useSnap ? snap : NULL, boundSizing ? bounds : NULL, boundSizingSnap ? boundsSnap : NULL );

			if( ImGuizmo::IsUsing() )
			{
				//if( mCurrentGizmoOperation == ImGuizmo::TRANSLATE )
				{
					gizmoMatrix = manipMatrix.Transpose();
					cur.origin = gizmoMatrix.GetTranslation();

					changes = true;
				}

				if( ( mCurrentGizmoOperation & ImGuizmo::SCALE ) == 0 )
				{
					idMat3 axis = gizmoMatrix.ToMat3();
					cur.angles = axis.ToAngles();

					changes = true;
				}

				if( mCurrentGizmoOperation == ImGuizmo::SCALE )
				{
					// Use DecomposeMatrixToComponents just for the scaling
					float matrixTranslation[3], matrixRotation[3], matrixScale[3];
					ImGuizmo::DecomposeMatrixToComponents( &manipMatrix[0][0], matrixTranslation, matrixRotation, matrixScale );

					cur.scale.x = matrixScale[0];
					cur.scale.y = matrixScale[1];
					cur.scale.z = matrixScale[2];

					if( matrixScale[0] != 1.0f || matrixScale[1] != 1.0f || matrixScale[2] != 1.0f )
					{
						if( cur.lightType == LIGHT_SPOT )
						{
							cur.lightRight = curNotMoving.lightRight * matrixScale[0];
							cur.lightUp = curNotMoving.lightUp * matrixScale[1];
							cur.lightTarget = curNotMoving.lightTarget * matrixScale[2];
						}
						else //if( cur.lightType == LIGHT_POINT )
						{
							cur.lightRadius.x = curNotMoving.lightRadius.x * matrixScale[0];
							cur.lightRadius.y = curNotMoving.lightRadius.y * matrixScale[1];
							cur.lightRadius.z = curNotMoving.lightRadius.z * matrixScale[2];

							if( matrixScale[0] != matrixScale[1] || matrixScale[1] != matrixScale[2] )
							{
								cur.equalRadius = false;
							}
						}

						changes = true;
					}
				}
			}
		}
	}
	ImGui::End();

	if( showTool )
	{
		changes |= DrawLightTextureBrowser();
	}

	if( changes )
	{
		TempApplyChanges();
	}
}

void LightEditor::OnClosed()
{
	gameEdit->PlayerEnableFreeCam( false );
	imguiSystem->GetEditor()->ReleaseMouse( false );
}

void LightEditorInit( const idDict* spawnArgs, idEntity* ent )
{
	if( spawnArgs == NULL || ent == NULL )
	{
		return;
	}

	idassert( idStr::Icmp( spawnArgs->GetString( "spawnclass" ), "idLight" ) == 0
			  && "LightEditorInit() must only be called with light entities or NULL!" );

	LightEditor::Instance().ShowIt( true );
	imguiSystem->GetEditor()->RegisterWindow( LightEditor::Instance() );
	imguiSystem->GetEditor()->ReleaseMouse( true );
	gameEdit->PlayerEnableFreeCam( true );
	imguiSystem->RegisterDockWindow( "Light Texture Browser", DOCK_REGION_BOTTOM );

	LightEditor::ReInit( spawnArgs, ent );
}