/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition Source Code ("Doom 3 BFG Edition Source Code").

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

#ifndef __CONSOLEWIDGET_H__
#define __CONSOLEWIDGET_H__

#include <wx/wx.h>
#include <wx/stc/stc.h>

#include <vector>
#include <functional>

class ConsoleWidget : public wxPanel
{
public:
	explicit ConsoleWidget( wxWindow* parent );
	virtual ~ConsoleWidget();

	// Handlers.
	typedef std::function<void( const char* command )> CommandHandler;
	typedef std::function<void()> QuitHandler;

	// Appends the given text to the output pane.
	void    AddText( const char* msg );

	// Clears the output pane.
	void    ClearOutput();

	// Sets the input line contents.
	void    SetInputText( const idStr& text );

	// Executes a given command.
	void    ExecuteCommand( const idStr& cmd = "" );

	// Hides the Copy/Clear and Quit buttons
	void    HideUtilityButtons( bool hide = true );

	// Quit Handlers
	void    SetQuitHandler( QuitHandler handler );
	void    ClearQuitHandler();

	// Copy the current output text to the clipboard.
	void    CopyOutputToClipboard();

	// Command Handlers
	void    SetCommandHandler( CommandHandler handler );
	void    ClearCommandHandler();

	// History
	const idStrList&    GetHistory() const
	{
		return m_history;
	}
	void	ClearHistory();

	// Theming
	void    SetOutputColors( const wxColour& background, const wxColour& foreground );
	void    SetInputColors( const wxColour& background, const wxColour& foreground );
	void    SetConsoleFont( const wxFont& font );

	// Enable or disable command entry.
	void    SetInputEnabled( bool enabled );

	// Move keyboard focus to the input line.
	void    FocusInput();

private:
	struct ColorRun
	{
		int     start;
		int     length;
		int     style;
	};

	void    SetupOutputPane();
	void    SetupColorStyles();

	void    ApplyColorRuns( int basePos, const std::vector<ColorRun>& runs );

	void    TrimOutputIfNeeded( int incomingLength );

	void    OnInputEnter( wxCommandEvent& event );
	void    OnInputKeyDown( wxKeyEvent& event );
	void    OnInputFocus( wxFocusEvent& event );

	void    BuildUtilityButtons();
	void    OnCopyClicked( wxCommandEvent& event );
	void    OnClearClicked( wxCommandEvent& event );
	void    OnQuitClicked( wxCommandEvent& event );

	void    HistoryUp();
	void    HistoryDown();
	void    PrintHistory();

private:
	CommandHandler      m_commandHandler;

	wxPanel*			m_buttonBar;
	wxButton*			m_copyButton;
	wxButton*			m_clearButton;
	wxButton*			m_quitButton;

	QuitHandler			m_quitHandler;

	wxStyledTextCtrl*   m_output;
	wxTextCtrl*         m_input;

	idStrList           m_history;
	idStr               m_currentCommand;
	int                 m_currentHistoryPosition;
	bool                m_saveCurrentCommand;

	static const int    kOutputLimit    = 32768;
	static const int    kMaxHistory     = 16;
	static const int    kColorStyleBase = 100;
};

#endif /* !__CONSOLEWIDGET_H__ */