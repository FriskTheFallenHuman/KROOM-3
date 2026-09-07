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

#include "../util/ImGui_IdWidgets.h"

#include "ScriptEditor.h"

typedef struct scriptEventInfo_s
{
	idStr		name;
	idStr		parms;
	idStr		help;
} scriptEventInfo_t;

static idList<scriptEventInfo_t> scriptEvents;

/*
================
GetScriptEvents
================
*/
static bool GetScriptEvents( const char* objectName, idList<idStr>& listBox )
{
	for( int i = 0; i < scriptEvents.Num(); i++ )
	{
		listBox.Append( scriptEvents[i].name );
	}
	return true;
}

/*
================
GetFunctionParms
================
*/
static bool GetFunctionParms( const char* funcName, idStr& parmString )
{
	for( int i = 0; i < scriptEvents.Num(); i++ )
	{
		if( scriptEvents[i].name.Cmp( funcName ) == 0 )
		{
			parmString = scriptEvents[i].parms;
			return true;
		}
	}
	return false;
}

/*
================
GetToolTip
================
*/
static bool GetToolTip( const char* name, idStr& string )
{
	for( int i = 0; i < scriptEvents.Num(); i++ )
	{
		if( scriptEvents[i].name.Cmp( name ) == 0 )
		{
			string = scriptEvents[i].help + scriptEvents[i].parms;
			return true;
		}
	}
	return false;
}

// ScriptEditor dialog

ScriptEditor& ScriptEditor::Instance()
{
	static ScriptEditor instance;
	return instance;
}

void ScriptEditor::Reset()
{
	windowText = "Script Editor###ScriptEditor";
	errorText.Clear();
	statusBarText.Clear();
	scriptEdit.SetText( "" );
	//scriptEdit.SetTabSize( TAB_SIZE );
	fileName.Clear();
	firstLine = 0;

	UpdateStatusBar();
}

void ScriptEditor::DrawContents( bool& showTool )
{
	bool clickedNew = false;
	bool clickedSelect = false;

	if( ImGui::BeginMenuBar() )
	{
		if( ImGui::BeginMenu( "File" ) )
		{
			if( ImGui::MenuItem( "New", "Ctrl+N" ) )
			{
				clickedNew = true;
			}
			if( ImGui::MenuItem( "Open..", "Ctrl+O" ) )
			{
				clickedSelect = true;
			}
			if( ImGui::MenuItem( "Save", "Ctrl+S", false, scriptEdit.IsEdited() ) )
			{
				OnBnClickedOk();
			}
			ImGui::Separator();
			if( ImGui::MenuItem( "Close", "Ctrl+W" ) )
			{
				showTool = false;
			}
			ImGui::EndMenu();
		}

		if( ImGui::BeginMenu( "Edit" ) )
		{
			if( ImGui::MenuItem( "Undo", "Ctrl+Z", false, scriptEdit.CanUndo() ) )
			{
				scriptEdit.Undo();
			}
			if( ImGui::MenuItem( "Redo", "Ctrl+Y", false, scriptEdit.CanRedo() ) )
			{
				scriptEdit.Redo();
			}
			ImGui::Separator();
			if( ImGui::MenuItem( "Cut", "Ctrl+X", false, scriptEdit.CanCut() ) )
			{
				scriptEdit.Cut();
			}
			if( ImGui::MenuItem( "Copy", "Ctrl+C", false, scriptEdit.CanCopy() ) )
			{
				scriptEdit.Copy();
			}
			if( ImGui::MenuItem( "Paste", "Ctrl+V", false, scriptEdit.CanPaste() ) )
			{
				scriptEdit.Paste();
			}
			ImGui::Separator();
			if( ImGui::MenuItem( "Delete", "Del", false, scriptEdit.CanDelete() ) )
			{
				scriptEdit.Delete();
			}
			ImGui::EndMenu();
		}

		if( ImGui::BeginMenu( "Misc" ) )
		{
			if( ImGui::MenuItem( "Find / Replace", "Ctrl+F" ) )
			{
				scriptEdit.GetTextEditor()->OpenFindReplaceWindow();
			}
			if( ImGui::MenuItem( "Go To Line", "Ctrl+G" ) )
			{
				scriptEdit.OnEditGoToLine();
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	if( clickedNew ) {}
	if( clickedSelect ) {}

	if( ImGui::IsKeyChordPressed( ImGuiMod_Ctrl | ImGuiKey_G ) )
	{
		scriptEdit.OnEditGoToLine();
	}

	scriptEdit.Draw();

	float bar_height = ImGui::GetFrameHeight();
	float status_bar_y = ImGui::GetWindowHeight() - bar_height - ImGui::GetStyle().WindowPadding.y;
	ImGui::SetCursorPosY( status_bar_y );

	ImGui::Separator();

	ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "%s", errorText.c_str() );
	ImGui::SameLine();

	UpdateStatusBar();

	ImGui::SameLine( ImGui::GetWindowWidth() - 140.0f );
	ImGui::TextUnformatted( statusBarText.c_str() );
}

void ScriptEditor::OnClosed()
{
	com_editors &= ~EDITOR_SCRIPT;
	imguiSystem->GetEditor()->ReleaseMouse( false );
}

/*
================
ScriptEditor::UpdateStatusBar
================
*/
void ScriptEditor::UpdateStatusBar()
{
	int line, column, character;

	scriptEdit.GetCursorPos( line, column, character );

	statusBarText = va( "Line: %d, Column: %d", line + 1, column + 1 );
}

/*
================
ScriptEditor::InitScriptEvents
================
*/
void ScriptEditor::InitScriptEvents()
{
	int index;
	idParser src;
	idToken token;
	idStr whiteSpace;
	scriptEventInfo_t info;

	if( !src.LoadFile( "script/doom_events.script" ) )
	{
		return;
	}

	scriptEvents.Clear();

	while( src.ReadToken( &token ) )
	{
		if( token == "scriptEvent" )
		{

			src.GetLastWhiteSpace( whiteSpace );
			index = whiteSpace.Find( "//" );
			if( index != -1 )
			{
				info.help = whiteSpace.Right( whiteSpace.Length() - index );
				info.help.Replace( "\r", "" );
				info.help.Replace( "\n", "\r\n" );
			}
			else
			{
				info.help = "";
			}

			src.ExpectTokenType( TT_NAME, 0, &token );

			info.parms = token;

			src.ExpectTokenType( TT_NAME, 0, &token );

			info.name = token;

			src.ExpectTokenString( "(" );

			info.parms += " " + info.name + "(";
			while( src.ReadToken( &token ) && token != ";" )
			{
				info.parms.Append( " " + token );
			}

			scriptEvents.Append( info );
		}
	}
}

/*
================
ScriptEditor::OpenFile
================
*/
void ScriptEditor::OpenFile( const char* fileName )
{
	int numLines = 0;
	int numCharsPerLine = 0;
	int maxCharsPerLine = 0;
	idStr scriptText, extension;
	void* buffer;

	scriptEdit.AllowPathNames( false );
	scriptEdit.SetText( "" );

	idStr( fileName ).ExtractFileExtension( extension );

	if( extension.Icmp( "script" ) == 0 )
	{
		InitScriptEvents();

		scriptEdit.SetCaseSensitive( true );
		scriptEdit.LoadKeyWordsFromFile( "editors/script.def" );
		scriptEdit.SetObjectMemberCallback( GetScriptEvents );
		scriptEdit.SetFunctionParmCallback( GetFunctionParms );
		scriptEdit.SetToolTipCallback( GetToolTip );

	}
	else if( extension.Icmp( "gui" ) == 0 )
	{

		//scriptEdit.SetStringColor(SRE_COLOR_DARK_CYAN, SRE_COLOR_LIGHT_BROWN);
		scriptEdit.SetCaseSensitive( false );
		scriptEdit.LoadKeyWordsFromFile( "editors/gui.def" );
		scriptEdit.SetObjectMemberCallback( NULL );
		scriptEdit.SetFunctionParmCallback( NULL );
		scriptEdit.SetToolTipCallback( NULL );
	}

	if( fileSystem->ReadFile( fileName, &buffer ) == -1 )
	{
		errorText = "Unable to read the selected file";
		return;
	}
	scriptText = ( char* ) buffer;
	fileSystem->FreeFile( buffer );

	this->fileName = fileName;

	scriptEdit.SetText( scriptText.c_str() );

	for( const char* ptr = scriptText.c_str(); *ptr; ptr++ )
	{
		if( *ptr == '\r' )
		{
			if( numCharsPerLine > maxCharsPerLine )
			{
				maxCharsPerLine = numCharsPerLine;
			}
			numCharsPerLine = 0;
			numLines++;
		}
		else if( *ptr == '\t' )
		{
			numCharsPerLine += TAB_SIZE;
		}
		else
		{
			numCharsPerLine++;
		}
	}

	windowText = va( "Script Editor (%s)###ScriptEditor", fileName );

	UpdateStatusBar();

	scriptEdit.SetFocus();
}

// ScriptEditor message handlers

/*
================
ScriptEditor::OnBnClickedOk
================
*/
void ScriptEditor::OnBnClickedOk()
{
	idStr scriptText;

	common->Printf( "Writing \'%s\'...\n", fileName.c_str() );

	scriptEdit.GetText( scriptText );

	if( fileSystem->WriteFile( fileName, scriptText, scriptText.Length(), "fs_savepath" ) == -1 )
	{
		errorText = va( "Couldn't save: %s", fileName.c_str() );
		return;
	}
}

/*
================
ScriptEditor::OnBnClickedCancel
================
*/
void ScriptEditor::OnBnClickedCancel()
{
	if( scriptEdit.IsEdited() )
	{
		//if ( MessageBox( "Cancel changes?", "Cancel", MB_YESNO | MB_ICONQUESTION ) != IDYES ) {
		return;
		//}
	}
	//OnCancel();
}

/*
==================
ScriptEditorInit
==================
*/
void ScriptEditorInit( const idDict* spawnArgs )
{
	com_editors |= EDITOR_SCRIPT;

	ScriptEditor::Instance().ShowIt( true );
	imguiSystem->GetEditor()->RegisterWindow( ScriptEditor::Instance() );
	imguiSystem->GetEditor()->ReleaseMouse( true );

	if( spawnArgs )
	{
		const char* str = spawnArgs->GetString( "script" );
		ScriptEditor::Instance().OpenFile( str );
	}
}