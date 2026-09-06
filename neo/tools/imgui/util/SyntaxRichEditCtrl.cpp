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

static keyWord_t defaultKeyWords[] =
{
	{ NULL, vec3_origin, "" }
};

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
	keyWords = defaultKeyWords;
	keyWordLengths = NULL;
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
	FreeKeyWordsFromFile();
}

/*
================
SyntaxRichEditCtrl::RebuildLanguage
================
*/
void SyntaxRichEditCtrl::RebuildLanguage()
{
	int i, numKeyWords, hash;

	for( numKeyWords = 0; keyWords[numKeyWords].keyWord; numKeyWords++ )
	{
		assert( numKeyWords < 4096 );
	}

	delete[] keyWordLengths;
	if( numKeyWords > 0 )
	{
		keyWordLengths = new( TAG_CRAP ) int[numKeyWords];
		for( i = 0; i < numKeyWords; i++ )
		{
			keyWordLengths[i] = idStr::Length( keyWords[i].keyWord );
		}
	}
	else
	{
		keyWordLengths = NULL;
	}

	keyWordHash.Clear( 1024, 1024 );
	for( i = 0; i < numKeyWords; i++ )
	{
		hash = caseSensitive ? idStr::Hash( keyWords[i].keyWord, keyWordLengths[i] )
			   : idStr::IHash( keyWords[i].keyWord, keyWordLengths[i] );
		keyWordHash.Add( hash, i );
	}

	language = *TextEditor::Language::Cpp();
	language.name = "doomscript";
	language.caseSensitive = caseSensitive;
	language.keywords.clear();
	language.identifiers.clear();
	language.declarations.clear();

	for( i = 0; i < numKeyWords; i++ )
	{
		if( keyWords[i].description && keyWords[i].description[0] != '\0' )
		{
			language.identifiers.insert( keyWords[i].keyWord );
		}
		else
		{
			language.keywords.insert( keyWords[i].keyWord );
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
SyntaxRichEditCtrl::FindKeyWord
================
*/
ID_INLINE int SyntaxRichEditCtrl::FindKeyWord( const char* keyWord, int length ) const
{
	int i, hash;

	if( caseSensitive )
	{
		hash = idStr::Hash( keyWord, length );
	}
	else
	{
		hash = idStr::IHash( keyWord, length );
	}
	for( i = keyWordHash.First( hash ); i != -1; i = keyWordHash.Next( i ) )
	{
		if( length != keyWordLengths[i] )
		{
			continue;
		}
		if( caseSensitive )
		{
			if( idStr::Cmpn( keyWords[i].keyWord, keyWord, length ) != 0 )
			{
				continue;
			}
		}
		else
		{
			if( idStr::Icmpn( keyWords[i].keyWord, keyWord, length ) != 0 )
			{
				continue;
			}
		}
		return i;
	}
	return -1;
}

/*
================
SyntaxRichEditCtrl::SetKeyWords
================
*/
void SyntaxRichEditCtrl::SetKeyWords( const keyWord_t kws[] )
{
	keyWords = kws;
	RebuildLanguage();
}

/*
================
SyntaxRichEditCtrl::LoadKeyWordsFromFile
================
*/
bool SyntaxRichEditCtrl::LoadKeyWordsFromFile( const char* fileName )
{
	idParser src;
	idToken token, name, description;
	byte red, green, blue;
	keyWord_t keyword;

	if( !src.LoadFile( fileName ) )
	{
		return false;
	}

	FreeKeyWordsFromFile();

	while( src.ReadToken( &token ) )
	{
		if( token.Icmp( "keywords" ) == 0 )
		{
			src.ExpectTokenString( "{" );
			while( src.ReadToken( &token ) )
			{
				if( token == "}" )
				{
					break;
				}
				if( token == "{" )
				{

					// parse name
					src.ExpectTokenType( TT_STRING, 0, &name );
					src.ExpectTokenString( "," );

					// parse color
					src.ExpectTokenString( "(" );
					src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
					red = token.GetIntValue();
					src.ExpectTokenString( "," );
					src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
					green = token.GetIntValue();
					src.ExpectTokenString( "," );
					src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
					blue = token.GetIntValue();
					src.ExpectTokenString( ")" );
					src.ExpectTokenString( "," );

					// parse description
					src.ExpectTokenType( TT_STRING, 0, &description );
					src.ExpectTokenString( "}" );

					keyword.keyWord = Mem_CopyString( name );
					keyword.color = idVec3( red / 255.0f, green / 255.0f, blue / 255.0f );
					keyword.description = Mem_CopyString( description );

					keyWordsFromFile.Append( keyword );
				}
			}
		}
		else
		{
			src.SkipBracedSection();
		}
	}

	keyword.keyWord = NULL;
	keyword.color = idVec3( 1.0f, 1.0f, 1.0f );
	keyword.description = NULL;
	keyWordsFromFile.Append( keyword );

	SetKeyWords( keyWordsFromFile.Ptr() );

	return true;
}

/*
================
SyntaxRichEditCtrl::FreeKeyWordsFromFile
================
*/
void SyntaxRichEditCtrl::FreeKeyWordsFromFile()
{
	for( int i = 0; i < keyWordsFromFile.Num(); i++ )
	{
		Mem_Free( const_cast<char*>( keyWordsFromFile[i].keyWord ) );
		Mem_Free( const_cast<char*>( keyWordsFromFile[i].description ) );
	}
	keyWordsFromFile.Clear();
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

	for( int i = 0; keyWords[i].keyWord; i++ )
	{
		if( state.searchTerm.empty()
				|| idStr::Cmpn( keyWords[i].keyWord, state.searchTerm.c_str(), ( int )state.searchTerm.length() ) == 0 )
		{
			state.suggestions.push_back( keyWords[i].keyWord );
		}
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

	int keyWordIndex = FindKeyWord( word.c_str(), word.Length() );
	if( keyWordIndex != -1 && keyWords[keyWordIndex].description[0] != '\0' )
	{
		ImGui::TextUnformatted( keyWords[keyWordIndex].description );
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