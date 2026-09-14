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

#include <wx/clipbrd.h>

#include "ConsoleWidget.h"

/*
================
ConsoleWidget::ConsoleWidget
================
*/
ConsoleWidget::ConsoleWidget( wxWindow* parent )
	: wxPanel( parent )
	, m_output( nullptr )
	, m_input( nullptr )
	, m_buttonBar( nullptr )
	, m_copyButton( nullptr )
	, m_clearButton( nullptr )
	, m_quitButton( nullptr )
	, m_currentHistoryPosition( 0 )
	, m_saveCurrentCommand( true )
{
	// Layout metrics.
	const int kOuterPad      = 9;   // inset from the frame's client edges
	const int kOutputInputGap = 8;  // vertical gap between output and input

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	m_output = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE );

	m_input = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER | wxBORDER_SIMPLE );

	SetupOutputPane();
	SetupColorStyles();

	wxFont font( 9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
	m_output->StyleSetFont( wxSTC_STYLE_DEFAULT, font );
	m_input->SetFont( font );

	// Output pane: 12px inset from top, left, right.  No bottom padding
	// because the gap to the input line is controlled below.
	sizer->Add( m_output, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, kOuterPad );

	// Gap between output and input.
	sizer->AddSpacer( kOutputInputGap );

	// Input line: 12px inset from left and right.  No top or bottom padding;
	// the top gap is the spacer above, the bottom gap is the button bar's
	// top padding.
	sizer->Add( m_input, 0, wxEXPAND | wxLEFT | wxRIGHT, kOuterPad );

	// Button bar: 12px inset on all sides.
	BuildUtilityButtons();
	sizer->Add( m_buttonBar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, kOuterPad );

	SetSizer( sizer );

	m_input->Bind( wxEVT_TEXT_ENTER, &ConsoleWidget::OnInputEnter, this );
	m_input->Bind( wxEVT_KEY_DOWN, &ConsoleWidget::OnInputKeyDown, this );
	m_input->Bind( wxEVT_SET_FOCUS, &ConsoleWidget::OnInputFocus, this );
}

/*
================
ConsoleWidget::~ConsoleWidget
================
*/
ConsoleWidget::~ConsoleWidget()
{
}

/*
================
ConsoleWidget::SetupOutputPane
================
*/
void ConsoleWidget::SetupOutputPane()
{
	m_output->SetLexer( wxSTC_LEX_NULL );

	m_output->SetReadOnly( true );

	// Hide margins and caret.
	m_output->SetMarginWidth( 0, 0 );
	m_output->SetMarginWidth( 1, 0 );
	m_output->SetCaretLineVisible( false );
	m_output->SetCaretWidth( 0 );

	// Long lines wrap.
	m_output->SetWrapMode( wxSTC_WRAP_WORD );

	m_output->SetUseHorizontalScrollBar( false );
	m_output->SetUseVerticalScrollBar( true );
}

/*
================
ConsoleWidget::SetupColorStyles
================
*/
void ConsoleWidget::SetupColorStyles()
{
	for( int i = 0; i < 10; i++ )
	{
		const idVec4 c = idStr::ColorForIndex( i );

		const wxColour colour(
			( unsigned char )( idMath::ClampFloat( 0.0f, 1.0f, c.x ) * 255.0f ),
			( unsigned char )( idMath::ClampFloat( 0.0f, 1.0f, c.y ) * 255.0f ),
			( unsigned char )( idMath::ClampFloat( 0.0f, 1.0f, c.z ) * 255.0f ) );

		m_output->StyleSetForeground( kColorStyleBase + i, colour );
	}
}

/*
================
ConsoleWidget::TrimOutputIfNeeded
================
*/
void ConsoleWidget::TrimOutputIfNeeded( int incomingLength )
{
	const int currentLength = m_output->GetTextLength();
	if( currentLength + incomingLength <= kOutputLimit )
	{
		return;
	}

	// Trim the oldest 25% of the buffer.
	const int dropCount = ( int )( kOutputLimit * 0.25 );
	if( dropCount > currentLength )
	{
		return;
	}

	m_output->SetReadOnly( false );
	m_output->DeleteRange( 0, dropCount );
	m_output->SetReadOnly( true );
}


/*
================
ConsoleWidget::ApplyColorRuns
================
*/
void ConsoleWidget::ApplyColorRuns( int basePos, const std::vector<ColorRun>& runs )
{
	for( size_t i = 0; i < runs.size(); i++ )
	{
		const ColorRun& r = runs[i];
		m_output->StartStyling( basePos + r.start );
		m_output->SetStyling( r.length, r.style );
	}
}

/*
================
ConsoleWidget::BuildUtilityButtons
================
*/
void ConsoleWidget::BuildUtilityButtons()
{
	m_buttonBar = new wxPanel( this );

	m_copyButton  = new wxButton( m_buttonBar, wxID_ANY, "Copy" );
	m_clearButton = new wxButton( m_buttonBar, wxID_ANY, "Clear" );
	m_quitButton  = new wxButton( m_buttonBar, wxID_ANY, "Quit" );

	auto* barSizer = new wxBoxSizer( wxHORIZONTAL );
	barSizer->Add( m_copyButton,  0, wxLEFT | wxTOP | wxBOTTOM, 4 );
	barSizer->Add( m_clearButton, 0, wxLEFT | wxTOP | wxBOTTOM, 4 );
	barSizer->AddStretchSpacer( 1 );
	barSizer->Add( m_quitButton,  0, wxRIGHT | wxTOP | wxBOTTOM, 4 );

	m_buttonBar->SetSizer( barSizer );

	m_copyButton->Bind( wxEVT_BUTTON, &ConsoleWidget::OnCopyClicked,  this );
	m_clearButton->Bind( wxEVT_BUTTON, &ConsoleWidget::OnClearClicked, this );
	m_quitButton->Bind( wxEVT_BUTTON, &ConsoleWidget::OnQuitClicked,  this );

	m_quitButton->Hide();
}

/*
================
ConsoleWidget::HideUtilityButtons
================
*/
void ConsoleWidget::HideUtilityButtons( bool hide )
{
	if( !m_buttonBar )
	{
		return;
	}

	m_buttonBar->Show( !hide );

	if( m_quitButton )
	{
		m_quitButton->Show( !hide && ( m_quitHandler != nullptr ) );
	}

	Layout();
}

/*
================
ConsoleWidget::SetQuitHandler
================
*/
void ConsoleWidget::SetQuitHandler( QuitHandler handler )
{
	m_quitHandler = std::move( handler );

	if( m_quitButton )
	{
		m_quitButton->Show( m_buttonBar && m_buttonBar->IsShown() && ( m_quitHandler != nullptr ) );
		Layout();
	}
}

/*
================
ConsoleWidget::ClearQuitHandler
================
*/
void ConsoleWidget::ClearQuitHandler()
{
	m_quitHandler = nullptr;

	if( m_quitButton )
	{
		m_quitButton->Hide();
		Layout();
	}
}

/*
================
ConsoleWidget::CopyOutputToClipboard
================
*/
void ConsoleWidget::CopyOutputToClipboard()
{
	if( !m_output )
	{
		return;
	}

	// Prefer the selection if there is one, otherwise copy everything.
	wxString text = m_output->GetSelectedText();
	if( text.IsEmpty() )
	{
		text = m_output->GetText();
	}

	if( !text.IsEmpty() && wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new wxTextDataObject( text ) );
		wxTheClipboard->Close();
	}
}

/*
================
ConsoleWidget::OnCopyClicked
================
*/
void ConsoleWidget::OnCopyClicked( wxCommandEvent& /*event*/ )
{
	CopyOutputToClipboard();
}

/*
================
ConsoleWidget::OnClearClicked
================
*/
void ConsoleWidget::OnClearClicked( wxCommandEvent& /*event*/ )
{
	ClearOutput();
}

/*
================
ConsoleWidget::OnQuitClicked
================
*/
void ConsoleWidget::OnQuitClicked( wxCommandEvent& /*event*/ )
{
	if( m_quitHandler )
	{
		m_quitHandler();
	}
}

/*
================
ConsoleWidget::AddText
================
*/
void ConsoleWidget::AddText( const char* msg )
{
	if( !m_output || !msg )
	{
		return;
	}

	TrimOutputIfNeeded( ( int )strlen( msg ) );

	const int basePos = m_output->GetTextLength();

	idStr clean;
	std::vector<ColorRun> runs;

	int currentStyle = 0;   // 0 means "default style"
	int cleanRunStart = 0;

	const char* p = msg;
	while( *p )
	{
		if( p[0] == '^' && p[1] >= '0' && p[1] <= '9' )
		{
			// Flush the current output.
			const int cleanLen = clean.Length() - cleanRunStart;
			if( cleanLen > 0 && currentStyle != 0 )
			{
				ColorRun r;
				r.start  = cleanRunStart;
				r.length = cleanLen;
				r.style  = currentStyle;
				runs.push_back( r );
			}

			currentStyle  = kColorStyleBase + ( p[1] - '0' );
			cleanRunStart = clean.Length();
			p += 2;
		}
		else
		{
			clean += *p++;
		}
	}

	// Flush the final output.
	{
		const int cleanLen = clean.Length() - cleanRunStart;
		if( cleanLen > 0 && currentStyle != 0 )
		{
			ColorRun r;
			r.start  = cleanRunStart;
			r.length = cleanLen;
			r.style  = currentStyle;
			runs.push_back( r );
		}
	}

	// Append the text.
	m_output->SetReadOnly( false );
	m_output->AppendText( wxString::FromUTF8( clean.c_str() ) );

	ApplyColorRuns( basePos, runs );

	m_output->SetReadOnly( true );

	// Auto-scroll to the last line.
	const int lastLine = m_output->GetLineCount() - 1;
	if( lastLine >= 0 )
	{
		m_output->ScrollToLine( lastLine );
		m_output->GotoLine( lastLine );
	}
}

/*
================
ConsoleWidget::ClearOutput
================
*/
void ConsoleWidget::ClearOutput()
{
	m_output->SetReadOnly( false );
	m_output->ClearAll();
	m_output->SetReadOnly( true );
}

/*
================
ConsoleWidget::SetInputText
================
*/
void ConsoleWidget::SetInputText( const idStr& text )
{
	if( m_input )
	{
		m_input->ChangeValue( wxString::FromUTF8( text.c_str() ) );
	}
}

/*
================
ConsoleWidget::ClearHistory
================
*/
void ConsoleWidget::ClearHistory()
{
	m_history.Clear();
	m_currentHistoryPosition = 0;
	m_currentCommand.Clear();
	m_saveCurrentCommand = true;
}

/*
================
ConsoleWidget::SetOutputColors
================
*/
void ConsoleWidget::SetOutputColors( const wxColour& background, const wxColour& foreground )
{
	if( !m_output )
	{
		return;
	}

	m_output->StyleSetBackground( wxSTC_STYLE_DEFAULT, background );
	m_output->StyleSetForeground( wxSTC_STYLE_DEFAULT, foreground );
	m_output->StyleClearAll();

	SetupColorStyles();

	m_output->SetBackgroundColour( background );
	m_output->Refresh();
}

/*
================
ConsoleWidget::SetInputColors
================
*/
void ConsoleWidget::SetInputColors( const wxColour& background, const wxColour& foreground )
{
	if( !m_input )
	{
		return;
	}

	m_input->SetBackgroundColour( background );
	m_input->SetForegroundColour( foreground );
	m_input->Refresh();
}

/*
================
ConsoleWidget::SetConsoleFont
================
*/
void ConsoleWidget::SetConsoleFont( const wxFont& font )
{
	if( m_output )
	{
		m_output->StyleSetFont( wxSTC_STYLE_DEFAULT, font );
		m_output->StyleClearAll();
		SetupColorStyles();
		m_output->Refresh();
	}

	if( m_input )
	{
		m_input->SetFont( font );
		m_input->Refresh();
	}
}

/*
================
ConsoleWidget::SetInputEnabled
================
*/
void ConsoleWidget::SetInputEnabled( bool enabled )
{
	if( !m_input )
	{
		return;
	}

	m_input->SetEditable( enabled );

	if( enabled )
	{
		m_input->SetFocus();
	}
}

/*
================
ConsoleWidget::FocusInput
================
*/
void ConsoleWidget::FocusInput()
{
	if( m_input )
	{
		m_input->SetFocus();
	}
}

/*
================
ConsoleWidget::SetCommandHandler
================
*/
void ConsoleWidget::SetCommandHandler( CommandHandler handler )
{
	m_commandHandler = std::move( handler );
}

/*
================
ConsoleWidget::ClearCommandHandler
================
*/
void ConsoleWidget::ClearCommandHandler()
{
	m_commandHandler = nullptr;
}

/*
================
ConsoleWidget::ExecuteCommand
================
*/
void ConsoleWidget::ExecuteCommand( const idStr& cmd )
{
	wxString input;
	if( cmd.Length() > 0 )
	{
		input = wxString::FromUTF8( cmd.c_str() );
	}
	else if( m_input )
	{
		input = m_input->GetValue();
	}

	if( input.IsEmpty() )
	{
		return;
	}

	if( m_input )
	{
		m_input->ChangeValue( "" );
	}

	const wxScopedCharBuffer utf8 = input.ToUTF8();
	const char* inputUtf8 = utf8.data();

	// Echo the command back through.
	common->Printf( "%s\n", inputUtf8 );

	// History.
	idStr cmdStr( inputUtf8 );
	const int histCount = m_history.Num();

	if( histCount == 0 || cmdStr.Cmp( m_history[histCount - 1] ) != 0 )
	{
		if( m_history.Num() > kMaxHistory )
		{
			m_history.RemoveIndex( 0 );
		}
		m_currentHistoryPosition = m_history.Append( cmdStr );
	}
	else
	{
		m_currentHistoryPosition = m_history.Num() - 1;
	}

	m_currentCommand.Clear();
	m_saveCurrentCommand = true;

	// local commands.
	if( input.CmpNoCase( "clear" ) == 0 )
	{
		ClearOutput();
		return;
	}

	if( input.CmpNoCase( "edit" ) == 0 )
	{
		return;
	}

	if( m_commandHandler )
	{
		m_commandHandler( inputUtf8 );
	}
	else
	{
		common->Printf( "%s\n", inputUtf8 );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, inputUtf8 );
	}
}

/*
================
ConsoleWidget::OnInputEnter
================
*/
void ConsoleWidget::OnInputEnter( wxCommandEvent& /*event*/ )
{
	ExecuteCommand();
}

/*
================
ConsoleWidget::OnInputFocus
================
*/
void ConsoleWidget::OnInputFocus( wxFocusEvent& event )
{
	if( m_input )
	{
		m_input->SetInsertionPointEnd();
	}
	event.Skip();
}

/*
================
ConsoleWidget::OnInputKeyDown
================
*/
void ConsoleWidget::OnInputKeyDown( wxKeyEvent& event )
{
	switch( event.GetKeyCode() )
	{
		case WXK_RETURN:
		case WXK_NUMPAD_ENTER:
			ExecuteCommand();
			return;

		case WXK_UP:
			HistoryUp();
			return;

		case WXK_DOWN:
			HistoryDown();
			return;

		case WXK_TAB:
			PrintHistory();
			return;

		case WXK_PAGEDOWN:
			if( m_output )
			{
				m_output->LineScroll( 0, 10 );
			}
			return;

		case WXK_PAGEUP:
			if( m_output )
			{
				m_output->LineScroll( 0, -10 );
			}
			return;

		case WXK_HOME:
			if( m_output )
			{
				m_output->GotoLine( 0 );
			}
			return;

		case WXK_END:
			if( m_output )
			{
				m_output->GotoLine( m_output->GetLineCount() - 1 );
			}
			return;
	}

	event.Skip();
}

/*
================
ConsoleWidget::HistoryUp
================
*/
void ConsoleWidget::HistoryUp()
{
	if( m_history.Num() == 0 )
	{
		return;
	}

	if( m_saveCurrentCommand )
	{
		m_currentCommand = m_input->GetValue().ToUTF8().data();
		m_saveCurrentCommand = false;
	}

	m_input->ChangeValue( wxString::FromUTF8( m_history[m_currentHistoryPosition].c_str() ) );
	m_input->SetInsertionPointEnd();

	if( m_currentHistoryPosition > 0 )
	{
		--m_currentHistoryPosition;
	}
}

/*
================
ConsoleWidget::HistoryDown
================
*/
void ConsoleWidget::HistoryDown()
{
	if( m_history.Num() == 0 )
	{
		return;
	}

	if( m_currentHistoryPosition < m_history.Num() - 1 )
	{
		++m_currentHistoryPosition;
		m_input->ChangeValue( wxString::FromUTF8( m_history[m_currentHistoryPosition].c_str() ) );
	}
	else
	{
		m_input->ChangeValue( wxString::FromUTF8( m_currentCommand.c_str() ) );
		m_currentCommand.Clear();
		m_saveCurrentCommand = true;
	}

	m_input->SetInsertionPointEnd();
}

/*
================
ConsoleWidget::PrintHistory
================
*/
void ConsoleWidget::PrintHistory()
{
	common->Printf( "Command History\n----------------\n" );
	for( int i = 0; i < m_history.Num(); i++ )
	{
		common->Printf( "[cmd %d]:  %s\n", i, m_history[i].c_str() );
	}
	common->Printf( "----------------\n" );
}