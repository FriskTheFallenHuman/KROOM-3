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

#ifndef __SYNTAXRICHEDITCTR_H__
#define __SYNTAXRICHEDITCTR_H__

#include "../../extern/ImGuiColorTextEdit/TextEditor.h"
#include "../../common/SyntaxKeywords.h"

/*
===============================================================================

	Rich Edit Control with:

	- syntax highlighting
	- braced section highlighting
	- braced section auto-indentation
	- multi-line tabs
	- keyword auto-completion
	- object member auto-completion
	- keyword tool tip
	- function parameter tool tip

===============================================================================
*/

static const int		TAB_SIZE = 4;

typedef struct
{
	const char* 	keyWord;
	idVec3			color;
	const char* 	description;
} keyWord_t;

typedef bool ( *objectMemberCallback_t )( const char* objectName, idStrList& listBox );
typedef bool ( *toolTipCallback_t )( const char* name, idStr& string );

class SyntaxRichEditCtrl
{
public:
	SyntaxRichEditCtrl();
	~SyntaxRichEditCtrl();

	void					Init();
	void					Draw();

	void					SetCaseSensitive( bool caseSensitive );
	void					AllowPathNames( bool allow );
	void					EnableKeyWordAutoCompletion( bool enable );
	void					SetKeyWords( const keyWord_t kws[] );
	bool					LoadKeyWordsFromFile( const char* fileName );
	void					SetObjectMemberCallback( objectMemberCallback_t callback );
	void					SetFunctionParmCallback( toolTipCallback_t callback );
	void					SetToolTipCallback( toolTipCallback_t callback );

	void					GetCursorPos( int& line, int& column, int& character ) const;

	void					GetText( idStr& text ) const;
	void					SetText( const char* text );

	void					SetReadOnly( bool readOnly );
	bool					GetReadOnly();

	TextEditor*				GetTextEditor()
	{
		return scriptEdit;
	}

	bool					CanCopy();
	void					Copy();
	bool					CanCut();
	void					Cut();
	bool					CanPaste();
	void					Paste();
	bool					CanUndo();
	void					Undo();
	bool					CanRedo();
	void					Redo();
	bool					CanDelete();
	void					Delete();

	bool					IsEdited() const;

	void					SetFocus();

	void					OnEditGoToLine();

	SyntaxKeywords&			GetKeywords()
	{
		return m_keywords;
	}

private:
	TextEditor*				scriptEdit;
	ImVec2					scriptEditPos;
	ImVec2					scriptEditSize;
	GoToLineDialog			gotoDlg;
	MessageBoxDialog		msgBoxDlg;
	int						firstLine;

	SyntaxKeywords			m_keywords;

	bool					caseSensitive;
	bool					allowPathNames;
	bool					keyWordAutoCompletion;

	objectMemberCallback_t	GetObjectMembers;
	toolTipCallback_t		GetFunctionParms;
	toolTipCallback_t		GetToolTip;

	TextEditor::Language	language;
	TextEditor::AutoCompleteConfig	autoCompleteConfig;

private:
	void					RebuildLanguage();

	void					AutoCompleteCallback( TextEditor::AutoCompleteState& state );
	void					TextHoverCallback( TextEditor::PopupData& data );
};

#endif /* !__SYNTAXRICHEDITCTR_H__ */
