/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson

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

#include "ImGui_IdWidgets.h"

static const char* bodyContentsNames[5] =
{
	"solid",
	"body",
	"corpse",
	"playerclip",
	"monsterclip"
};

static int contentMappingFlags[5] =
{
	CONTENTS_SOLID,
	CONTENTS_BODY,
	CONTENTS_CORPSE,
	CONTENTS_PLAYERCLIP,
	CONTENTS_MONSTERCLIP
};

MultiSelectWidget::MultiSelectWidget( const char** aNames, int* contentMapping, int aNumEntries )
	: names( aNames )
	, contentMapping( contentMapping )
	, numEntries( aNumEntries )
	, selectables( nullptr )
{
	selectables = ( bool* )Mem_Alloc( numEntries * sizeof( bool ), TAG_CRAP );
	memset( selectables, 0, numEntries * sizeof( bool ) );
}

MultiSelectWidget::~MultiSelectWidget()
{
	Mem_Free( selectables );
}

void MultiSelectWidget::Update( int index, bool value )
{
	assert( index < numEntries );
	selectables[index] = value;
}

void MultiSelectWidget::UpdateWithBitFlags( int bitFlags )
{
	for( int i = 0; i < numEntries; i++ )
	{
		Update( i, bitFlags & contentMapping[i] );
	}
}

bool DoMultiSelect( MultiSelectWidget* widget, int* contents )
{
	bool pressed = false;
	for( int i = 0; i < 5; i++ )
	{
		if( ImGui::Selectable( widget->names[i], &widget->selectables[i] ) )
		{
			pressed = true;
			if( widget->selectables[i] )
			{
				*contents |= widget->contentMapping[i];
			}
			else
			{
				*contents &= ~widget->contentMapping[i];
			}
		}
	}

	return pressed;
}

void HelpMarker( const char* desc )
{
	ImGui::TextDisabled( "(?)" );
	if( ImGui::IsItemHovered() )
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos( ImGui::GetFontSize() * 35.0f );
		ImGui::TextUnformatted( desc );
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

const char* StringListItemGetter( void* data, int index )
{
	idStrList* list = reinterpret_cast<idStrList*>( data );
	assert( index < list->Num() );

	return ( *list )[index];
}

MultiSelectWidget MakePhysicsContentsSelector()
{
	return MultiSelectWidget( bodyContentsNames, contentMappingFlags, 5 );
}

ColorPicker::ColorPicker( const char* _label )
{
	label = _label;
	color.Set( 0, 0, 0, 1.0f );
}

bool ColorPicker::Button( const idVec4& _color )
{
	ImVec4 col = ImVec4( _color.x, _color.y, _color.z, _color.w );

	if( ImGui::ColorButton( label, col ) )
	{
		oldColor = _color;
		ImGui::OpenPopup( label );
		return true;
	}

	return false;
}

bool ColorPicker::Draw()
{
	idStr realLabel;
	bool isAccepted = false;

	if( ImGui::BeginPopupModal( label, nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
	{
		realLabel = label;
		realLabel += "Picker";

		bool changed = ImGui::ColorPicker4( realLabel.c_str(), color.ToFloatPtr(), ImGuiColorEditFlags_AlphaBar, oldColor.ToFloatPtr() );

		if( ImGui::Button( "OK" ) )
		{
			isAccepted = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if( ImGui::Button( "Cancel" ) )
		{
			isAccepted = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	return isAccepted;
}

DeclNewSelect::DeclNewSelect( declType_t _declType, const char* _directory, const char* _extension, const char* _label )
	: declType( _declType )
	, directory( _directory )
	, extension( _extension )
	, label( _label )
	, fileSelection( -1 )
	, files()
	, fileName( "" )
	, name( "" )
	, errorText( "" )
	, dp( NULL )
	, state( DONE )
{
}

void DeclNewSelect::Start()
{
	files.Clear();

	idFileList* names = fileSystem->ListFiles( directory, extension, true, true );
	for( int i = 0; i < names->GetNumFiles(); i++ )
	{
		idStr file = names->GetFile( i );

		file.StripPath();
		file.StripFileExtension();

		files.Append( file );
	}
	fileSystem->FreeFileList( names );

	fileSelection = -1;
	fileName.Clear();
	name.Clear();
	errorText.Clear();
	dp = NULL;
	state = NAME;

	ImGui::OpenPopup( label );
}

bool DeclNewSelect::Draw()
{
	if( state == DONE )
	{
		return false;
	}

	bool accepted = false;
	bool canceled = false;

	if( ImGui::BeginPopupModal( label, nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
	{
		ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "%s", errorText.c_str() );

		if( ImGui::InputTextStr( "File Name", &fileName ) )
		{
			// nop
		}

		if( ImGui::BeginListBox( "Files##prtFileSelect" ) )
		{
			for( int i = 0; i < files.Num(); i++ )
			{
				if( fileName.Length() && files[i].Find( fileName.c_str(), false ) == -1 )
				{
					continue;
				}

				bool selected = ( i == fileSelection );

				ImGui::PushID( i );
				if( ImGui::Selectable( files[i].c_str(), selected ) )
				{
					fileSelection = i;
					fileName = files[fileSelection];
				}
				if( selected )
				{
					ImGui::SetItemDefaultFocus();
				}
				ImGui::PopID();
			}

			ImGui::EndListBox();
		}

		if( ImGui::InputTextStr( "Name", &name ) )
		{
			// nop
		}

		if( ImGui::Button( "OK" ) )
		{
			errorText.Clear();

			if( name.IsEmpty() )
			{
				errorText += "Please enter a name\n";
				accepted = false;
			}

			idDecl* newDecl = const_cast<idDecl*>( declManager->FindType( declType, name.c_str(), false ) );
			if( newDecl )
			{
				errorText += va( "Decl %s already exists in %s. Please select a different name\n", name.c_str(), newDecl->GetFileName() );
				accepted = false;
			}

			if( errorText.IsEmpty() )
			{
				idStr fullName;

				fullName = directory;
				fullName += fileName;
				fullName += extension;

				// create it
				dp = declManager->CreateNewDecl( declType, name.c_str(), fullName.c_str() );
				state = DONE;

				accepted = true;
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if( ImGui::Button( "Cancel" ) )
		{
			accepted = false;
			state = DONE;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	return accepted;
}

DeclSelect::DeclSelect( declType_t _declType, const char* _label )
	: declType( _declType )
	, label( _label )
	, listSel( -1 )
	, list()
	, name( "" )
	, errorText( "" )
	, dp( NULL )
	, state( DONE )
{
}

void DeclSelect::Start( const char* _name )
{
	list.Clear();
	for( int i = 0; i < declManager->GetNumDecls( declType ); i++ )
	{
		const idDecl* idp = declManager->DeclByIndex( declType, i, false );
		list.Append( idp->GetName() );
	}
	if( _name )
	{
		name = _name;
		listSel = list.FindIndex( name );
	}
	else
	{
		name.Clear();
		listSel = -1;
	}

	errorText.Clear();
	dp = NULL;
	state = NAME;

	ImGui::OpenPopup( label );
}

bool DeclSelect::Draw()
{
	if( state == DONE )
	{
		return false;
	}

	bool accepted = false;
	bool canceled = false;

	if( ImGui::BeginPopupModal( label, nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
	{
		ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "%s", errorText.c_str() );

		if( ImGui::InputTextStr( "Name", &name ) )
		{
			// nop
		}

		if( ImGui::BeginListBox( "Decls##prtSystemSelect" ) )
		{
			for( int i = 0; i < list.Num(); i++ )
			{
				if( name.Length() && list[i].Find( name.c_str(), false ) == -1 )
				{
					continue;
				}

				bool selected = ( i == listSel );

				ImGui::PushID( i );
				if( ImGui::Selectable( list[i].c_str(), selected ) )
				{
					listSel = i;
					name = list[listSel];
				}
				if( selected )
				{
					ImGui::SetItemDefaultFocus();
				}
				ImGui::PopID();
			}

			ImGui::EndListBox();
		}

		if( ImGui::Button( "OK" ) )
		{
			errorText.Clear();

			if( name.IsEmpty() )
			{
				errorText += "Please enter a name or select a decl from the list\n";
				accepted = false;
			}

			idDecl* decl = const_cast<idDecl*>( declManager->FindType( declType, name.c_str(), false ) );
			if( !decl )
			{
				errorText += va( "Decl %s does not exist. Please select a different decl\n", name.c_str() );
				accepted = false;
			}

			if( errorText.IsEmpty() )
			{
				dp = decl;
				state = DONE;

				accepted = true;
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if( ImGui::Button( "Cancel" ) )
		{
			accepted = false;
			state = DONE;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	return accepted;
}

GoToLineDialog::GoToLineDialog()
	: numberEdit( 0 )
	, firstLine( 0 )
	, lastLine( 0 )
	, waiting( false )
	, valid( false )
	, focus( false )
	, caption()
{
}

void GoToLineDialog::Start( int _firstLine, int _lastLine, int _line )
{
	firstLine = _firstLine;
	lastLine = _lastLine;
	numberEdit = _line;
	valid = ( idMath::ClampInt( firstLine, lastLine, numberEdit ) == numberEdit );
	waiting = true;
	focus = true;
	caption = va( "Line number (%d - %d)", firstLine, lastLine );
}

bool GoToLineDialog::Draw( const ImVec2& pos, const ImVec2& size )
{
	bool accepted = false;

	if( !waiting )
	{
		return accepted;
	}

	ImGuiStyle& style = ImGui::GetStyle();
	float fieldWidth = 250.0f;

	float captionWidth = ImGui::CalcTextSize( caption.c_str() ).x;

	float windowHeight =
		style.ChildBorderSize * 2.0f +
		style.WindowPadding.y * 2.0f +
		ImGui::GetFrameHeight() * 2.0f +
		style.ItemSpacing.y;

	float windowWidth =
		style.ChildBorderSize * 2.0f +
		style.WindowPadding.x * 2.0f +
		fieldWidth + style.ItemSpacing.x +
		captionWidth;

	ImVec2 oldCursorPos = ImGui::GetCursorPos();

	// TODO: this seems off, the dialog should be centered
	ImGui::SetCursorPos( ImVec2(
							 pos.x + ( size.x - windowWidth ) * 0.5f,
							 pos.y + ( size.y - windowHeight ) * 0.5f ) );

	if( ImGui::BeginChild( "Go To Line", ImVec2( windowWidth, windowHeight ), ImGuiChildFlags_Borders ) )
	{
		ImGui::SetNextItemWidth( fieldWidth );
		if( ImGui::InputInt( caption.c_str(), &numberEdit, 0, 0 ) )
		{
			valid = ( idMath::ClampInt( firstLine, lastLine, numberEdit ) == numberEdit );
		}
		if( focus )
		{
			ImGui::SetKeyboardFocusHere( -1 );
			focus = false;
		}

		ImGui::BeginDisabled( !valid );
		if( ImGui::Button( "OK" ) )
		{
			waiting = false;
			accepted = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if( ImGui::Button( "Cancel" ) )
		{
			waiting = false;
			accepted = false;
		}
	}
	ImGui::EndChild();
	ImGui::SetCursorPos( oldCursorPos );

	return accepted;
}

FindReplaceDialog::FindReplaceDialog()
	: replace()
	, find()
	, matchCase( false )
	, matchWhole( false )
	, replacement( false )
	, valid( false )
	, visible( false )
	, focus( false )
{
}

void FindReplaceDialog::Start( idStr& selection, bool _replacement )
{
	if( selection.Length() )
	{
		find = selection;
	}
	replace.Clear();
	replacement = _replacement;
	valid = ( find.Length() > 0 );
	visible = true;
	focus = true;
}

FindReplaceDialog::command_t FindReplaceDialog::Draw( const ImVec2& pos, const ImVec2& size )
{
	command_t command = command_t::NONE;

	if( !visible )
	{
		return command;
	}

	ImGuiStyle& style = ImGui::GetStyle();
	float fieldWidth = 250.0f;

	float replaceWidth = ImGui::CalcTextSize( " Next " ).x + style.FramePadding.x * 2.0f;
	float replaceAllWidth = ImGui::CalcTextSize( " All " ).x + style.FramePadding.x * 2.0f;
	float optionWidth = ImGui::CalcTextSize( "Aa" ).x + style.FramePadding.x * 2.0f;

	float windowHeight =
		style.ChildBorderSize * 3.0f +
		style.WindowPadding.y * 3.0f +
		ImGui::GetFrameHeight() * 3.0f +
		style.ItemSpacing.y;

	float windowWidth =
		style.ChildBorderSize * 2.0f +
		style.WindowPadding.x * 2.0f +
		fieldWidth + style.ItemSpacing.x +
		replaceWidth + style.ItemSpacing.x +
		replaceAllWidth + style.ItemSpacing.x;

	ImVec2 oldCursorPos = ImGui::GetCursorPos();

	ImGui::SetCursorPos( ImVec2(
							 pos.x + size.x - windowWidth - style.ScrollbarSize - style.ItemSpacing.x,
							 pos.y + style.ItemSpacing.y * 2.0f ) );

	if( ImGui::BeginChild( "Find/Replace", ImVec2( windowWidth, windowHeight ), ImGuiChildFlags_Borders ) )
	{

		ImGui::SetNextItemWidth( fieldWidth );

		if( ImGui::InputTextStr( "###Find", &find ) )
		{
			valid = ( find.Length() > 0 );
		}
		if( focus )
		{
			ImGui::SetKeyboardFocusHere( -1 );
			focus = false;
		}
		ImGui::SetItemTooltip( "Search term" );
		ImGui::SameLine();

		ImGui::BeginDisabled( !valid );
		if( ImGui::ArrowButton( "Next", ImGuiDir_Down ) )
		{
			command = command_t::FIND_NEXT;
		}
		ImGui::SetItemTooltip( "Find next occurrence" );
		ImGui::SameLine();
		if( ImGui::ArrowButton( "Prev", ImGuiDir_Up ) )
		{
			command = command_t::FIND_PREV;
		}
		ImGui::SetItemTooltip( "Find previous occurrence" );
		ImGui::EndDisabled();

		ImGui::SameLine();

		if( ImGui::ToggleButton( "R", &replacement, ImVec2( optionWidth, 0.0f ) ) )
		{

		}
		ImGui::SetItemTooltip( "Toggle to switch between find and replace modes" );

		ImGui::SameLine();

		if( ImGui::Button( "x", ImVec2( optionWidth, 0.0f ) ) )
		{
			visible = false;
			command = DONE;
		}

		ImGui::SetNextItemWidth( fieldWidth );
		ImGui::BeginDisabled( !replacement );
		if( ImGui::InputTextStr( "###Replace with", &replace ) )
		{
		}
		ImGui::SetItemTooltip( "Replacement term" );
		ImGui::SameLine();
		if( ImGui::Button( "Next###ReplaceNext" ) )
		{
			command = command_t::REPLACE_NEXT;
		}
		ImGui::SetItemTooltip( "Replace Next" );
		ImGui::SameLine();
		if( ImGui::Button( "All" ) )
		{
			command = command_t::REPLACE_ALL;
		}
		ImGui::SetItemTooltip( "Replace All" );
		ImGui::EndDisabled();

		if( ImGui::ToggleButton( "Aa", &matchCase, ImVec2( optionWidth, 0.0f ) ) )
		{
		}
		ImGui::SetItemTooltip( "Match case" );

		ImGui::SameLine();

		if( ImGui::ToggleButton( "[]", &matchWhole, ImVec2( optionWidth, 0.0f ) ) )
		{
		}
		ImGui::SetItemTooltip( "Match whole word" );
	}
	ImGui::EndChild();

	ImGui::SetCursorPos( oldCursorPos );

	return command;
}

MessageBoxDialog::MessageBoxDialog()
	: message()
	, choice( false )
	, error( false )
	, visible( false )
	, acked( false )
	, focus( false )
{
}

void MessageBoxDialog::Start( const char* _message, bool _choice, bool _error )
{
	message = _message;
	choice = _choice;
	error = _error;
	visible = true;
	acked = false;
	focus = true;
}

bool MessageBoxDialog::Draw( const ImVec2& pos, const ImVec2& size )
{
	if( !visible )
	{
		return false;
	}

	ImGuiStyle& style = ImGui::GetStyle();

	ImVec2 textSize = ImGui::CalcTextSize( message.c_str() );

	float windowHeight =
		style.ChildBorderSize * 2.0f +
		style.WindowPadding.y * 2.0f +
		ImGui::GetFrameHeight() * 2.0f +
		textSize.y;

	float windowWidth =
		style.ChildBorderSize * 2.0f +
		style.WindowPadding.x * 2.0f +
		textSize.x;

	ImVec2 oldCursorPos = ImGui::GetCursorPos();

	bool interacted = false;

	ImGui::SetCursorPos( ImVec2(
							 pos.x + size.x * 0.5f - windowWidth * 0.5f,
							 pos.y + size.y * 0.5f - windowHeight * 0.5f ) );

	if( ImGui::BeginChild( "Message", ImVec2( windowWidth, windowHeight ), ImGuiChildFlags_Borders ) )
	{
		if( error )
		{
			ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "%s", message.c_str() );
		}
		else
		{
			ImGui::TextUnformatted( message.c_str() );
		}

		if( focus )
		{
			ImGui::SetKeyboardFocusHere( -1 );
			focus = false;
		}

		if( choice )
		{
			if( ImGui::Button( "Yes" ) )
			{
				acked = true;
				interacted = true;
				visible = false;
			}
			ImGui::SameLine();
			if( ImGui::Button( "No" ) )
			{
				acked = false;
				interacted = true;
				visible = false;
			}
		}
		else
		{
			if( ImGui::Button( "OK" ) )
			{
				visible = false;
				interacted = true;
				acked = true;
			}
		}
	}
	ImGui::EndChild();

	ImGui::SetCursorPos( oldCursorPos );

	return interacted;
}