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

#include "DeviceConsole.h"
#include "doom_ico.h"
#include "../tools/common/ConsoleWidget.h"
#include "../tools/common/ImageUtils.h"

static ConsoleFrame* s_console = NULL;
static bool s_consoleInited = false;
static idStr s_returnedText;
static bool s_quitRequested = false;

/*
=================
ConsoleFrame::ConsoleFrame
=================
*/
ConsoleFrame::ConsoleFrame()
	: wxFrame( NULL, wxID_ANY, GAME_NAME, wxDefaultPosition, wxSize( 560, 500 ), wxDEFAULT_FRAME_STYLE )
	, m_console( NULL )
	, m_errorText( NULL )
	, m_quitOnClose( false )
{
	wxIcon appIcon = ImageUtils::IconFromEmbedded( doom_icon.pixel_data, doom_icon.width, doom_icon.height, doom_icon.bytes_per_pixel );
	if( appIcon.IsOk() )
	{
		SetIcon( appIcon );
	}

	m_blinkTimer.SetOwner( this );
	Bind( wxEVT_TIMER, &ConsoleFrame::OnBlinkTimer, this );

	// Error banner.
	wxFont m_errorTextFont( 12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD );

	m_errorText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1, 30 ), wxTE_READONLY | wxTE_CENTRE | wxBORDER_NONE );
	m_errorText->SetForegroundColour( *wxRED );
	m_errorText->SetFont( m_errorTextFont );
	m_errorText->Hide();

	// Console Widget
	m_console = new ConsoleWidget( this );

	const wxColour consoleBg( 0x00, 0x00, 0x80 );
	const wxColour consoleFg( 0xFF, 0xFF, 0x00 );

	m_console->SetOutputColors( consoleBg, consoleFg );

	// 8pt Courier New.
	wxFont consoleFont( 8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
	m_console->SetConsoleFont( consoleFont );

	m_console->SetCommandHandler( [this]( const char* cmd )
	{
		this->OnCommandFromConsole( cmd );
	} );
	m_console->SetQuitHandler( [this]()
	{
		this->OnQuitCommand( NULL );
	} );

	// Error banner on top.
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_errorText, 0, wxEXPAND | wxTOP, 10 );
	sizer->Add( m_console, 1, wxEXPAND );
	SetSizer( sizer );

	Bind( wxEVT_CLOSE_WINDOW, &ConsoleFrame::OnClose, this );

	CenterOnScreen();
}

/*
=================
ConsoleFrame::~ConsoleFrame
=================
*/
ConsoleFrame::~ConsoleFrame()
{
}

/*
=================
ConsoleFrame::ShowConsole
=================
*/
void ConsoleFrame::ShowConsole( int visLevel, bool quitOnClose )
{
	m_quitOnClose = quitOnClose;

	switch( visLevel )
	{
		case 0:
			Hide();
			break;
		case 1:
			Show();
			Raise();
			Layout();
			if( m_console )
			{
				m_console->FocusInput();
			}
			break;
		case 2:
			Iconize( true );
			break;
		default:
			Sys_Error( "Invalid visLevel %d sent to Sys_ShowConsole", visLevel );
			break;
	}
}

/*
=================
ConsoleFrame::AppendOutput
=================
*/
void ConsoleFrame::AppendOutput( const char* msg )
{
	if( m_console && msg )
	{
		m_console->AddText( msg );
	}
}

/*
=================
ConsoleFrame::SetErrorText
=================
*/
void ConsoleFrame::SetErrorText( const char* text )
{
	if( !m_errorText )
	{
		return;
	}

	if( text && text[0] )
	{
		m_errorText->ChangeValue( wxString::FromUTF8( text ) );
		m_errorText->SetForegroundColour( *wxRED );
		m_errorText->Show();

		m_blinkTimer.Start( 1000 );

		if( m_console )
		{
			m_console->SetInputEnabled( false );
		}
	}
	else
	{
		m_blinkTimer.Stop();
		m_errorText->Hide();

		if( m_console )
		{
			m_console->SetInputEnabled( true );
		}
	}

	Layout();
}

/*
=================
ConsoleFrame::OnBlinkTimer
=================
*/
void ConsoleFrame::OnBlinkTimer( wxTimerEvent& /*event*/ )
{
	if( m_errorText && m_errorText->IsShown() )
	{
		if( m_errorText->GetForegroundColour() == *wxRED )
		{
			m_errorText->SetForegroundColour( wxColour( 100, 0, 0 ) );
		}
		else
		{
			m_errorText->SetForegroundColour( *wxRED );
		}
		m_errorText->Refresh();
	}
}

/*
=================
ConsoleFrame::DrainInput
=================
*/
bool ConsoleFrame::DrainInput( idStr& out )
{
	if( m_pendingCommands.Length() == 0 )
	{
		return false;
	}

	out = m_pendingCommands;
	m_pendingCommands.Clear();
	return true;
}

/*
=================
ConsoleFrame::OnClose
=================
*/
void ConsoleFrame::OnClose( wxCloseEvent& event )
{
	if( m_quitOnClose )
	{
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
	else
	{
		Hide();
	}

	event.Veto();  // don't actually destroy the window on close
}

/*
=================
ConsoleFrame::OnQuitCommand
=================
*/
void ConsoleFrame::OnQuitCommand( const char* /*cmd*/ )
{
	if( m_quitOnClose )
	{
		s_quitRequested = true;
	}
	else
	{
		ShowConsole( 0, false );
	}
}

/*
=================
ConsoleFrame::OnCommandFromConsole
=================
*/
void ConsoleFrame::OnCommandFromConsole( const char* cmd )
{
	// Echo with the "]" prompt prefix.
	Sys_Printf( "]%s\n", cmd );

	// Accumulate for the engine to pick up on its next Sys_ConsoleInput call.
	m_pendingCommands += cmd;
	m_pendingCommands += "\n";
}

/*
=================
Sys_CreateConsole
=================
*/
void Sys_CreateConsole()
{
	if( s_consoleInited )
	{
		return;
	}

	wxApp::SetInstance( new DeviceConsoleApp() );

	if( !wxInitialize() )
	{
		return;
	}

	s_consoleInited = true;

	s_console = new ConsoleFrame();
}

/*
=================
Sys_DestroyConsole
=================
*/
void Sys_DestroyConsole()
{
	if( s_console )
	{
		s_console->Destroy();
		s_console = NULL;
	}
}

/*
=================
Sys_ShowConsole
=================
*/
void Sys_ShowConsole( int visLevel, bool quitOnClose )
{
	if( s_console )
	{
		s_console->ShowConsole( visLevel, quitOnClose );
	}
}

/*
=================
Sys_ConsoleInput
=================
*/
char* Sys_ConsoleInput()
{
	if( !s_console )
	{
		return NULL;
	}

	if( !s_console->DrainInput( s_returnedText ) )
	{
		return NULL;
	}

	return const_cast<char*>( s_returnedText.c_str() );
}

/*
=================
Sys_ConsoleExists
=================
*/
bool Sys_ConsoleExists()
{
	return s_console != NULL;
}

/*
=================
Sys_ConsoleQuitRequested
=================
*/
bool Sys_ConsoleQuitRequested()
{
	return s_quitRequested;
}

/*
=================
Conbuf_AppendText
=================
*/
void Conbuf_AppendText( const char* pMsg )
{
	if( !pMsg )
	{
		return;
	}

	if( s_console )
	{
		s_console->AppendOutput( pMsg );
	}
	else
	{
		Sys_Printf( "%s", pMsg );
	}
}

/*
=================
Win_SetErrorText
=================
*/
void Win_SetErrorText( const char* buf )
{
	if( s_console )
	{
		s_console->SetErrorText( buf );
	}
}