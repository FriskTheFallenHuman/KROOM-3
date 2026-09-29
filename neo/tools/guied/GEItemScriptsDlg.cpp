/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software

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

#include "GEApp.h"

namespace
{
// Case-insensitive look up, because somepeople really can't be nice...
bool KeywordExistsNoCase( const SyntaxKeywords& keywords, const char* name )
{
	if( !name || !name[0] )
	{
		return false;
	}

	for( int i = 0; i < keywords.GetCount(); ++i )
	{
		if( idStr::Icmp( keywords.GetEntry( i ).name.c_str(), name ) == 0 )
		{
			return true;
		}
	}
	return false;
}
}

/*
================
rvGEItemScriptsDlg::rvGEItemScriptsDlg
================
*/
rvGEItemScriptsDlg::rvGEItemScriptsDlg( wxWindow* parent, idWindow* window )
	: wxDialog( parent, wxID_ANY, "Item Scripts", wxDefaultPosition, wxSize( 640, 480 ), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
	, m_window( window )
	, m_applied( false )
	, m_nextStyle( STYLE_FIRST_KEYWORD )
{
	m_edit = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE );

	wxFont mono( 9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );

	m_edit->StyleSetFont( STYLE_DEFAULT, mono );
	m_edit->StyleSetForeground( STYLE_DEFAULT, wxColour( 30, 30, 30 ) );
	m_edit->StyleSetBackground( STYLE_DEFAULT, *wxWHITE );
	m_edit->StyleSetFont( STYLE_COMMENT, mono );
	m_edit->StyleSetForeground( STYLE_COMMENT, wxColour( 0, 128, 0 ) );
	m_edit->StyleSetItalic( STYLE_COMMENT, true );
	m_edit->StyleSetBackground( STYLE_COMMENT, *wxWHITE );
	for( int s = STYLE_FIRST_KEYWORD; s < 32; ++s )
	{
		m_edit->StyleSetFont( s, mono );
		m_edit->StyleSetBold( s, true );
		m_edit->StyleSetBackground( s, *wxWHITE );
	}
	m_edit->StyleClearAll();
	m_edit->SetTabWidth( 4 );
	m_edit->SetUseTabs( true );
	m_edit->SetMarginWidth( 0, 32 );
	m_edit->SetLexer( wxSTC_LEX_CONTAINER );
	m_edit->SetMouseDwellTime( 400 );
	m_edit->Bind( wxEVT_STC_DWELLSTART, &rvGEItemScriptsDlg::OnDwellStart, this );
	m_edit->Bind( wxEVT_STC_DWELLEND, &rvGEItemScriptsDlg::OnDwellEnd, this );
	m_edit->SetStyleBits( 5 );

	m_edit->Bind( wxEVT_STC_STYLENEEDED, &rvGEItemScriptsDlg::OnStyleNeeded, this );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_edit, 1, wxEXPAND | wxALL, 6 );
	sizer->Add( CreateButtonSizer( wxOK | wxCANCEL ), 0, wxEXPAND | wxALL, 6 );
	SetSizer( sizer );

	Bind( wxEVT_CLOSE_WINDOW, &rvGEItemScriptsDlg::OnClose, this );
	Bind( wxEVT_CHAR_HOOK, &rvGEItemScriptsDlg::OnCharHook, this );

	Init();
}

/*
================
rvGEItemScriptsDlg::~rvGEItemScriptsDlg
================
*/
rvGEItemScriptsDlg::~rvGEItemScriptsDlg()
{

}

/*
================
rvGEItemScriptsDlg::Init
================
*/
bool rvGEItemScriptsDlg::Init()
{
	rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( m_window );
	if( !wrapper )
	{
		return false;
	}

	if( m_keywords.GetCount() == 0 )
	{
		if( !m_keywords.LoadFromFile( "editors/defs/gui.def" ) )
		{
			// Auto-append events and/or commands the .def missed.
			static const idVec3 kEventFallbackColor( 0.0f, 0.0f, 0.7f );
			static const idVec3 kCommandFallbackColor( 0.7f, 0.0f, 0.0f );

			for( int i = 0; i < idWindow::SCRIPT_COUNT; ++i )
			{
				const char* name = idWindow::ScriptNames[i];
				if( name && name[0] && !m_keywords.Find( name ) )
				{
					m_keywords.Add( name, "built-in script event", kEventFallbackColor );
				}
			}

			const int cmdCount = idGuiScript::GetCommandCount();
			for( int i = 0; i < cmdCount; ++i )
			{
				const char* name = idGuiScript::GetCommandName( i );
				if( !name || !name[0] || m_keywords.Find( name ) )
				{
					continue;
				}
				idStr desc = va( "script command (%d to %d parameters)", idGuiScript::GetCommandMinParms( i ), idGuiScript::GetCommandMaxParms( i ) );
				m_keywords.Add( name, desc, kCommandFallbackColor );
			}
		}

		wxFont mono( 9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
		BuildKeywordStyleMap( mono );
	}

	// build the text
	wxString text;

	const idDict& vars = wrapper->GetVariableDict();
	for( int i = 0; i < vars.GetNumKeyVals(); ++i )
	{
		const idKeyValue* kv = vars.GetKeyVal( i );
		if( !kv )
		{
			continue;
		}
		text += wxString::FromUTF8( kv->GetKey().c_str() );
		text += "\t";
		text += wxString::FromUTF8( kv->GetValue().c_str() );
		text += "\r\n";
	}
	if( vars.GetNumKeyVals() )
	{
		text += "\r\n";
	}

	const idDict& scripts = wrapper->GetScriptDict();
	for( int i = 0; i < scripts.GetNumKeyVals(); ++i )
	{
		const idKeyValue* kv = scripts.GetKeyVal( i );
		if( !kv )
		{
			continue;
		}
		text += wxString::FromUTF8( kv->GetKey().c_str() );
		text += "\r\n";
		text += wxString::FromUTF8( kv->GetValue().c_str() );
		text += "\r\n\r\n";
	}

	m_edit->SetText( text );

	ApplyStyles();

	m_edit->GotoPos( 0 );
	m_edit->SetFocus();
	return true;
}

/*
================
rvGEItemScriptsDlg::Apply
================
*/
bool rvGEItemScriptsDlg::Apply()
{
	rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( m_window );
	if( !wrapper )
	{
		return false;
	}

	wxString text = m_edit->GetText();
	idStr parse = text.utf8_str();

	try
	{
		idParser src( parse, parse.Length(), "", LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
		idToken token;

		wrapper->GetVariableDict().Clear();
		wrapper->GetScriptDict().Clear();

		while( src.ReadToken( &token ) )
		{
			idStr scriptName;
			idStr out;

			if( !token.Icmp( "definevec4" ) )
			{
				idToken token2;
				if( !src.ReadToken( &token2 ) )
				{
					src.Error( "expected define name" );
					return false;
				}

				idWinVec4 var;
				idUserInterfaceLocal ui;
				idWindow tempwin( &ui );

				src.SetMarker();
				for( int i = 0; i < 3; ++i )
				{
					tempwin.ParseExpression( &src, &var );
					src.ExpectTokenString( "," );
				}
				tempwin.ParseExpression( &src, &var );
				src.GetStringFromMarker( out, true );

				wrapper->GetVariableDict().Set( token + "\t\"" + token2 + "\"", out );
				continue;
			}
			else if( !token.Icmp( "definefloat" ) || !token.Icmp( "float" ) )
			{
				idToken token2;
				if( !src.ReadToken( &token2 ) )
				{
					src.Error( "expected define name" );
					return false;
				}

				idWinFloat var;
				idUserInterfaceLocal ui;
				idWindow tempwin( &ui );

				src.SetMarker();
				tempwin.ParseExpression( &src, &var );
				src.GetStringFromMarker( out, true );

				wrapper->GetVariableDict().Set( token + "\t\"" + token2 + "\"", out );
				continue;
			}
			else if( !token.Icmp( "scriptdef" ) )
			{
				scriptName = "scriptDef";
			}
			else if( !token.Icmp( "ontime" ) )
			{
				if( !src.ReadToken( &token ) )
				{
					src.Error( "expected time" );
					return false;
				}
				scriptName = "onTime ";
				scriptName.Append( token );
			}
			else if( !token.Icmp( "onevent" ) )
			{
				if( !src.ReadToken( &token ) )
				{
					src.Error( "expected event" );
					return false;
				}
				scriptName = "onEvent ";
				scriptName.Append( token );
			}
			else
			{
				int i = 0;
				for( ; i < idWindow::SCRIPT_COUNT; ++i )
				{
					if( idStr::Icmp( idWindow::ScriptNames[i], token ) == 0 )
					{
						scriptName = idWindow::ScriptNames[i];
						break;
					}
				}
				if( i >= idWindow::SCRIPT_COUNT )
				{
					src.Error( "expected script name" );
					return false;
				}
			}

			if( !src.ParseBracedSectionExact( out, 1 ) )
			{
				return false;
			}

			wrapper->GetScriptDict().Set( scriptName, out );
		}
	}
	catch( idException& e )
	{
		wxMessageBox( e.GetError(), "Script Error",
					  wxOK | wxICON_ERROR, this );
		return false;
	}

	return true;
}

/*
================
rvGEItemScriptsDlg::OnClose
================
*/
void rvGEItemScriptsDlg::OnClose( wxCloseEvent& e )
{
	if( e.CanVeto() && !Apply() )
	{
		e.Veto( true );
		return;
	}
	m_applied = true;
	e.Skip();
}

/*
================
rvGEItemScriptsDlg::OnCharHook
================
*/
void rvGEItemScriptsDlg::OnCharHook( wxKeyEvent& e )
{
	if( e.GetKeyCode() == WXK_ESCAPE )
	{
		Close( true );
		return;
	}
	e.Skip();
}

/*
================
rvGEItemScriptsDlg::BuildKeywordStyleMap
================
*/
void rvGEItemScriptsDlg::BuildKeywordStyleMap( const wxFont& mono )
{
	m_colourToStyle.clear();
	m_wordToStyle.clear();
	m_nextStyle = STYLE_FIRST_KEYWORD;

	for( int i = 0; i < m_keywords.GetCount(); ++i )
	{
		const SyntaxKeywords::Entry& e = m_keywords.GetEntry( i );

		const unsigned char r = ( unsigned char )( e.color.x * 255.0f );
		const unsigned char g = ( unsigned char )( e.color.y * 255.0f );
		const unsigned char b = ( unsigned char )( e.color.z * 255.0f );

		const unsigned long key = ( ( unsigned long )r << 16 ) |
								  ( ( unsigned long )g <<  8 ) |
								  ( unsigned long )b;

		int style;
		auto it = m_colourToStyle.find( key );
		if( it == m_colourToStyle.end() )
		{
			if( m_nextStyle >= 32 )
			{
				style = m_nextStyle - 1;
			}
			else
			{
				style = m_nextStyle++;
				m_edit->StyleSetFont( style, mono );
				m_edit->StyleSetBold( style, true );
				m_edit->StyleSetBackground( style, *wxWHITE );
				m_edit->StyleSetForeground( style, wxColour( r, g, b ) );
				m_colourToStyle[key] = style;
			}
		}
		else
		{
			style = it->second;
		}

		m_wordToStyle[wxString::FromUTF8( e.name.c_str() )] = style;
	}
}

/*
================
rvGEItemScriptsDlg::OnStyleNeeded
================
*/
void rvGEItemScriptsDlg::OnStyleNeeded( wxStyledTextEvent& /*e*/ )
{
	ApplyStyles();
}

/*
================
rvGEItemScriptsDlg::ApplyStyles

Walks the buffer and stamps out style runs.
================
*/
void rvGEItemScriptsDlg::ApplyStyles()
{
	if( !m_edit )
	{
		return;
	}

	const int len = m_edit->GetTextLength();
	if( len <= 0 )
	{
		return;
	}

	const wxString text = m_edit->GetText();
	const int n = ( int )text.length();

	m_edit->StartStyling( 0 );

	int i = 0;
	int defaultStart = 0;   // first byte of the current un-styled run

	auto flushDefault = [&]( int end )
	{
		if( end > defaultStart )
		{
			m_edit->SetStyling( end - defaultStart, STYLE_DEFAULT );
		}
	};

	while( i < n )
	{
		const wxUniChar c = text[i];

		// line comments
		if( c == '/' && i + 1 < n && text[i + 1] == '/' )
		{
			flushDefault( i );
			int end = i;
			while( end < n && text[end] != '\n' )
			{
				++end;
			}
			m_edit->SetStyling( end - i, STYLE_COMMENT );
			i = end;
			defaultStart = i;
			continue;
		}

		// identifiers / keywords
		if( wxIsalpha( c ) || c == '_' )
		{
			int end = i;
			while( end < n && ( wxIsalnum( text[end] ) || text[end] == '_' ) )
			{
				++end;
			}

			const wxString word = text.Mid( i, end - i );
			auto it = m_wordToStyle.find( word );
			if( it != m_wordToStyle.end() )
			{
				flushDefault( i );
				m_edit->SetStyling( end - i, it->second );
				defaultStart = end;
			}

			i = end;
			continue;
		}

		++i;
	}

	// Flush the tail.
	flushDefault( n );
}

/*
================
rvGEItemScriptsDlg::OnDwellStart
================
*/
void rvGEItemScriptsDlg::OnDwellStart( wxStyledTextEvent& e )
{
	if( !m_edit || m_keywords.GetCount() == 0 )
	{
		return;
	}

	int pos = e.GetPosition();
	if( pos < 0 || pos >= m_edit->GetTextLength() )
	{
		return;
	}

	// Expand to the whole word under the cursor.
	const int start = m_edit->WordStartPosition( pos, true );
	const int end   = m_edit->WordEndPosition( pos, true );
	if( start == end )
	{
		return;
	}

	const wxString word = m_edit->GetTextRange( start, end );
	const SyntaxKeywords::Entry* entry = m_keywords.Find( word.utf8_str() );
	if( !entry || entry->description.IsEmpty() )
	{
		return;
	}

	m_edit->CallTipShow( start, wxString::FromUTF8( entry->description.c_str() ) );
}

/*
================
rvGEItemScriptsDlg::OnDwellEnd
================
*/
void rvGEItemScriptsDlg::OnDwellEnd( wxStyledTextEvent& /*e*/ )
{
	if( m_edit )
	{
		m_edit->CallTipCancel();
	}
}