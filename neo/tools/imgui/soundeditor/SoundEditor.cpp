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

#include "SoundEditor.h"

#include "../../../extern/imgui/imgui_internal.h"
#include "renderer/GLMatrix.h"
#include "renderer/RenderCommon.h"

SoundInfo::SoundInfo()
{
	name = "";
	shader = "";
	group = "";
	origin.Zero();
	angles.Zero();
	volume = 0.0f;
	minDistance = 1.0f;
	maxDistance = 10.0f;
	leadThrough = 0.1f;
	random = 0.0f;
	wait = 0.0f;
	shakes = 0.0f;
	omni = false;
	occlusion = false;
	plain = false;
	looping = true;
	unclamped = false;
	waitForTrigger = -1;
}

void SoundInfo::FromDict( const idDict* dict )
{
	if( dict == NULL )
	{
		return;
	}

	name = dict->GetString( "name" );
	shader = dict->GetString( "s_shader" );
	group = dict->GetString( "soundgroup" );
	dict->GetVector( "origin", "0 0 0", origin );
	if( !dict->GetAngles( "angles", "0 0 0", angles ) )
	{
		angles = idAngles( 0.0f, dict->GetFloat( "angle" ), 0.0f );
	}
	volume = dict->GetFloat( "s_volume", "0" );
	minDistance = dict->GetFloat( "s_mindistance", "1" );
	maxDistance = dict->GetFloat( "s_maxdistance", "10" );
	leadThrough = dict->GetFloat( "s_leadthrough", "0.1" );
	random = dict->GetFloat( "random", "0" );
	wait = dict->GetFloat( "wait", "0" );
	shakes = dict->GetFloat( "s_shakes", "0" );
	omni = dict->GetInt( "s_omni", "-1" ) != 0;
	occlusion = dict->GetBool( "s_occlusion", "0" );
	plain = dict->GetBool( "s_plain", "0" );
	looping = dict->GetBool( "s_looping", "1" );
	unclamped = dict->GetBool( "s_unclamped", "0" );
	waitForTrigger = dict->GetInt( "s_waitfortrigger", "-1" );
}

void SoundInfo::ToDict( idDict* dict ) const
{
	if( dict == NULL )
	{
		return;
	}

	dict->SetVector( "origin", origin );
	dict->SetAngles( "angles", angles );
	dict->Set( "angle", "" );
	dict->Set( "s_shader", shader );
	dict->Set( "soundgroup", group );
	dict->SetFloat( "s_volume", volume );
	dict->SetFloat( "s_mindistance", minDistance );
	dict->SetFloat( "s_maxdistance", maxDistance );
	dict->SetFloat( "s_leadthrough", leadThrough );
	dict->SetFloat( "random", random );
	dict->SetFloat( "wait", wait );
	dict->SetFloat( "s_shakes", shakes );
	dict->SetInt( "s_omni", omni ? 1 : 0 );
	dict->SetBool( "s_occlusion", occlusion );
	dict->SetBool( "s_plain", plain );
	dict->SetBool( "s_looping", looping );
	dict->SetBool( "s_unclamped", unclamped );
	dict->SetInt( "s_waitfortrigger", waitForTrigger );
}

SoundEditor& SoundEditor::Instance()
{
	static SoundEditor instance;
	return instance;
}

void SoundEditor::ReInit( const idDict* dict, idEntity* entity )
{
	Instance().Init( dict, entity );
}

SoundEditor::SoundEditor()
{
	Reset();
}

void SoundEditor::Reset()
{
	isShown = false;
	title = "Sound Editor: no speaker selected!";
	entityName = "";
	soundEntity = NULL;
	original = SoundInfo();
	current = SoundInfo();
	currentShaderIndex = 0;
	currentGizmoOperation = ImGuizmo::TRANSLATE;
	currentGizmoMode = ImGuizmo::WORLD;
	useSnap = false;
	gridSnap[0] = gridSnap[1] = gridSnap[2] = 4.0f;
	angleSnap = 15.0f;
}

void SoundEditor::Init( const idDict* dict, idEntity* entity )
{
	Reset();
	LoadShaders();

	if( dict != NULL )
	{
		original.FromDict( dict );
		current = original;
		entityName = original.name;
		idVec3 entityOrigin;
		gameEdit->EntityGetOrigin( entity, entityOrigin );
		current.origin = entityOrigin;
		original.origin = entityOrigin;
		title = va( "Sound Editor: %s###SoundEditor", entityName.c_str() );

		currentShaderIndex = 0;
		for( int i = 0; i < shaderNames.Num(); ++i )
		{
			if( shaderNames[i] == current.shader )
			{
				currentShaderIndex = i;
				break;
			}
		}
	}

	soundEntity = entity;
}

void SoundEditor::LoadShaders()
{
	shaderNames.Clear();
	for( int i = 0; i < declManager->GetNumDecls( DECL_SOUND ); ++i )
	{
		const idSoundShader* sound = declManager->SoundByIndex( i, false );
		if( sound != NULL )
		{
			shaderNames.AddUnique( sound->GetName() );
		}
	}
}

const char* SoundEditor::ShaderItemsGetter( void* data, int index )
{
	SoundEditor* editor = static_cast<SoundEditor*>( data );
	if( index < 0 || index >= editor->shaderNames.Num() )
	{
		return "<Invalid Shader>";
	}
	return editor->shaderNames[index].c_str();
}

void SoundEditor::ApplyChanges()
{
	if( soundEntity == NULL )
	{
		return;
	}

	idDict changes;
	current.ToDict( &changes );
	gameEdit->EntityChangeSpawnArgs( soundEntity, &changes );
	gameEdit->EntitySetOrigin( soundEntity, current.origin );
	gameEdit->EntitySetAxis( soundEntity, current.angles.ToMat3() );
	gameEdit->EntityUpdateChangeableSpawnArgs( soundEntity, NULL );
}

void SoundEditor::SaveChanges()
{
	if( soundEntity != NULL && !entityName.IsEmpty() )
	{
		idDict changes;
		current.ToDict( &changes );
		gameEdit->MapCopyDictToEntity( entityName, &changes );
		original = current;
		gameEdit->MapSave();
	}
}

void SoundEditor::CancelChanges()
{
	current = original;
	ApplyChanges();
}

void SoundEditor::PlayShader( const char* shader ) const
{
	if( shader == NULL || shader[0] == '\0' )
	{
		return;
	}
	idSoundWorld* soundWorld = soundSystem->GetPlayingSoundWorld();
	if( soundWorld != NULL )
	{
		soundWorld->PlayShaderDirectly( shader );
	}
}

void SoundEditor::DrawGizmo( bool& changed )
{
	if( soundEntity == NULL )
	{
		return;
	}

	viewDef_t viewDef = {};
	if( !gameEdit->PlayerGetRenderView( viewDef.renderView ) )
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();

	static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
									| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground
									| ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs;

	ImGui::SetNextWindowPos( ImVec2( 0.0f, 0.0f ) );
	ImGui::SetNextWindowSize( io.DisplaySize );

	if( ImGui::Begin( "###SoundEditorGizmoHost", NULL, flags ) )
	{
		ImGuizmo::SetRect( 0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y );
		ImGuizmo::SetOrthographic( false );
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetID( 1 );
		R_SetupViewMatrix( &viewDef );
		R_SetupProjectionMatrix( &viewDef );

		idMat4 gizmoMatrix( current.angles.ToMat3(), current.origin );
		idMat3 displayScale = mat3_identity;
		displayScale[0][0] = 16.0f;
		displayScale[1][1] = 16.0f;
		displayScale[2][2] = 16.0f;
		idMat4 displayMatrix( displayScale, current.origin );
		ImGuizmo::DrawCubes( viewDef.worldSpace.modelViewMatrix, viewDef.projectionMatrix, displayMatrix.Transpose().ToFloatPtr(), 1 );

		const float* snap = NULL;
		if( useSnap )
		{
			snap = currentGizmoOperation == ImGuizmo::TRANSLATE ? gridSnap : &angleSnap;
		}

		idMat4 manipMatrix = gizmoMatrix.Transpose();
		ImGuizmo::Manipulate( viewDef.worldSpace.modelViewMatrix, viewDef.projectionMatrix, currentGizmoOperation, currentGizmoMode, manipMatrix.ToFloatPtr(), NULL, snap );
		if( ImGuizmo::IsUsing() )
		{
			gizmoMatrix = manipMatrix.Transpose();
			current.origin = gizmoMatrix.GetTranslation();
			current.angles = gizmoMatrix.ToMat3().ToAngles();
			changed = true;
		}
	}
	ImGui::End();
}

void SoundEditor::Draw()
{
	if( !isShown || soundEntity == NULL )
	{
		return;
	}

	bool changed = false;
	bool showTool = true;
	if( ImGui::Begin( title, &showTool, ImGuiWindowFlags_NoCollapse ) )
	{
		if( ImGui::IsKeyPressed( ImGuiKey_Escape ) )
		{
			CancelChanges();
			showTool = false;
		}

		ImGui::SeparatorText( "Speaker" );
		ImGui::Text( "%s", entityName.c_str() );
		if( ImGui::Combo( "Shader", &currentShaderIndex, ShaderItemsGetter, this, shaderNames.Num() ) )
		{
			current.shader = shaderNames[currentShaderIndex];
			changed = true;
		}
		if( ImGui::Button( "Play" ) )
		{
			PlayShader( current.shader );
		}
		ImGui::SameLine();
		if( ImGui::Button( "Trigger" ) )
		{
			gameEdit->TriggerSelected();
		}

		ImGui::SeparatorText( "Sound Properties" );
		changed |= ImGui::DragFloat( "Volume", &current.volume, 0.01f, -100.0f, 100.0f, "%.2f" );
		changed |= ImGui::DragFloat( "Min Distance", &current.minDistance, 0.1f, 0.0f, 100000.0f, "%.1f" );
		changed |= ImGui::DragFloat( "Max Distance", &current.maxDistance, 0.1f, 0.0f, 100000.0f, "%.1f" );
		changed |= ImGui::DragFloat( "Lead Through", &current.leadThrough, 0.01f, 0.0f, 100.0f, "%.2f" );
		changed |= ImGui::DragFloat( "Random", &current.random, 0.01f, 0.0f, 100000.0f, "%.2f" );
		changed |= ImGui::DragFloat( "Wait", &current.wait, 0.01f, 0.0f, 100000.0f, "%.2f" );
		changed |= ImGui::DragFloat( "Shakes", &current.shakes, 0.01f, 0.0f, 100000.0f, "%.2f" );
		bool waitForTrigger = current.waitForTrigger != 0;
		if( ImGui::Checkbox( "Wait For Trigger", &waitForTrigger ) )
		{
			current.waitForTrigger = waitForTrigger ? 1 : 0;
			changed = true;
		}
		changed |= ImGui::Checkbox( "Omni", &current.omni );
		changed |= ImGui::Checkbox( "Occlusion", &current.occlusion );
		changed |= ImGui::Checkbox( "Plain", &current.plain );
		changed |= ImGui::Checkbox( "Looping", &current.looping );
		changed |= ImGui::Checkbox( "Unclamped", &current.unclamped );

		ImGui::SeparatorText( "Transform" );
		changed |= ImGui::DragVec3( "Origin", current.origin, 1.0f, 0.0f, 0.0f, "%.1f" );
		changed |= ImGui::InputFloat3( "Angles", current.angles.ToFloatPtr() );
		if( ImGui::RadioButton( "Translate", currentGizmoOperation == ImGuizmo::TRANSLATE ) )
		{
			currentGizmoOperation = ImGuizmo::TRANSLATE;
		}
		ImGui::SameLine();
		if( ImGui::RadioButton( "Rotate", currentGizmoOperation == ImGuizmo::ROTATE ) )
		{
			currentGizmoOperation = ImGuizmo::ROTATE;
		}
		ImGui::Checkbox( "Use Snapping", &useSnap );
		if( useSnap )
		{
			if( currentGizmoOperation == ImGuizmo::TRANSLATE )
			{
				ImGui::InputFloat3( "Grid Snap", gridSnap );
			}
			else
			{
				ImGui::InputFloat( "Angle Snap", &angleSnap );
			}
		}

		if( ImGui::Button( "Save to .map" ) )
		{
			SaveChanges();
			showTool = false;
		}
		ImGui::SameLine();
		if( ImGui::Button( "Cancel" ) )
		{
			CancelChanges();
			showTool = false;
		}
	}
	ImGui::End();

	DrawGizmo( changed );
	if( changed )
	{
		ApplyChanges();
	}

	if( isShown && !showTool )
	{
		isShown = false;
		gameEdit->PlayerEnableFreeCam( false );
		imguiSystem->GetEditor()->ReleaseMouse( false );
	}
}

void SoundEditorInit( const idDict* spawnArgs, idEntity* ent )
{
	if( spawnArgs == NULL || ent == NULL )
	{
		return;
	}

	idassert( idStr::Icmp( spawnArgs->GetString( "spawnclass" ), "idSound" ) == 0
			  && "SoundEditorInit() must only be called with sound entities or NULL!" );

	SoundEditor::ReInit( spawnArgs, ent );
	SoundEditor::Instance().ShowIt( true );
	imguiSystem->GetEditor()->RegisterWindow( SoundEditor::Instance() );
	imguiSystem->GetEditor()->ReleaseMouse( true );
	imguiSystem->RegisterDockWindow( "###SoundEditor", DOCK_REGION_RIGHT );
	gameEdit->PlayerEnableFreeCam( true );
}