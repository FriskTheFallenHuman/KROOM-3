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

#include "../../extern/ImGuiColorTextEdit/TextEditor.h"

#include "ImGui_IdWidgets.h"
#include "SyntaxRichEditCtrl.h"

/*
================
SyntaxRichEditCtrl::SyntaxRichEditCtrl
================
*/
SyntaxRichEditCtrl::SyntaxRichEditCtrl()
	: scriptEdit( NULL )
	, scriptEditPos( 0.0f, 0.0f )
	, scriptEditSize( 400.0f, 400.0f )
	, firstLine( 0 )
{
	caseSensitive = false;
	allowPathNames = true;
	keyWordAutoCompletion = true;
	GetObjectMembers = NULL;
	GetFunctionParms = NULL;
	GetToolTip = NULL;
}

/*
================
SyntaxRichEditCtrl::~SyntaxRichEditCtrl
================
*/
SyntaxRichEditCtrl::~SyntaxRichEditCtrl()
{
}

/*
================
SyntaxRichEditCtrl::RebuildLanguage
================
*/
void SyntaxRichEditCtrl::RebuildLanguage()
{
	language = *TextEditor::Language::Cpp();
	language.name = "doomscript";
	language.caseSensitive = caseSensitive;
	language.keywords.clear();
	language.identifiers.clear();
	language.declarations.clear();

	const int count = m_keywords.GetCount();
	for( int i = 0; i < count; i++ )
	{
		const SyntaxKeywords::Entry& entry = m_keywords.GetEntry( i );

		if( entry.description.Length() > 0 )
		{
			language.identifiers.insert( entry.name.c_str() );
		}
		else
		{
			language.keywords.insert( entry.name.c_str() );
		}
	}

	if( scriptEdit != NULL )
	{
		scriptEdit->SetLanguage( &language );
	}
}

/*
================
SyntaxRichEditCtrl::Init
================
*/
void SyntaxRichEditCtrl::Init()
{
	scriptEdit = new( TAG_CRAP ) TextEditor();

	RebuildLanguage();

	autoCompleteConfig.callback = [this]( TextEditor::AutoCompleteState & state )
	{
		AutoCompleteCallback( state );
	};
	scriptEdit->SetAutoCompleteConfig( keyWordAutoCompletion ? &autoCompleteConfig : nullptr );

	scriptEdit->SetTextHoverCallback( [this]( TextEditor::PopupData & data )
	{
		TextHoverCallback( data );
	} );

	SetFocus();
}

/*
================
SyntaxRichEditCtrl::Draw
================
*/
void SyntaxRichEditCtrl::Draw()
{
	scriptEditPos = ImGui::GetCursorPos();
	scriptEditSize = ImVec2( 800, 600 );

	scriptEdit->Render( "Text", scriptEditSize );

	if( gotoDlg.Draw( scriptEditPos, scriptEditSize ) )
	{
		TextEditor::DocPos coords( gotoDlg.GetLine() - 1 - firstLine, 0 );

		scriptEdit->SetCursor( coords );
		SetFocus();
	}

	if( msgBoxDlg.Draw( scriptEditPos, scriptEditSize ) )
	{
		SetFocus();
	}
}

/*
================
SyntaxRichEditCtrl::SetKeyWords
================
*/
void SyntaxRichEditCtrl::SetKeyWords( const keyWord_t kws[] )
{
	m_keywords.Clear();

	if( kws != NULL )
	{
		for( int i = 0; kws[i].keyWord; i++ )
		{
			m_keywords.Add( kws[i].keyWord, kws[i].description, kws[i].color );
		}
	}

	RebuildLanguage();
}

/*
================
SyntaxRichEditCtrl::LoadKeyWordsFromFile
================
*/
bool SyntaxRichEditCtrl::LoadKeyWordsFromFile( const char* fileName )
{
	const bool ok = m_keywords.LoadFromFile( fileName );

	RebuildLanguage();

	return ok;
}

/*
================
SyntaxRichEditCtrl::SetObjectMemberCallback
================
*/
void SyntaxRichEditCtrl::SetObjectMemberCallback( objectMemberCallback_t callback )
{
	GetObjectMembers = callback;
}

/*
================
SyntaxRichEditCtrl::SetFunctionParmCallback
================
*/
void SyntaxRichEditCtrl::SetFunctionParmCallback( toolTipCallback_t callback )
{
	GetFunctionParms = callback;
}

/*
================
SyntaxRichEditCtrl::SetToolTipCallback
================
*/
void SyntaxRichEditCtrl::SetToolTipCallback( toolTipCallback_t callback )
{
	GetToolTip = callback;
}

/*
================
SyntaxRichEditCtrl::SetCaseSensitive
================
*/
void SyntaxRichEditCtrl::SetCaseSensitive( bool caseSensitive )
{
	this->caseSensitive = caseSensitive;
}

/*
================
SyntaxRichEditCtrl::AllowPathNames
================
*/
void SyntaxRichEditCtrl::AllowPathNames( bool allow )
{
	allowPathNames = allow;
}

/*
================
SyntaxRichEditCtrl::EnableKeyWordAutoCompletion
================
*/
void SyntaxRichEditCtrl::EnableKeyWordAutoCompletion( bool enable )
{
	keyWordAutoCompletion = enable;

	if( scriptEdit != NULL )
	{
		scriptEdit->SetAutoCompleteConfig( enable ? &autoCompleteConfig : nullptr );
	}
}

/*
================
SyntaxRichEditCtrl::AutoCompleteCallback
================
*/
void SyntaxRichEditCtrl::AutoCompleteCallback( TextEditor::AutoCompleteState& state )
{
	if( state.inComment || state.inString || state.inNumber )
	{
		return;
	}

	idStr line = scriptEdit->GetLineText( state.searchTermStart.line ).c_str();
	bool isMemberAccess = state.searchTermStart.index > 0 && line[state.searchTermStart.index - 1] == '.';

	if( isMemberAccess && GetObjectMembers != NULL )
	{
		return;
	}

	std::vector<idStr> matches;
	m_keywords.CollectByPrefix( state.searchTerm.c_str(), ( int )state.searchTerm.length(), matches );

	for( size_t i = 0; i < matches.size(); i++ )
	{
		state.suggestions.push_back( matches[i].c_str() );
	}
}

/*
================
SyntaxRichEditCtrl::TextHoverCallback
================
*/
void SyntaxRichEditCtrl::TextHoverCallback( TextEditor::PopupData& data )
{
	idStr word = scriptEdit->GetWordAtMousePos( ImGui::GetMousePos() ).c_str();
	if( word.IsEmpty() )
	{
		ImGui::CloseCurrentPopup();
		return;
	}

	idStr text;

	if( GetToolTip != NULL && GetToolTip( word.c_str(), text ) )
	{
		ImGui::TextUnformatted( text.c_str() );
		return;
	}

	const SyntaxKeywords::Entry* entry = m_keywords.Find( word.c_str() );
	if( entry != NULL && entry->description.Length() > 0 )
	{
		ImGui::TextUnformatted( entry->description.c_str() );
		return;
	}

	if( GetFunctionParms != NULL && GetFunctionParms( word.c_str(), text ) )
	{
		ImGui::TextUnformatted( text.c_str() );
		return;
	}

	ImGui::CloseCurrentPopup();
}

/*
================
SyntaxRichEditCtrl::GetCursorPos
================
*/
void SyntaxRichEditCtrl::GetCursorPos( int& line, int& column, int& character ) const
{
	TextEditor::DocPos coords = scriptEdit->GetCursorPosition( 0 );
	line = ( int )coords.line;
	column = ( int )coords.index;
	character = 0;
}

/*
================
SyntaxRichEditCtrl::GetText
================
*/
void SyntaxRichEditCtrl::GetText( idStr& text ) const
{
	text = scriptEdit->GetText().c_str();
}

/*
================
SyntaxRichEditCtrl::SetText
================
*/
void SyntaxRichEditCtrl::SetText( const char* text )
{
	scriptEdit->SetText( std::string( text ) );
}

/*
================
SyntaxRichEditCtrl::SetReadOnly
================
*/
void SyntaxRichEditCtrl::SetReadOnly( bool readOnly )
{
	scriptEdit->SetReadOnlyEnabled( readOnly );
}

/*
================
SyntaxRichEditCtrl::GetReadOnly
================
*/
bool SyntaxRichEditCtrl::GetReadOnly()
{
	return scriptEdit->IsReadOnlyEnabled();
}

/*
================
SyntaxRichEditCtrl::IsEdited
================
*/
bool SyntaxRichEditCtrl::IsEdited() const
{
	return scriptEdit->CanUndo();
}

bool SyntaxRichEditCtrl::CanCopy()
{
	return scriptEdit->AnyCursorHasSelection();
}

void SyntaxRichEditCtrl::Copy()
{
	scriptEdit->Copy();
}

bool SyntaxRichEditCtrl::CanCut()
{
	return !scriptEdit->IsReadOnlyEnabled() && scriptEdit->AnyCursorHasSelection();
}

void SyntaxRichEditCtrl::Cut()
{
	scriptEdit->Cut();
}

bool SyntaxRichEditCtrl::CanPaste()
{
	const char* clipboardText = ImGui::GetClipboardText();

	return !scriptEdit->IsReadOnlyEnabled() && clipboardText && *clipboardText;
}

void SyntaxRichEditCtrl::Paste()
{
	scriptEdit->Paste();
}

bool SyntaxRichEditCtrl::CanUndo()
{
	return scriptEdit->CanUndo();
}

void SyntaxRichEditCtrl::Undo()
{
	scriptEdit->Undo();
}

bool SyntaxRichEditCtrl::CanRedo()
{
	return scriptEdit->CanRedo();
}

void SyntaxRichEditCtrl::Redo()
{
	scriptEdit->Redo();
}

bool SyntaxRichEditCtrl::CanDelete()
{
	// TODO
	return false;
}

void SyntaxRichEditCtrl::Delete()
{
	// TODO
}

/*
================
SyntaxRichEditCtrl::SetFocus
================
*/
void SyntaxRichEditCtrl::SetFocus()
{
	scriptEdit->SetFocus();
}

/*
================
SyntaxRichEditCtrl::OnEditGoToLine
================
*/
void SyntaxRichEditCtrl::OnEditGoToLine()
{
	TextEditor::DocPos coords = scriptEdit->GetCursorPosition( 0 );

	gotoDlg.Start( firstLine + 1, firstLine + ( int )scriptEdit->GetLineCount(), ( int )coords.line + 1 );
}